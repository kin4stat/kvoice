#include "bass/bass.h"
#include "sound_output_impl.hpp"

#include "stream_impl.hpp"
#include "voice_exception.hpp"

kvoice::sound_output_impl::set_device_volume_message::set_device_volume_message(float volume)
    : task([this]() { return work_impl(); }),
      volume(volume) {
}

kvoice::sound_output_impl::set_device_message::set_device_message(std::string device_name)
    : task([this]() { return work_impl(); }),
      device_name(std::move(device_name)) {
}

kvoice::sound_output_impl::update_device_message::update_device_message(sound_output_impl& impl_ref)
    : task([this]() { return work_impl(); }),
      impl_ref(impl_ref) {
}

kvoice::sound_output_impl::create_stream_message::create_stream_message(sound_output_impl* impl_ref)
    : task([this]() { return work_impl(); }),
      impl_ref(impl_ref) {
}

kvoice::sound_output_impl::create_online_stream_message::create_online_stream_message(sound_output_impl* impl_ref,
    std::string url) :
    url(std::move(url)),
    task([this]() { return work_impl(); }),
    impl_ref(impl_ref) {
}

BOOL kvoice::sound_output_impl::set_device_volume_message::work_impl() const {
    return BASS_SetVolume(this->volume);
}

BOOL kvoice::sound_output_impl::set_device_message::work_impl() const {
    BASS_DEVICEINFO info;
    for (auto i = 1u; BASS_GetDeviceInfo(i, &info); i++) {
        if (info.flags & BASS_DEVICE_ENABLED) {
            if (device_name == info.name) {
                if (BASS_SetDevice(i)) {
                    return TRUE;
                } else {
                    BASS_SetDevice(-1);
                }
            }
        }
    }
    return BASS_SetDevice(-1);
}

BOOL kvoice::sound_output_impl::update_device_message::work_impl() const {
    std::lock_guard lock(impl_ref.spatial_mtx);

    auto vec_convert = [](kvoice::vector vec) {
        return BASS_3DVECTOR{ vec.x, vec.y, vec.z };
    };

    BASS_3DVECTOR pos = vec_convert(impl_ref.listener_pos);
    BASS_3DVECTOR vel = vec_convert(impl_ref.listener_vel);
    BASS_3DVECTOR front = vec_convert(impl_ref.listener_front);
    BASS_3DVECTOR up = vec_convert(impl_ref.listener_up);

    auto result = BASS_Set3DPosition(&pos, &vel, &front, &up);
    BASS_Apply3D();
    return result;
}

std::unique_ptr<kvoice::stream> kvoice::sound_output_impl::create_stream_message::work_impl() const {
    return std::make_unique<stream_impl>(impl_ref, impl_ref->sampling_rate);
}

std::unique_ptr<kvoice::stream> kvoice::sound_output_impl::create_online_stream_message::work_impl() const {
    return std::make_unique<stream_impl>(impl_ref, url, impl_ref->sampling_rate);
}

kvoice::sound_output_impl::sound_output_impl(std::string_view device_name, std::uint32_t sample_rate)
    : sampling_rate(sample_rate), output_thread_messages(16){
    using namespace std::string_literals;

    std::mutex              condvar_mtx{};
    std::condition_variable output_initialization{};
    output_alive = true;
    output_thread = std::thread([this, sample_rate, device_name, &output_initialization]() {
        int init_idx = -1;
        if (!device_name.empty()) {
            BASS_DEVICEINFO info;
            for (auto i = 1; BASS_GetDeviceInfo(i, &info); i++) {
                if (info.flags & BASS_DEVICE_ENABLED) {
                    if (device_name == info.name) {
                        init_idx = i;
                        break;
                    }
                }
            }

        }
        BASS_Init(init_idx, sample_rate, BASS_DEVICE_MONO | BASS_DEVICE_3D, nullptr, nullptr);

        output_initialization.notify_one();

        while (output_alive.load()) {
            if (!output_thread_messages.empty()) {
                auto message = output_thread_messages.front();
                message->do_work();

                output_thread_messages.pop();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        BASS_Free();
    });

    std::unique_lock lock{ condvar_mtx };
    output_initialization.wait(lock);
}

kvoice::sound_output_impl::~sound_output_impl() {
    output_alive = false;
    output_thread.join();
}

void kvoice::sound_output_impl::set_my_position(vector pos) noexcept {
    std::lock_guard lock{ spatial_mtx };
    listener_pos = pos;
}

void kvoice::sound_output_impl::set_my_velocity(vector vel) noexcept {
    std::lock_guard lock{ spatial_mtx };
    listener_vel = vel;
}

void kvoice::sound_output_impl::set_my_orientation_up(vector up) noexcept {
    std::lock_guard lock{ spatial_mtx };
    listener_up = up;
}

void kvoice::sound_output_impl::set_my_orientation_front(vector front) noexcept {
    std::lock_guard lock{ spatial_mtx };
    listener_front = front;
}

void kvoice::sound_output_impl::update_me() {
    update_device_message msg{ *this };
    output_thread_messages.push(&msg);
    auto result = msg.task.get_future().get();
    if (!result) {
        throw voice_exception::create_formatted("Couldn't update device");
    }
}

void kvoice::sound_output_impl::set_gain(float gain) noexcept {
    output_gain = gain;
    set_device_volume_message msg{ output_gain };
    output_thread_messages.push(&msg);
    msg.task.get_future().get();
}

void kvoice::sound_output_impl::change_device(std::string_view device_name) {
    set_device_message msg{ std::string{ device_name } };
    output_thread_messages.push(&msg);
    auto result = msg.task.get_future().get();
    if (!result) {
        throw voice_exception::create_formatted("Couldn't open device {}", device_name);
    }
}

std::unique_ptr<kvoice::stream> kvoice::sound_output_impl::create_stream() {
    create_stream_message msg{ this };
    output_thread_messages.push(&msg);
    return msg.task.get_future().get();
}

std::unique_ptr<kvoice::stream> kvoice::sound_output_impl::create_stream(std::string_view url) {
    create_online_stream_message msg{ this, std::string{ url } };
    output_thread_messages.push(&msg);
    return msg.task.get_future().get();
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include "sound_output_impl.hpp"

#include "stream_impl.hpp"
#include "voice_exception.hpp"

kvoice::sound_output_impl::sound_output_impl(std::string_view device_name, std::uint32_t sample_rate, std::uint32_t src_count) : sampling_rate(sample_rate) {
    using namespace std::string_literals;

    device = alcOpenDevice(device_name.data());  // NOLINT(cppcoreguidelines-prefer-member-initializer)

    if (!device) throw voice_exception::create_formatted("Couldn't open device {}", device_name);

    ctx = alcCreateContext(device, nullptr);

    if (!ctx || !alcMakeContextCurrent(ctx)) {
        if (ctx) {
            alcDestroyContext(ctx);
        }
        alcCloseDevice(device);
        throw voice_exception("Couldn't set context");
    }
    ALCint max_mono_sources;

    alcGetIntegerv(device, ALC_MONO_SOURCES, 1, &max_mono_sources);

    if (static_cast<ALCint>(src_count) > max_mono_sources) src_count = max_mono_sources;

    sources = new std::uint32_t[src_count];

    alGenSources(static_cast<ALCint>(src_count), sources);

    if (alGetError()) {
        throw voice_exception::create_formatted("Couldn't create {} sources", src_count);
    }
    this->src_count = src_count;

    for (auto i = 0u; i < src_count; ++i) {
        free_sources.push(sources[i]);
    }
}

kvoice::sound_output_impl::~sound_output_impl() {

    alDeleteSources(static_cast<ALCint>(src_count), sources);
    delete[] sources;

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}

void kvoice::sound_output_impl::set_my_position(vector pos) noexcept {
    listener_pos = pos;
}

void kvoice::sound_output_impl::set_my_velocity(vector vel) noexcept {
    listener_vel = vel;
}

void kvoice::sound_output_impl::set_my_orientation_up(vector up) noexcept {
    listener_up = up;
}

void kvoice::sound_output_impl::set_my_orientation_front(vector front) noexcept {
    listener_front = front;
}

void kvoice::sound_output_impl::update_me() {
    float orientation[]{
            listener_front.x, listener_front.y, listener_front.z,
            listener_up.x, listener_up.y, listener_up.z
    };

    alListenerfv(AL_POSITION, &listener_pos.x);
    alListenerfv(AL_VELOCITY, &listener_vel.x);
    alListenerfv(AL_ORIENTATION, orientation);
}

void kvoice::sound_output_impl::set_gain(float gain) noexcept {
    output_gain = gain;
}

void kvoice::sound_output_impl::change_device(std::string_view device_name) {
    drop_source_signal.emit();

    while (!free_sources.empty()) {
        free_sources.pop();
    }

    alDeleteSources(src_count, sources);
    delete[] sources;

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);

    device = alcOpenDevice(device_name.data());  // NOLINT(cppcoreguidelines-prefer-member-initializer)

    if (!device) throw voice_exception::create_formatted("Couldn't open device {}", device_name);

    ctx = alcCreateContext(device, nullptr);

    if (!ctx || !alcMakeContextCurrent(ctx)) {
        if (ctx) {
            alcDestroyContext(ctx);
        }
        alcCloseDevice(device);
        throw voice_exception("Couldn't set context");
    }
    ALCint max_mono_sources;

    alcGetIntegerv(device, ALC_MONO_SOURCES, 1, &max_mono_sources);

    if (static_cast<ALCint>(src_count) > max_mono_sources) src_count = max_mono_sources;

    sources = new std::uint32_t[src_count];

    alGenSources(static_cast<ALCint>(src_count), sources);

    if (alGetError()) {
        throw voice_exception::create_formatted("Couldn't create {} sources", src_count);
    }

    for (auto i = 0u; i < src_count; ++i) {
        free_sources.push(sources[i]);
    }
}

std::uint32_t kvoice::sound_output_impl::get_source() {
    if (free_sources.empty()) throw voice_exception("There isn't free sources");

    auto result = free_sources.front();
    free_sources.pop();
    return result;
}

void kvoice::sound_output_impl::free_source(std::uint32_t source) noexcept {
    free_sources.push(source);
}

void kvoice::sound_output_impl::set_buffering_time(std::uint32_t time_ms) {
    buffering_time = time_ms;
}

std::unique_ptr<kvoice::stream> kvoice::sound_output_impl::create_stream() {
    return std::make_unique<stream_impl>(this, sampling_rate);
}

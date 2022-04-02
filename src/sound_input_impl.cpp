#include "sound_input_impl.hpp"

#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>
#include <opus.h>

#include <algorithm>
#include <array>
#include <boost/circular_buffer.hpp>

#include "voice_exception.hpp"

kvoice::sound_input_impl::sound_input_impl(std::string_view device_name, std::int32_t        sample_rate,
                                           std::int32_t     frames_per_buffer, std::uint32_t bitrate)
    : sample_rate_(sample_rate),
      frames_per_buffer_(frames_per_buffer),
      input_device(alcCaptureOpenDevice(device_name.data(), sample_rate, AL_FORMAT_MONO_FLOAT32, frames_per_buffer)) {

    if (!input_device) throw voice_exception::create_formatted("Couldn't open capture device {}", device_name);

    sleep_time = std::chrono::milliseconds{ frames_per_buffer * 500 / sample_rate };

    int opus_err;
    encoder = opus_encoder_create(sample_rate, 1, OPUS_APPLICATION_VOIP, &opus_err);

    if (opus_err != OPUS_OK || !encoder)
        throw voice_exception::create_formatted("Couldn't create opus encoder (errc = {})", opus_err);

    if ((opus_err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate))) != OPUS_OK)
        throw voice_exception::create_formatted("Couldn't set encoder bitrate (errc = {})", opus_err);

    input_alive = true;
    input_thread = std::thread(&sound_input_impl::process_input, this);
}

kvoice::sound_input_impl::~sound_input_impl() {
    input_alive = false;
    input_thread.join();

    alcCaptureCloseDevice(input_device);
    opus_encoder_destroy(encoder);
}

bool kvoice::sound_input_impl::enable_input() {
    if (!input_active) {
        std::unique_lock lck(device_mutex);
        if (input_device) {
            input_active = true;
            alcCaptureStart(input_device);
            return true;
        }
        return false;
    }
    return false;
}


bool kvoice::sound_input_impl::disable_input() {
    if (input_active) {
        std::unique_lock lck(device_mutex);
        input_active = false;
        if (input_device)
            alcCaptureStart(input_device);
        return true;
    }
    return false;
}

void kvoice::sound_input_impl::set_mic_gain(float gain) {
    input_gain.store(gain);
}

void kvoice::sound_input_impl::change_device(std::string_view device_name) {
    std::unique_lock lck(device_mutex);

    alcCaptureCloseDevice(input_device);

    input_device = alcCaptureOpenDevice(device_name.data(), sample_rate_, AL_FORMAT_MONO_FLOAT32, frames_per_buffer_);

    if (!input_device) throw voice_exception::create_formatted("Couldn't open capture device {}", device_name);
}

void kvoice::sound_input_impl::set_input_callback(std::function<on_voice_input_t> cb) {
    on_voice_input = std::move(cb);
}

void kvoice::sound_input_impl::set_raw_input_callback(std::function<on_voice_raw_input> cb) {
    on_raw_voice_input = std::move(cb);
}

void kvoice::sound_input_impl::process_input() {
    using namespace std::chrono_literals;

    std::array<std::uint8_t, kPacketMaxSize> packet{};
    std::vector<float>                       capture_buffer(frames_per_buffer_);
    std::vector<float>                       temporary_buffer;
    temporary_buffer.reserve(kOpusFrameSize);

    std::int32_t captured_frames;
    bool         buffer_captured;

    while (input_alive) {
        buffer_captured = false;

        {
            std::unique_lock lck(device_mutex);
            if (!input_device) {
                std::this_thread::sleep_for(sleep_time);
                continue;
            }
            alcGetIntegerv(input_device, ALC_CAPTURE_SAMPLES, 1, &captured_frames);
            if (captured_frames >= frames_per_buffer_) {
                capture_buffer.resize(frames_per_buffer_);
                alcCaptureSamples(input_device, capture_buffer.data(), frames_per_buffer_);
                buffer_captured = true;
            }
        }

        if (buffer_captured) {
            float mic_level = *std::max_element(capture_buffer.begin(), capture_buffer.end());

            on_raw_voice_input(capture_buffer.data(), capture_buffer.size(), mic_level);

            std::transform(capture_buffer.begin(), capture_buffer.end(), capture_buffer.begin(),
                           [gain = input_gain.load()](const float v) { return v * gain; });

            std::ptrdiff_t needed_data = kOpusFrameSize - static_cast<std::ptrdiff_t>(temporary_buffer.size());

            // move all data to temp buffer by default
            auto end_it = capture_buffer.end();

            if (needed_data < static_cast<std::ptrdiff_t>(capture_buffer.size())) {
                // move only needed amount of data if needed data size < captured data size
                end_it = std::next(capture_buffer.begin(), needed_data);
            }

            // move
            temporary_buffer.insert(temporary_buffer.cend(), std::make_move_iterator(capture_buffer.begin()),
                                    std::make_move_iterator(end_it));
            capture_buffer.erase(capture_buffer.begin(), end_it);

            // if there enough data then pass it to the encoder
            if (temporary_buffer.size() == kOpusFrameSize) {
                // encode data and pass it to callback, then clear temporary data buffer
                int len = opus_encode_float(encoder, temporary_buffer.data(), kOpusFrameSize, packet.data(),
                                            kPacketMaxSize);
                if (len < 0 || len > kPacketMaxSize) return;
                on_voice_input(packet.data(), len);
                temporary_buffer.clear();
            }
            auto           remaining_data = capture_buffer.size();
            std::ptrdiff_t idx = 0;
            // process the remaining data if any and if its size more or equals to opus frame size
            while (remaining_data >= kOpusFrameSize) {
                int len = opus_encode_float(encoder, &capture_buffer[idx], kOpusFrameSize, packet.data(),
                                            kPacketMaxSize);
                if (len < 0 || len > kPacketMaxSize) return;
                on_voice_input(packet.data(), len);
                idx += kOpusFrameSize;
                remaining_data -= kOpusFrameSize;
            }
            // add remaining data to the temporary buffer if any
            if (remaining_data > 0) {
                temporary_buffer.insert(temporary_buffer.cend(),
                                        std::make_move_iterator(std::next(capture_buffer.begin(), idx)),
                                        std::make_move_iterator(capture_buffer.end()));
                capture_buffer.clear();
            }

        }

        std::this_thread::sleep_for(sleep_time);
    }
}

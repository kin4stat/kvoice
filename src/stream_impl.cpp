#include "stream_impl.hpp"

#include "voice_exception.hpp"
#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>
#include <opus.h>

kvoice::stream_impl::stream_impl(sound_output_impl* output, std::int32_t sample_rate)
    : sample_rate(sample_rate),
      output_impl(output),
      signal_connection(output->drop_source_signal.scoped_connect([this]() { if (has_source) drop_source(); })) {
    alGenBuffers(kBuffersCount, buffers.data());

    for (auto buffer : buffers) {
        free_buffers.push(buffer);
    }

    ALenum errc;
    if ((errc = alGetError()) != AL_NO_ERROR)
        throw voice_exception::create_formatted(
            "Failed to create al buffers (errc = {})", errc);

    int opus_err;
    decoder = opus_decoder_create(sample_rate, 1, &opus_err);

    if (opus_err != OPUS_OK || !decoder)
        throw voice_exception::create_formatted(
            "Failed to opus decoder (errc = {})", opus_err);
}

kvoice::stream_impl::~stream_impl() {
    if (has_source)
        output_impl->free_source(source);
    alDeleteBuffers(kBuffersCount, buffers.data());
}

bool kvoice::stream_impl::push_opus_buffer(const void* data, std::size_t count) {
    std::array<float, kOpusBufferSize> out{};

    const int frame_size = opus_decode_float(decoder, reinterpret_cast<const unsigned char*>(data),
                                             static_cast<int>(count), out.data(),
                                             kOpusBufferSize, 0);
    if (frame_size < 0) return false;

    float final_gain = extra_gain * output_impl->get_gain();
    if (final_gain != 1.f) {
        std::transform(out.begin(), out.begin() + frame_size, out.begin(),
                       [final_gain](float v) { return v * final_gain; });
    }

    ring_buffer.writeBuff(out.data(), frame_size);
    return true;
}

void kvoice::stream_impl::set_position(vector pos) {
    position = pos;

    if (has_source && is_spatial)
        alSourcefv(source, AL_POSITION, &position.x);
}

void kvoice::stream_impl::set_velocity(vector vel) {
    velocity = vel;

    if (has_source && is_spatial)
        alSourcefv(source, AL_VELOCITY, &velocity.x);
}

void kvoice::stream_impl::set_direction(vector dir) {
    direction = dir;

    if (has_source && is_spatial)
        alSourcefv(source, AL_DIRECTION, &direction.x);
}

void kvoice::stream_impl::set_min_distance(float distance) {
    min_distance = distance;

    if (has_source && is_spatial)
        alSourcef(source, AL_REFERENCE_DISTANCE, min_distance);
}

void kvoice::stream_impl::set_max_distance(float distance) {
    max_distance = distance;

    if (has_source && is_spatial)
        alSourcef(source, AL_MAX_DISTANCE, max_distance);
}

void kvoice::stream_impl::set_rolloff_factor(float rolloff) {
    rollof_factor = rolloff;
    if (has_source && is_spatial)
        alSourcef(source, AL_ROLLOFF_FACTOR, rollof_factor);
}

void kvoice::stream_impl::set_spatial_state(bool spatial_state) {
    if (this->is_spatial == spatial_state) return;
    if (!has_source) return;

    this->is_spatial = spatial_state;
    setup_spatial();
}

void kvoice::stream_impl::set_gain(float gain) {
    output_gain = gain;
}

bool kvoice::stream_impl::is_playing() {
    return playing;
}

bool kvoice::stream_impl::update() {
    if (!has_source) {
        if (ring_buffer.isEmpty())
            return true;

        try {
            source = output_impl->get_source();
        } catch (voice_exception&) {
            return false;
        }

        has_source = true;
        last_source_request_time = std::chrono::steady_clock::now();

        source_used_once = false;

        try {
            update_source(source);
        } catch (voice_exception&) {
            drop_source();
            return false;
        }
        playing = false;
    }

    std::int32_t state, processed;

    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (alGetError() != AL_NO_ERROR)
        return false;

    playing = state == AL_PLAYING;

    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    if (alGetError() != AL_NO_ERROR) {
        drop_source();
        return false;
    }

    if (ring_buffer.isEmpty() && !playing && source_used_once) {
        while (processed > 0) {
            ALuint bufid;
            alSourceUnqueueBuffers(source, 1, &bufid);
            free_buffers.push(bufid);
            processed--;
        }

        drop_source();
        return true;
    }

    if (ring_buffer.readAvailable() >= kRingBufferSize / 2)
        alSourcef(source, AL_PITCH, 1.05f);
    else
        alSourcef(source, AL_PITCH, 1.f);

    if (alGetError() != AL_NO_ERROR) {
        drop_source();
        return false;
    }

    while (processed > 0) {
        ALuint bufid;
        alSourceUnqueueBuffers(source, 1, &bufid);
        free_buffers.push(bufid);
        processed--;
    }

    while (!ring_buffer.isEmpty() && !free_buffers.empty()) {
        std::array<float, 4096> temp_buffer{};
        const std::uint32_t     buffer_id = free_buffers.front();
        free_buffers.pop();

        if (const std::size_t readed = ring_buffer.readBuff(temp_buffer.data(), temp_buffer.size()); readed > 0) {
            alBufferData(buffer_id, AL_FORMAT_MONO_FLOAT32, temp_buffer.data(),
                         static_cast<int>(readed * sizeof(float)), sample_rate);
            if (alGetError() != AL_NO_ERROR) {
                drop_source();
                return false;
            }

            alSourceQueueBuffers(source, 1, &buffer_id);
            if (alGetError() != AL_NO_ERROR) {
                drop_source();
                return false;
            }
        } else
            break;
    }

    if (!playing) {
        auto ctime = std::chrono::steady_clock::now();
        auto time_from_first_buffer = std::chrono::duration_cast<std::chrono::milliseconds>(
            ctime - last_source_request_time).count();
        if (time_from_first_buffer > output_impl->get_buffering_time()) {
            alSourcePlay(source);
            source_used_once = true;
            if (alGetError() != AL_NO_ERROR) {
                drop_source();
                return false;
            }
        }
    }
    return true;
}

void kvoice::stream_impl::setup_spatial() const {
    if (this->is_spatial) {
        vector zeros{ 0.f, 0.f, 0.f };

        alSourcefv(source, AL_POSITION, &zeros.x);
        alSourcefv(source, AL_VELOCITY, &zeros.x);
        alSourcefv(source, AL_DIRECTION, &zeros.x);
        alSourcef(source, AL_MAX_DISTANCE, 100.f);
        alSourcef(source, AL_REFERENCE_DISTANCE, 100.f);
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.f);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    } else {
        alSourcefv(source, AL_POSITION, &position.x);
        alSourcefv(source, AL_VELOCITY, &velocity.x);
        alSourcefv(source, AL_DIRECTION, &direction.x);
        alSourcef(source, AL_MAX_DISTANCE, max_distance);
        alSourcef(source, AL_REFERENCE_DISTANCE, min_distance);
        alSourcef(source, AL_ROLLOFF_FACTOR, rollof_factor);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
    }
}

void kvoice::stream_impl::update_source(std::uint32_t source_handle) const {
    alSourceRewind(source_handle);

    alSourcei(source_handle, AL_LOOPING, false);
    alSourcei(source_handle, AL_BUFFER, 0);

    setup_spatial();

    ALenum errc;
    if ((errc = alGetError()) != AL_NO_ERROR)
        throw voice_exception::create_formatted(
            "failed to update source (last errc = {})", errc);
}

void kvoice::stream_impl::drop_source() {
    if (has_source) {
        alSourceStop(source);
        output_impl->free_source(source);

        has_source = false;
    }
}

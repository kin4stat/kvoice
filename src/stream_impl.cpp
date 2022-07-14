#include "stream_impl.hpp"

#include "voice_exception.hpp"
#include <opus.h>

kvoice::stream_impl::stream_impl(sound_output_impl* output, std::string_view url, std::uint32_t file_offset, std::int32_t sample_rate)
    : sample_rate(sample_rate),
      output_impl(output) {
    stream_handle = BASS_StreamCreateURL(url.data(), 0, BASS_SAMPLE_MONO | BASS_SAMPLE_3D, nullptr, nullptr);

    auto err = BASS_ErrorGetCode();

    BASS_ChannelSetSync(stream_handle, BASS_SYNC_END, 0, [](HSYNC handle, DWORD channel, DWORD data, void *user) {
        auto ptr = reinterpret_cast<stream_impl*>(user);
        if (ptr->on_end_cb) {
            ptr->on_end_cb();
        }
    }, this);

    
    type = stream_type::kOnlineDataStream;

    decoder = nullptr;

    file_offset = BASS_ChannelSeconds2Bytes(stream_handle, file_offset);
    BASS_ChannelPlay(stream_handle, false);
    BASS_ChannelSetPosition(stream_handle, file_offset, BASS_POS_BYTE | BASS_POS_DECODETO);
    BASS_ChannelSetPosition(stream_handle, file_offset, BASS_POS_BYTE);
}

kvoice::stream_impl::stream_impl(sound_output_impl* output, std::int32_t sample_rate)
    : sample_rate(sample_rate),
      output_impl(output) {
    stream_handle = BASS_StreamCreate(sample_rate, 1, BASS_SAMPLE_FLOAT, &bass_cb, this);
    type = stream_type::kLocalDataStream;
    int opus_err;
    decoder = opus_decoder_create(sample_rate, 1, &opus_err);

    if (opus_err != OPUS_OK || !decoder)
        throw voice_exception::create_formatted(
            "Failed to opus decoder (errc = {})", opus_err);

    BASS_ChannelSetAttribute(stream_handle, BASS_ATTRIB_GRANULE, 960);
    BASS_ChannelPlay(stream_handle, false);
}

kvoice::stream_impl::~stream_impl() {
    BASS_StreamFree(stream_handle);
}

bool kvoice::stream_impl::push_opus_buffer(const void* data, std::size_t count) {
    if (type != stream_type::kLocalDataStream) return false;
    std::array<float, kOpusBufferSize> out{};

    const int frame_size = opus_decode_float(decoder, reinterpret_cast<const unsigned char*>(data),
                                             static_cast<int>(count), out.data(),
                                             kOpusBufferSize, 0);
    if (frame_size < 0) return false;

    float final_gain = extra_gain;
    if (final_gain != 1.f) {
        std::transform(out.begin(), out.begin() + frame_size, out.begin(),
                       [final_gain](float v) { return v * final_gain; });
    }

    ring_buffer.writeBuff(out.data(), frame_size);
    return true;
}

void kvoice::stream_impl::set_position(vector pos) {
    position = pos;
}

void kvoice::stream_impl::set_velocity(vector vel) {
    velocity = vel;
}

void kvoice::stream_impl::set_direction(vector dir) {
    direction = dir;
}

void kvoice::stream_impl::set_min_distance(float distance) {
    min_distance = distance;
}

void kvoice::stream_impl::set_max_distance(float distance) {
    max_distance = distance;
}

void kvoice::stream_impl::set_rolloff_factor(float rolloff) {
    rollof_factor = rolloff;
}

void kvoice::stream_impl::set_spatial_state(bool spatial_state) {
    if (this->is_spatial == spatial_state) return;

    this->is_spatial = spatial_state;
    setup_spatial();
}

void kvoice::stream_impl::set_gain(float gain) {
    output_gain = gain;
    BASS_ChannelSetAttribute(stream_handle, BASS_ATTRIB_VOL, gain);
}

bool kvoice::stream_impl::is_playing() {
    return playing;
}

void kvoice::stream_impl::update() {
    setup_spatial();
}

void kvoice::stream_impl::on_end_stream_cb(kvoice::on_stream_end_cb cb) {
    on_end_cb = std::move(cb);
}

void kvoice::stream_impl::set_url(std::string_view url) {
    BASS_StreamFree(stream_handle);

    stream_handle = BASS_StreamCreateURL(url.data(), 0, BASS_SAMPLE_MONO | BASS_SAMPLE_3D | BASS_SAMPLE_FLOAT, nullptr,
                                         nullptr);

    BASS_ChannelSetSync(stream_handle, BASS_SYNC_END, 0, [](HSYNC handle, DWORD channel, DWORD data, void *user) {
        reinterpret_cast<stream_impl*>(user)->on_end_cb();
    }, this);

    BASS_ChannelPlay(stream_handle, false);
}

void kvoice::stream_impl::continue_playing() {
    BASS_ChannelPlay(stream_handle, false);
}

void kvoice::stream_impl::pause_playing() {
    BASS_ChannelPause(stream_handle);
}

void kvoice::stream_impl::mute_stream() {
    BASS_ChannelSetAttribute(stream_handle, BASS_ATTRIB_VOL, 0.0f);
    BASS_ChannelPlay(stream_handle, false);
}

void kvoice::stream_impl::unmute_stream() {
    BASS_ChannelSetAttribute(stream_handle, BASS_ATTRIB_VOL, 1.0f);
}

DWORD kvoice::stream_impl::process_output(void* buffer, DWORD length) {
    if (ring_buffer.isEmpty())
        return 0;
    if (auto length_in_floats = length / sizeof(float); ring_buffer.readAvailable() >= length_in_floats) {
        ring_buffer.readBuff(static_cast<float*>(buffer), length_in_floats);
        return length;
    } else {
        auto max_bytes = ring_buffer.readAvailable() * sizeof(float);
        ring_buffer.readBuff(static_cast<float*>(buffer), ring_buffer.readAvailable());
        return max_bytes;
    }
}

void kvoice::stream_impl::setup_spatial() const {
    if (!this->is_spatial) {
        BASS_ChannelSet3DAttributes(stream_handle, BASS_3DMODE_OFF, 0, 0, -1, -1, 0);
    } else {

        auto vec_convert = [](kvoice::vector vec) {
            return BASS_3DVECTOR{ vec.x, vec.y, vec.z };
        };
        BASS_3DVECTOR pos = vec_convert(position);
        BASS_3DVECTOR vel = vec_convert(velocity);
        BASS_3DVECTOR dir = vec_convert(direction);

        BASS_ChannelSet3DAttributes(stream_handle, BASS_3DMODE_NORMAL, min_distance, max_distance, -1, -1, -1);
        BASS_ChannelSet3DPosition(stream_handle, &pos, &dir, &vel);
    }
    BASS_Apply3D();
}

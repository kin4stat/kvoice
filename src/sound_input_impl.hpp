#pragma once

#include <cstdint>
#include <mutex>
#include <atomic>

#include "ringbuffer.hpp"
#include "bass/bass.h"
#include "sound_input.hpp"

struct OpusEncoder;

namespace kvoice {
constexpr auto kOpusFrameSize = 960;
constexpr auto kPacketMaxSize = 32768;

class sound_input_impl final : public sound_input {
public:
    sound_input_impl(std::string_view device_name, std::int32_t sample_rate, std::int32_t frames_per_buffer, std::uint32_t bitrate);
    ~sound_input_impl() override;
    bool enable_input() override;
    bool disable_input() override;
    void set_mic_gain(float gain) override;
    void change_device(std::string_view device_name) override;
    void set_input_callback(std::function<on_voice_input_t> cb) override;
    void set_raw_input_callback(std::function<on_voice_raw_input> cb) override;
private:
    BOOL process_input(HRECORD handle,const void* buffer, DWORD length);

    static BOOL __stdcall bass_cb(HRECORD handle, const void* buffer, DWORD length, void* user);


    std::int32_t sample_rate_{ 48000 };
    std::int32_t frames_per_buffer_{ 420 };

    OpusEncoder* encoder{ nullptr };

    HRECORD record_handle{ false };

    std::function<on_voice_input_t>   on_voice_input{};
    std::function<on_voice_raw_input> on_raw_voice_input{};

    bool input_active{ false };

    std::vector<float>                    encoder_buffer;
    jnk0le::Ringbuffer<float, 8192, true> temporary_buffer{};
};
}

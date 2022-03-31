#pragma once

#include <cstdint>
#include <mutex>
#include <atomic>

#include "sound_input.hpp"

struct OpusEncoder;
struct ALCdevice;

namespace kvoice {
constexpr auto kOpusFrameSize = 480;
constexpr auto kPacketMaxSize = 32768;

class sound_input_impl final : public sound_input {
public:
    sound_input_impl(std::string_view device_name, std::uint32_t sample_rate, std::uint32_t frames_per_buffer,
                     std::uint32_t    bitrate);
    ~sound_input_impl() override;
    bool enable_input() override;
    bool disable_input() override;
    void set_mic_gain(float gain) override;
    void change_device(std::string_view device_name) override;
    void set_input_callback(std::function<on_voice_input_t> cb) override;
    void set_raw_input_callback(std::function<on_voice_raw_input> cb) override;
private:
    void process_input();

    std::atomic<float>        input_gain{ 1.f };
    std::uint32_t             sample_rate_{ 48000 };
    std::uint32_t             frames_per_buffer_{ 420 };
    std::uint32_t             bitrate_{ 16384 };
    std::chrono::milliseconds sleep_time{ 1000 };

    OpusEncoder* encoder{ nullptr };

    ALCdevice* input_device{ nullptr };

    std::mutex  device_mutex;
    std::thread input_thread;

    std::function<on_voice_input_t>   on_voice_input{};
    std::function<on_voice_raw_input> on_raw_voice_input{};

    bool input_active{ false };
    bool input_alive{ false };
};
}

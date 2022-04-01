#pragma once

#include <cstdint>
#include <array>
#include <chrono>
#include <queue>

#include "ringbuffer.hpp"
#include "sound_output_impl.hpp"
#include "kv_vector.hpp"
#include "stream.hpp"

struct OpusDecoder;

namespace kvoice {
class stream_impl final : public stream {
    static void _foo();
    using sconnection_t = decltype(sound_output_impl::drop_source_signal.scoped_connect(&_foo));

    static constexpr auto kBuffersCount = 16;
    static constexpr auto kMinBuffersCount = 8;
    static constexpr auto kRingBufferSize = 262144;
    static constexpr auto kOpusBufferSize = 8196;
public:
    stream_impl(sound_output_impl* output, std::uint32_t sample_rate);
    ~stream_impl() override;

    bool push_opus_buffer(const void* data, std::size_t count) override;

    void set_position(vector pos) override;
    void set_velocity(vector vel) override;
    void set_direction(vector dir) override;
    void set_min_distance(float distance) override;
    void set_max_distance(float distance) override;
    void set_rolloff_factor(float rolloff) override;
    void set_spatial_state(bool is_spatial) override;
    void set_gain(float gain) override;

    bool is_playing() override;

    bool update() override;

private:
    void setup_spatial() const;
    void update_source(std::uint32_t source) const;
    void drop_source();

    std::array<std::uint32_t, kBuffersCount> buffers{};
    std::queue<std::uint32_t>                free_buffers{};
    std::uint32_t                            source{ 0 };
    std::uint32_t                            buffers_filled{ 0 };
    std::chrono::steady_clock::time_point    last_source_request_time{};
    std::uint32_t                            sample_rate{ 0 };

    vector position{};
    vector velocity{};
    vector direction{};

    float output_pitch{ 1.f };
    float output_gain{ 1.f };
    float min_distance{ 0.f };
    float max_distance{ 100.f };
    float rollof_factor{ 1.f };
    float extra_gain{ 1.f };

    OpusDecoder*       decoder{ nullptr };
    sound_output_impl* output_impl{ nullptr };

    sconnection_t signal_connection;

    bool playing{ false };
    bool has_source{ false };
    bool source_used_once{ false };
    bool is_spatial{ true };

    jnk0le::Ringbuffer<float, kRingBufferSize, true> ring_buffer{};
};
}

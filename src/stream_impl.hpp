#pragma once

#include <cstdint>
#include <array>
#include <chrono>
#include <queue>

#include "bass/bass.h"
#include "ringbuffer.hpp"
#include "sound_output_impl.hpp"
#include "kv_vector.hpp"
#include "stream.hpp"

struct OpusDecoder;

namespace kvoice {
class stream_impl final : public stream {
    enum class stream_type {
        kOnlineDataStream,
        kLocalDataStream,
    };

    static constexpr auto kBuffersCount = 16;
    static constexpr auto kMinBuffersCount = 8;
    static constexpr auto kRingBufferSize = 262144;
    static constexpr auto kOpusBufferSize = 8196;
public:
    stream_impl(sound_output_impl* output, std::string_view url, std::int32_t sample_rate);
    stream_impl(sound_output_impl* output, std::int32_t sample_rate);
    ~stream_impl() override;

    bool push_opus_buffer(const void* data, std::size_t count) override;

    void set_position(vector pos) override;
    void set_velocity(vector vel) override;
    void set_direction(vector dir) override;
    void set_min_distance(float distance) override;
    void set_max_distance(float distance) override;
    void set_rolloff_factor(float rolloff) override;
    void set_spatial_state(bool spatial_state) override;
    void set_gain(float gain) override;

    bool is_playing() override;

    void update() override;

    void on_end_stream_cb(kvoice::on_stream_end_cb) override;
    void set_url(std::string_view url) override;
private:
    DWORD process_output(void *buffer, DWORD length);

    static DWORD __stdcall bass_cb(HSTREAM handle, void *buffer, DWORD length, void *user) {
        return static_cast<stream_impl*>(user)->process_output(buffer, length);
    }

    void setup_spatial() const;

    HSTREAM stream_handle;

    stream_type type;

    std::chrono::steady_clock::time_point    last_source_request_time{};
    std::int32_t                             sample_rate{ 0 };

    vector position{};
    vector velocity{};
    vector direction{};

    float output_gain{ 1.f };
    float min_distance{ 0.f };
    float max_distance{ 100.f };
    float rollof_factor{ 1.f };
    float extra_gain{ 1.f };

    OpusDecoder*       decoder{ nullptr };
    sound_output_impl* output_impl{ nullptr };

    bool playing{ false };
    bool is_spatial{ true };

    kvoice::on_stream_end_cb on_end_cb;

    jnk0le::Ringbuffer<float, kRingBufferSize, true> ring_buffer{};
};
}

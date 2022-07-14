#pragma once
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>

#include "bass/bass.h"
#include <variant>
#include "sound_output.hpp"
#include "ktsignal/ktsignal.hpp"

namespace kvoice {
class sound_output_impl : public sound_output {

    struct online_stream_parameters {
        std::string url;
        std::uint32_t file_offset;
    };

    struct request_stream_message {
        std::optional<online_stream_parameters> params;
        on_create_callback on_creation_callback;

        request_stream_message(std::optional<online_stream_parameters> params, on_create_callback on_creation_callback)
            : params(std::move(params)),
              on_creation_callback(std::move(on_creation_callback)) {}

        request_stream_message() = default;
    };

public:
    /**
     * @brief Constructor
     * @param device_name Output device name in UTF-8(empty for default)
     * @param sample_rate Output device sampling rate
     * @param src_count Number of max sources
     */
    sound_output_impl(std::string_view device_name, std::uint32_t sample_rate);
    ~sound_output_impl() override;

    /**
     * @brief Sets local position(changes should be applied manually)
     * @param pos Local position
     * @ref update_me
     */
    void set_my_position(vector pos) noexcept override;
    /**
     * @brief Sets local velocity(changes should be applied manually)
     * @param vel Local velocity
     * @ref update_me
     */
    void set_my_velocity(vector vel) noexcept override;
    /**
     * @brief Sets local orientation up(changes should be applied manually)
     * @param up Local orientation
     * @ref update_me
     */
    void set_my_orientation_up(vector up) noexcept override;
    /**
     * @brief Sets local orientation front(changes should be applied manually)
     * @param front Local orientation
     * @ref update_me
     */
    void set_my_orientation_front(vector front) noexcept override;

    /**
     * @brief Updates the local player with previous data
    */
    void update_me() override{};

    /**
     * @brief Sets output gain
     * @param gain Output gain, from 0 to 1
    */
    void set_gain(float gain) noexcept override;

    /**
     * @brief changes output device
     * @param device_name name of new output device
     */
    void change_device(std::string_view device_name) override;

    void                        set_buffering_time(std::uint32_t time_ms) override { buffering_time = time_ms; }
    [[nodiscard]] std::uint32_t get_buffering_time() const { return buffering_time; }
    [[nodiscard]] float         get_gain() const { return output_gain.load(); }
    void     create_stream(on_create_callback cb) override;
    void     create_stream(on_create_callback cb, std::string_view url, std::uint32_t file_offset) override;

    ktsignal::ktsignal<void()> drop_source_signal;
private:
    boost::lockfree::spsc_queue<request_stream_message> requests_queue;
    std::atomic_bool                                   output_alive{ false };
    std::thread                                        output_thread{};

    std::mutex spatial_mtx{};

    vector     listener_pos{ 0.f, 0.f, 0.f };
    vector     listener_vel{ 0.f, 0.f, 0.f };
    vector     listener_front{ 0.f, 0.f, 0.f };
    vector     listener_up{ 0.f, 0.f, 0.f };

    std::atomic_bool device_need_update;
    std::string device_name;

    std::atomic<float> output_gain{ 1.f };

    std::uint32_t sampling_rate{ 0 };
    std::uint32_t buffering_time{ 0 };
};
} // namespace kvoice

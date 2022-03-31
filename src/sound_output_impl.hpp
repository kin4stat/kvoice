#pragma once
#include <queue>

#include "sound_output.hpp"
#include "ktsignal/ktsignal.hpp"

struct ALCdevice;
struct ALCcontext;

namespace kvoice {
class sound_output_impl : public sound_output {
public:
    /**
     * @brief Constructor
     * @param device_name Output device name in UTF-8(empty for default)
     * @param sample_rate Output device sampling rate
     * @param src_count Number of max sources
     */
    sound_output_impl(std::string_view device_name, std::uint32_t sample_rate, std::uint32_t src_count);
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
    void update_me() override;

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

    std::uint32_t get_source();
    void          free_source(std::uint32_t source) noexcept;

    void set_buffering_time(std::uint32_t time_ms) override;

    [[nodiscard]] float get_gain() const { return output_gain; }

    [[nodiscard]] std::uint32_t get_buffering_time() const { return buffering_time; }
    std::unique_ptr<stream>     create_stream() override;

    ktsignal::ktsignal<void()> drop_source_signal;
private:

    vector listener_pos{ 0.f, 0.f, 0.f };
    vector listener_vel{ 0.f, 0.f, 0.f };
    vector listener_front{ 0.f, 0.f, 0.f };
    vector listener_up{ 0.f, 0.f, 0.f };

    float output_gain{ 1.f };

    std::uint32_t* sources;
    std::uint32_t  src_count;
    std::uint32_t  buffering_time;
    std::uint32_t  sampling_rate;

    std::queue<std::uint32_t> free_sources;

    ALCdevice*  device;
    ALCcontext* ctx;
};
} // namespace kvoice

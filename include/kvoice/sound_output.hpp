#pragma once
#include "kv_vector.hpp"
#include <string_view>
#include <memory>
#include "stream.hpp"

namespace kvoice {
class sound_output {
public:
    /**
     * @brief destructor
     */
    virtual ~sound_output() = default;

    /**
     * @brief Sets local position(changes should be applied manually)
     * @param pos Local position
     * @ref update_me
     */
    virtual void set_my_position(vector pos) = 0;
    /**
     * @brief Sets local velocity(changes should be applied manually)
     * @param vel Local velocity
     * @ref update_me
     */
    virtual void set_my_velocity(vector vel) = 0;
    /**
     * @brief Sets local orientation up(changes should be applied manually)
     * @param up Local orientation
     * @ref update_me
     */
    virtual void set_my_orientation_up(vector up) = 0;
    /**
     * @brief Sets local orientation front(changes should be applied manually)
     * @param front Local orientation
     * @ref update_me
     */
    virtual void set_my_orientation_front(vector front) = 0;

    /**
     * @brief Updates the local player with previous data
    */
    virtual void update_me() = 0;

    /**
     * @brief Sets output gain
     * @param gain Output gain, from 0 to 1
    */
    virtual void set_gain(float gain) = 0;

    /**
     * @brief changes output device
     * @param device_name name of new output device
     * @throws voice_exception if device couldn't be open
     */
    virtual void change_device(std::string_view device_name) = 0;

    /**
     * @brief sets output buffering time
     * @param time_ms time in ms
     */
    virtual void set_buffering_time(std::uint32_t time_ms) = 0;

    /**
     * @brief creates new stream on output
     * @return pointer to stream
     */
    virtual std::unique_ptr<stream> create_stream() = 0;
    /**
     * @brief creates new stream on output
     * @return pointer to stream
     */
    virtual std::unique_ptr<stream> create_stream(std::string_view url) = 0;
};
}

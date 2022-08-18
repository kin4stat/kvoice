#pragma once

#include <cstddef>
#include <functional>
#include <string_view>
#include "kv_vector.hpp"

namespace kvoice {
using on_stream_end_cb = std::function<void()>;

class stream {
public:
    /**
     * @brief destructor
     */
    virtual ~stream() = default;

    /**
     * @brief pushes buffer with data to encoder and then to the sound output
     * @param data buffer with opus encoded data
     * @param count size of @p buffer
     * @return true on success, false on fail
     */
    virtual bool push_buffer(const void* data, std::size_t count) = 0;

    /**
     * @brief pushes buffer with data to encoder and then to the sound output
     * @param data buffer with opus encoded data
     * @param count size of @p buffer
     * @return true on success, false on fail
     */
    virtual bool push_opus_buffer(const void* data, std::size_t count) = 0;

    /**
     * @brief sets source position
     * @param pos new source position
     */
    virtual void set_position(vector pos) = 0;
    /**
     * @brief sets source velocity
     * @param vel new source velocity
     */
    virtual void set_velocity(vector vel) = 0;
    /**
     * @brief sets source direction
     * @param dir new source direction
     */
    virtual void set_direction(vector dir) = 0;

    /**
     * @brief sets min distance
     * @param distance new source min distance
     */
    virtual void set_min_distance(float distance) = 0;
    /**
     * @brief sets max distance
     * @param distance new source max distance
     */
    virtual void set_max_distance(float distance) = 0;
    /**
     * @brief sets new rolloff factor
     * @param rolloff new source rollof factor
     */
    virtual void set_rolloff_factor(float rolloff) = 0;
    /**
     * @brief sets source spatial state
     * @param spatial_state true if spatial 
     */
    virtual void set_spatial_state(bool spatial_state) = 0;
    /**
     * @brief sets output gain
     * @param gain new output gain
     */
    virtual void set_gain(float gain) = 0;

    /**
     * @brief is playing sound now
     * @return is playing
     */
    virtual bool is_playing() = 0;

    /**
     * @brief updates internal info(like openal buffers), pushes new data to output
     */
    virtual void update() = 0;

    /**
     * @brief 
     * @param granularity 
     */
    virtual void set_granularity(std::uint32_t granularity) = 0;

    /**
     * @brief sets callback, that will be called on end of stream. Available only when stream created for online playing
     * @param cb on end callback
     */
    virtual void on_end_stream_cb(on_stream_end_cb cb) = 0;

    /**
     * @brief sets url of stream. Available only when stream created for online playing
     * @param url new url
     */
    virtual void set_url(std::string_view url) = 0;

    /**
     * @brief continue playing for paused stream. Works only for online streams
     */
    virtual void continue_playing() = 0;

    /**
     * @brief pauses playing. Works only for online streams
     */
    virtual void pause_playing() = 0;

    /**
     * @brief mutes streams
     */
    virtual void mute_stream() = 0;

    /**
     * @brief unmutes streams
     */
    virtual void unmute_stream() = 0;
};
}

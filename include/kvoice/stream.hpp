#pragma once

#include <cstddef>
#include "kv_vector.hpp"

namespace kvoice {
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
     * @return true on success, false on fail
     */
    virtual bool update() = 0;
};
}

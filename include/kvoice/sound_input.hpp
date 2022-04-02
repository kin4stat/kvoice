#pragma once
#include <functional>

namespace kvoice {
/**
 * @brief type of user defined callback that being called after processing
 * @param buffer buffer with data
 * @param size size of @p buffer
 */
using on_voice_input_t = void(const void* buffer, std::size_t size);
/**
 * @brief type of user defined callback that being called before processing
 * @param buffer buffer with raw data
 * @param size size of @p buffer
 * @param mic_level max input volume
 */
using on_voice_raw_input = void(const void* buffer, std::size_t size, float mic_level);

class sound_input {
public:
    /**
     * @brief destructor
     */
    virtual ~sound_input() = default;

    /**
     * @brief enables input
     * @return true on success, false on fail
     */
    virtual bool enable_input() = 0;
    /**
     * @brief disables input
     * @return true on success, false on fail
     */
    virtual bool disable_input() = 0;
    /**
     * @brief sets input gain
     * @param gain float value from 0.0 to 1.0
     */
    virtual void set_mic_gain(float gain) = 0;
    /**
     * @brief changes input device immediately
     * @param device_name new device name
     * @throws voice_exception if device couldn't be open
     */
    virtual void change_device(std::string_view device_name) = 0;
    /**
     * @brief sets input callback(called after applying gain and noise suppression)
     * @param cb user callback
     */
    virtual void set_input_callback(std::function<on_voice_input_t> cb) = 0;
    /**
     * @brief sets raw input callback(called before processing)
     * @param cb user callback
     */
    virtual void set_raw_input_callback(std::function<on_voice_raw_input> cb) = 0;
};
}

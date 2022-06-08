#pragma once

#include "sound_input.hpp"
#include "sound_output.hpp"

#include <vector>
#include <string>

#ifdef _WIN32
#   ifdef KVOICE_STATIC
#       define KVOICE_API
#   else
#       ifdef EXPORT_KVOICE_API
#           define KVOICE_API __declspec(dllexport)
#       else
#           define KVOICE_API __declspec(dllimport)
#       endif
#   endif
#else
#   define KVOICE_API
#endif

namespace kvoice {
/**
 * @brief for internal usage
 * @details constructs a new view to string at current enumerator pos and increments enumerator by its size
 * useful for splitting OpenAL device list into strings
 * @param enumerator strings split with two \0
 * @return view to a string at current enumerator pos
 */
inline std::string_view get_next_str(const char*& enumerator) {
    if (enumerator && *enumerator != '\0') {
        std::string_view res{ enumerator };
        enumerator += res.size() + 1;
        return res;
    }
    return "";
}

/**
 * @brief transforms internal OpenAL device list, and returns it
 * @return list of OpenAL input devices
 */
KVOICE_API std::vector<std::string> get_input_devices();
/**
 * @brief transforms internal OpenAL device list, and returns it
 * @return list of OpenAL output devices
 */
KVOICE_API std::vector<std::string> get_output_devices();

/**
 * @brief creates OpenAL sound output device
 * @param device_name name of output device
 * @param sample_rate output device sampling rate
 * @param src_count count of max sound sources
 * @return pointer to sound device if successful, else error message string
 */
KVOICE_API std::unique_ptr<sound_output> create_sound_output(std::string_view device_name,
                                                               std::uint32_t    sample_rate);
/**
 * @brief creates OpenAL sound input device
 * @param device_name name of input device
 * @param sample_rate input device sampling rate
 * @param frames_per_buffer count of frames captured every tick
 * @param bitrate input device bitrate
 * @return pointer to sound device if successful, else error message string
 */
KVOICE_API std::unique_ptr<sound_input> create_sound_input(std::string_view device_name,
                                                             std::uint32_t    sample_rate,
                                                             std::uint32_t    frames_per_buffer,
                                                             std::uint32_t    bitrate);
}

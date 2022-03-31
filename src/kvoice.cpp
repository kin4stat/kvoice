#include "kvoice.hpp"

#include <al.h>
#include <alc.h>

#include "voice_exception.hpp"
#include "sound_output_impl.hpp"
#include "sound_input_impl.hpp"

std::vector<std::string> kvoice::get_input_devices() {
    const char* enumerator = nullptr;
    if (alcIsExtensionPresent(nullptr, "ALC_enumeration_EXT"))
        enumerator = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);

    std::vector<std::string> res;

    std::string_view s;

    while (!(s = get_next_str(enumerator)).empty()) {
        res.emplace_back(s.data(), s.size());
    }
    return res;
}

std::vector<std::string> kvoice::get_output_devices() {
    const char* enumerator = nullptr;
    if (alcIsExtensionPresent(nullptr, "ALC_enumeration_EXT"))
    {
        if (!alcIsExtensionPresent(nullptr, "ALC_enumerate_all_EXT"))
            enumerator = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        else
            enumerator = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
    }

    std::vector<std::string> res;

    std::string_view s;

    while (!(s = get_next_str(enumerator)).empty()) {
        res.emplace_back(s.data(), s.size());
    }

    return res;
}

kvoice::create_sound_device_result<kvoice::sound_output> kvoice::create_sound_output(std::string_view device_name, std::uint32_t sample_rate,
                                                                                     std::uint32_t    src_count) {

    try {
        auto output = std::make_unique<sound_output_impl>(device_name, sample_rate, src_count);
        return { std::move(output), "" };
    } catch (voice_exception& e) {
        return { nullptr, e.what() };
    }
}

kvoice::create_sound_device_result<kvoice::sound_input> kvoice::create_sound_input(std::string_view device_name, std::uint32_t sample_rate,
                                                                                    std::uint32_t frames_per_buffer, std::uint32_t bitrate) {
    try {
        auto output = std::make_unique<sound_input_impl>(device_name, sample_rate, frames_per_buffer, bitrate);
        return { std::move(output), "" };
    } catch (voice_exception& e) {
        return { nullptr, e.what() };
    }
}

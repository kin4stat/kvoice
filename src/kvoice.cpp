#include "kvoice.hpp"

#include "voice_exception.hpp"
#include "sound_output_impl.hpp"
#include "sound_input_impl.hpp"

std::vector<std::string> kvoice::get_input_devices() {
    std::vector<std::string> res;

    BASS_DEVICEINFO info;
    for (auto i = 0u; BASS_RecordGetDeviceInfo(i, &info); i++) {
        if ((info.flags & BASS_DEVICE_ENABLED) && (info.flags & BASS_DEVICE_TYPE_MASK) ==
            BASS_DEVICE_TYPE_MICROPHONE) {
            res.emplace_back(info.name);
        }
    }
    return res;
}

std::vector<std::string> kvoice::get_output_devices() {
    std::vector<std::string> res;

    BASS_DEVICEINFO info;
    for (auto i = 1u; BASS_GetDeviceInfo(i, &info); i++)
        if (info.flags & BASS_DEVICE_ENABLED)
            res.emplace_back(info.name);

    return res;
}

kvoice::create_device_res<kvoice::sound_output> kvoice::create_sound_output(
    std::string_view device_name, std::uint32_t sample_rate) {

    try {
        auto output = std::make_unique<sound_output_impl>(device_name, sample_rate);
        return { std::move(output), "" };
    } catch (voice_exception& e) {
        return { nullptr, e.what() };
    }
}

kvoice::create_device_res<kvoice::sound_input> kvoice::create_sound_input(
    std::string_view device_name, std::uint32_t       sample_rate,
    std::uint32_t    frames_per_buffer, std::uint32_t bitrate) {
    try {
        auto output = std::make_unique<sound_input_impl>(device_name, sample_rate, frames_per_buffer, bitrate);
        return { std::move(output), "" };
    } catch (voice_exception& e) {
        return { nullptr, e.what() };
    }
}

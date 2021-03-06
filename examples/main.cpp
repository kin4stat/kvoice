#include "kvoice/kvoice.hpp"

#include <sstream>
#include <thread>
#include <chrono>

constexpr float radius = 5.f;

std::unique_ptr<kvoice::stream> s1{};
std::unique_ptr<kvoice::stream> s2{};

bool s1_active = true;
bool s2_active = true;

float pos_on_circle = 0.f;

int main() {
    constexpr auto sample_rate = 24000;
    constexpr auto frames_per_buffer = 420;
    constexpr auto bitrate = 16000;

    auto [sound_input, error_msg] = kvoice::create_sound_input("", sample_rate, frames_per_buffer, bitrate);

    sound_input->set_input_callback([](const void* buffer, std::size_t count) {
        if (s1 && s1_active) s1->push_opus_buffer(buffer, count);
        if (s2 && s2_active) s2->push_opus_buffer(buffer, count);
    });
    sound_input->set_raw_input_callback([](const void*, std::size_t, float) {
    });
    sound_input->enable_input();

    auto [sound_output, error_msg1] = kvoice::create_sound_output("", sample_rate, 32);

    s1 = sound_output->create_stream();
    s1->set_max_distance(100.f);
    s1->set_max_distance(30.f);

    s2 = sound_output->create_stream();
    s2->set_max_distance(100.f);
    s2->set_min_distance(30.f);

    sound_output->set_my_position({ 0.f, 0.f, 0.f });
    sound_output->set_my_velocity({ 0.f, 0.f, 0.f });

    sound_output->set_my_orientation_front({ 0.f, 0.f, 1.f });
    sound_output->set_my_orientation_up({ 1.f, 0.f, 0.f });

    sound_output->update_me();

    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            pos_on_circle += 0.02f;
            float x = radius * cosf(pos_on_circle);
            float y = radius * -sinf(pos_on_circle);
            float z = 0;
            s1->set_position({ x, y, z });
            s1->set_velocity({ 0.0f, 0.0f, 0.0f });
            s1->set_direction({ 0.0f, 0.0f, 1.0f });
            if (!s1->update())
                return 1;

            x = radius * cosf(pos_on_circle + 1.5f);
            y = radius * -sinf(pos_on_circle + 1.5f);
            z = 0;
            s2->set_position({ x, y, z });
            s2->set_velocity({ 0.0f, 0.0f, 0.0f });
            s2->set_direction({ 0.0f, 0.0f, 1.0f });
            if (!s2->update())
                return 1;
        }
    }).detach();

    while (true) {
        int key = getchar();
        if (key == '1') s1_active = !s1_active;
        if (key == '2') s2_active = !s2_active;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

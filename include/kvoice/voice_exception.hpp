#pragma once
#include <stdexcept>
#include <string>
#include "fmt/core.h"

namespace kvoice {
class voice_exception final : virtual public std::exception {
    std::string error_msg;

public:
    template <typename... Ts>
    static voice_exception create_formatted(std::string_view fmt, Ts&&...args);

    explicit voice_exception(std::string msg)
        : error_msg(std::move(msg)) {
    }

    ~voice_exception() override = default;

    [[nodiscard]] const char* what() const override { return error_msg.c_str(); }
};

/**
 * @brief creates new @p voice_exception object with formatted message
 * @tparam Ts types of @p args
 * @param fmt format string
 * @param args args that are passed to format function
 * @return new @p voice_exception object
 */
template <typename ...Ts>
inline voice_exception voice_exception::create_formatted(std::string_view fmt, Ts&& ...args) {
    return voice_exception{ std::vformat(fmt, std::make_format_args(std::forward<Ts>(args)...)) };
}

}

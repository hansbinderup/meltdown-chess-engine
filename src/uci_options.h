#pragma once

#include "parsing/input_parsing.h"

#include "fmt/base.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace ucioption {

/* uci types for easier creation */
using string = std::string;
using check = bool;
using spin = uint64_t;

template<typename T>
struct Storage {
    T value;
    const T defaultValue;
    const std::function<void(const T&)> callback;
};

using Variant = std::variant<
    Storage<std::string>,
    Storage<bool>,
    Storage<uint64_t>>;

struct Limits {
    uint64_t min;
    uint64_t max;
};

struct UciOption {
    const std::string_view name;
    Variant variant;
    const std::optional<Limits> limits {};
};

template<typename T>
constexpr auto make(std::string_view name, T defaultVal, std::function<void(const T&)> cb)
{
    return UciOption {
        .name = name,
        .variant = Storage<T> { .value = defaultVal, .defaultValue = defaultVal, .callback = std::move(cb) },
    };
}

template<typename T>
constexpr auto make(std::string_view name, T defaultVal, Limits limits, std::function<void(const T&)> cb)
{
    return UciOption {
        .name = name,
        .variant = Storage<T> { .value = defaultVal, .defaultValue = defaultVal, .callback = std::move(cb) },
        .limits = std::move(limits),
    };
}

constexpr bool handleInput(UciOption& option, std::string_view input)
{
    return std::visit([input, limits = option.limits](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Storage<bool>>) {
            if (input == "true") {
                arg.value = true;
            } else if (input == "false") {
                arg.value = false;
            } else {
                return false;
            }
        } else if constexpr (std::is_same_v<T, Storage<std::string>>) {
            arg.value = input;
        } else if constexpr (std::is_same_v<T, Storage<uint64_t>>) {
            const auto inputNum = parsing::to_number(input);
            if (!inputNum.has_value())
                return false;

            if (limits.has_value()) {
                if (*inputNum < limits->min || *inputNum > limits->max) {
                    return false;
                }
            }

            arg.value = *inputNum;
        } else {
            static_assert(false, "Uci option type handling has not been implemented yet!");
        }

        assert(arg.callback);
        arg.callback(arg.value);

        return true;
    },
        option.variant);
}

/* custom printer to debug current set values */
constexpr void printDebug(const UciOption& option)
{
    std::visit([name = option.name, limits = option.limits](auto&& arg) {
        if (limits.has_value()) {
            fmt::println("name={} value={} limits=[{}:{}]",
                name, arg.value, limits->min, limits->max);
        } else {
            fmt::println("name={} value={}", name, arg.value);
        }
    },
        option.variant);
}

namespace {

constexpr std::string_view uciTypeToString(Variant variant)
{
    return std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Storage<bool>>) {
            return "check";
        } else if constexpr (std::is_same_v<T, Storage<std::string>>) {
            return "string";
        } else if constexpr (std::is_same_v<T, Storage<uint64_t>>) {
            return "spin";
        } else {
            static_assert(false, "string conversion has not yet been implemented");
        }
    },
        variant);
}

}

/* custom printer to print uci information (needed for eg. "uci" from handler) */
constexpr void printInfo(const UciOption& option)
{
    std::visit([typeString = uciTypeToString(option.variant), name = option.name, limits = option.limits](auto&& arg) {
        if (limits.has_value()) {
            fmt::println("option name {} type {} default {} min {} max {}",
                name, typeString, arg.defaultValue, limits->min, limits->max);
        } else {
            fmt::println("option name {} type {} default {}", name, typeString, arg.defaultValue);
        }
    },
        option.variant);
}

}

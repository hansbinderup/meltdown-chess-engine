#pragma once

#include "movegen/move_types.h"

#include "magic_enum/magic_enum.hpp"
#include <fmt/core.h>
#include <string_view>
#include <type_traits>

/* generic formatter to format enum types */
template<typename T>
    requires std::is_enum_v<T>
struct fmt::formatter<T> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const
    {
        std::string_view name = magic_enum::enum_name(value);
        if (name.empty()) {
            /* should not happen, but have a fall back anyways */
            return fmt::format_to(ctx.out(), "{}", magic_enum::enum_integer(value));
        } else {
            return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(value));
        }
    }
};

/* Move type formatter - allows for easy formatting of Moves */
template<>
struct fmt::formatter<movegen::Move> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(movegen::Move m, FormatContext& ctx) const
    {
        char fromC = 'a' + (m.fromPos() % 8); // Column
        char fromR = '1' + (m.fromPos() / 8); // Row
        char toC = 'a' + (m.toPos() % 8); // Column
        char toR = '1' + (m.toPos() / 8); // Row

        if (m.promotionType() != PromotionNone) {
            return fmt::format_to(ctx.out(), "{}{}{}{}{}", fromC, fromR, toC, toR, promotionToString(m.promotionType()));
        }

        return fmt::format_to(ctx.out(), "{}{}{}{}", fromC, fromR, toC, toR);
    }
};

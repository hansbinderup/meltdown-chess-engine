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

/*
 * helper type to specialize a fmt print
 * use as eg:
 * fmt::print("score {}", ScorePrint(score)); -> eg: "score mate 8"
 */
struct ScorePrint {
    Score score;
};

template<>
struct fmt::formatter<ScorePrint> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(ScorePrint sp, FormatContext& ctx) const
    {
        if (sp.score > -s_mateValue && sp.score < -s_mateScore) {
            return fmt::format_to(ctx.out(), "mate {}", -(sp.score + s_mateValue) / 2 - 1);
        } else if (sp.score > s_mateScore && sp.score < s_mateValue) {
            return fmt::format_to(ctx.out(), "mate {}", (s_mateValue - sp.score) / 2 + 1);
        } else {
            return fmt::format_to(ctx.out(), "cp {}", sp.score);
        }
    }
};

/*
 * helper type to specialize a fmt print
 * use as eg:
 * fmt::print("{}", TbHitPrint(tbHits)); -> eg: " tbhits 1" or ""
 * NOTE: will handle spacing by itself
 */
struct TbHitPrint {
    uint64_t tbHits;
};

template<>
struct fmt::formatter<TbHitPrint> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(TbHitPrint tbhit, FormatContext& ctx) const
    {
        if (tbhit.tbHits > 0) {
            return fmt::format_to(ctx.out(), " tbhits {}", tbhit.tbHits);
        }

        return fmt::format_to(ctx.out(), "");
    }
};

/*
 * helper type to specialize a fmt print
 * use as eg:
 * fmt::print("{}", NpsPrint(nodes, timeMs)); -> eg: " nps 1000000" or ""
 * NOTE: will handle spacing by itself
 */
struct NpsPrint {
    uint64_t nodes;
    uint64_t timeMs;
};

template<>
struct fmt::formatter<NpsPrint> : fmt::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const NpsPrint& nps, FormatContext& ctx) const
    {
        if (nps.timeMs > 0) {
            return fmt::format_to(ctx.out(), " nps {}", nps.nodes / nps.timeMs * 1000);
        }

        return fmt::format_to(ctx.out(), "");
    }
};


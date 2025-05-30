#pragma once

#include "core/bit_board.h"
#include "core/move_handling.h"
#include "movegen/move_types.h"
#include <charconv>
#include <optional>
#include <string_view>

namespace parsing {

inline std::optional<int64_t> to_number(std::string_view str)
{
    int64_t result {};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

    // Check for conversion errors or extra characters
    if (ec == std::errc() && ptr == str.data() + str.size()) {
        return result;
    }
    return std::nullopt;
}

namespace {

constexpr std::pair<std::string_view, std::string_view> split_sv_by_space(std::string_view sv)
{
    const auto firstSep = sv.find_first_of(' ');
    const auto first = sv.substr(0, firstSep);
    const auto second = firstSep == std::string_view::npos
        ? ""
        : sv.substr(firstSep + 1);

    return std::make_pair(first, second);
}

constexpr std::optional<std::string_view> sv_next_split(std::string_view& sv)
{
    const auto firstSep = sv.find_first_of(' ');

    if (firstSep == std::string_view::npos) {
        return std::nullopt;
    }

    const auto ret = sv.substr(0, firstSep);
    sv = sv.substr(firstSep + 1);

    return ret;
}

/*
 * Compare moves by only looking at positioning
 * We need to apply the same masks as the bitboard uses to emulate their moves
 */
constexpr static inline bool compareMove(movegen::Move a, uint8_t from, uint8_t to, PromotionType promotion)
{
    if (promotion == PromotionNone)
        return (a.fromPos() == from) && (a.toPos() == to);
    else
        return (a.fromPos() == from) && (a.toPos() == to) && a.promotionType() == promotion;
}

}

static inline std::optional<movegen::Move> moveFromString(const BitBoard& board, std::string_view sv)
{
    if (sv.size() < 4) {
        return std::nullopt;
    }

    // quick and dirty way to transform move string to integer values
    const uint8_t fromIndex = (sv.at(0) - 'a') + (sv.at(1) - '1') * 8;
    const uint8_t toIndex = (sv.at(2) - 'a') + (sv.at(3) - '1') * 8;

    PromotionType promotion { PromotionNone };
    if (sv.size() > 4) {
        const char promotionChar = sv.at(4);
        if (promotionChar == 'n')
            promotion = PromotionKnight;
        else if (promotionChar == 'b')
            promotion = PromotionBishop;
        else if (promotionChar == 'r')
            promotion = PromotionRook;
        else if (promotionChar == 'q')
            promotion = PromotionQueen;
    }

    movegen::ValidMoves moves;
    core::getAllMoves<movegen::MovePseudoLegal>(board, moves);
    for (const auto& move : moves) {
        if (compareMove(move, fromIndex, toIndex, promotion)) {
            return move;
        }
    }

    return std::nullopt;
}

}


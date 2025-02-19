#pragma once

#include "src/engine.h"
#include "src/movement/move_types.h"

namespace parsing {

namespace {

std::optional<uint64_t> to_number(std::string_view str)
{
    int result {};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

    // Check for conversion errors or extra characters
    if (ec == std::errc() && ptr == str.data() + str.size()) {
        return result;
    }
    return std::nullopt;
}

static std::pair<std::string_view, std::string_view> split_sv_by_space(std::string_view sv)
{
    const auto firstSep = sv.find_first_of(' ');
    const auto first = sv.substr(0, firstSep);
    const auto second = firstSep == std::string_view::npos
        ? ""
        : sv.substr(firstSep + 1);

    return std::make_pair(first, second);
}

static std::optional<std::string_view> sv_next_split(std::string_view& sv)
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
constexpr static inline bool compareMove(const movement::Move& a, uint8_t from, uint8_t to, PromotionType promotion)
{
    if (promotion == PromotionType::None)
        return (a.fromValue() == from) && (a.toValue() == to);
    else
        return (a.fromValue() == from) && (a.toValue() == to) && a.promotionType() == promotion;
}

}

static inline std::optional<movement::Move> moveFromString(const Engine& board, std::string_view sv)
{
    if (sv.size() < 4) {
        return std::nullopt;
    }

    // quick and dirty way to transform move string to integer values
    const uint8_t fromIndex = (sv.at(0) - 'a') + (sv.at(1) - '1') * 8;
    const uint8_t toIndex = (sv.at(2) - 'a') + (sv.at(3) - '1') * 8;

    PromotionType promotion { PromotionType::None };
    if (sv.size() > 4) {
        const char promotionChar = sv.at(4);
        if (promotionChar == 'n')
            promotion = PromotionType::Knight;
        else if (promotionChar == 'b')
            promotion = PromotionType::Bishop;
        else if (promotionChar == 'r')
            promotion = PromotionType::Rook;
        else if (promotionChar == 'q')
            promotion = PromotionType::Queen;
    }

    const auto allMoves = board.getAllMoves().getMoves();
    for (const auto& move : allMoves) {
        if (compareMove(move, fromIndex, toIndex, promotion)) {
            return move;
        }
    }

    return std::nullopt;
}

}


#pragma once

#include "core/bit_board.h"
#include "core/transposition.h"
#include "fmt/base.h"
#include "movegen/move_types.h"

#include "tbprobe.h"

namespace syzygy {

enum WdlResult : uint8_t {
    WdlResultLoss,
    WdlResultDraw,
    WdlResultWin,
    WdlResultFailed,
    WdlResultTableNotActive,
};

namespace {

std::span<uint32_t> sortDtzResults(std::span<uint32_t> results, uint32_t wdl)
{
    const bool lowDtz = wdl == TB_WIN || wdl == TB_CURSED_WIN;

    /* Step 1: Find the index of the TB_RESULT_FAILED sentinel */
    size_t validSize = results.size();
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i] == TB_RESULT_FAILED) {
            validSize = i;
            break;
        }
    }

    /* Step 2: Create a subspan with only the valid portion */
    results = results.first(validSize);

    /* Step 3: Insertion sort with custom comparison logic */
    for (size_t i = 1; i < results.size(); ++i) {
        uint32_t key = results[i];
        size_t j = i;

        /* Compare by WDL: higher is better */
        while (j > 0 && TB_GET_WDL(results[j - 1]) < TB_GET_WDL(key)) {
            results[j] = results[j - 1];
            --j;
        }

        if (lowDtz) {
            /* If WDL is equal, compare by DTZ: lower is better */
            while (j > 0 && TB_GET_WDL(results[j - 1]) == TB_GET_WDL(key) && TB_GET_DTZ(results[j - 1]) > TB_GET_DTZ(key)) {
                results[j] = results[j - 1];
                --j;
            }
        } else {
            /* If WDL is equal, compare by DTZ: higher is better */
            while (j > 0 && TB_GET_WDL(results[j - 1]) == TB_GET_WDL(key) && TB_GET_DTZ(results[j - 1]) < TB_GET_DTZ(key)) {
                results[j] = results[j - 1];
                --j;
            }
        }

        results[j] = key;
    }

    return results;
}

constexpr WdlResult wdlFromInt(uint64_t res)
{
    switch (res) {
    case TB_LOSS:
        return WdlResultLoss;
    case TB_CURSED_WIN:
    case TB_BLESSED_LOSS:
    case TB_DRAW:
        return WdlResultDraw;
    case TB_WIN:
        return WdlResultWin;
    default:
        break;
    }

    return WdlResultFailed;
}

}

constexpr bool isTableActive(const BitBoard& board)
{
    if (TB_LARGEST == 0)
        return false;

    return static_cast<unsigned>(std::popcount(board.occupation[Both])) <= TB_LARGEST;
}

inline bool init(std::string_view path)
{
    return tb_init(path.data());
}

inline void deinit()
{
    tb_free();
}

inline uint8_t tableSize()
{
    return TB_LARGEST;
}

inline WdlResult probeWdl(const BitBoard& board)
{
    if (!isTableActive(board)) {
        return WdlResultTableNotActive;
    }

    const uint64_t res = tb_probe_wdl(
        board.occupation[PlayerWhite],
        board.occupation[PlayerBlack],
        board.pieces[WhiteKing] | board.pieces[BlackKing],
        board.pieces[WhiteQueen] | board.pieces[BlackQueen],
        board.pieces[WhiteRook] | board.pieces[BlackRook],
        board.pieces[WhiteBishop] | board.pieces[BlackBishop],
        board.pieces[WhiteKnight] | board.pieces[BlackKnight],
        board.pieces[WhitePawn] | board.pieces[BlackPawn],
        board.halfMoves >= 100,
        board.castlingRights,
        board.enPessant.has_value() ? *board.enPessant : 0,
        board.player == PlayerWhite);

    return wdlFromInt(TB_GET_WDL(res));
}

inline uint32_t probeDtz(const BitBoard& board, std::span<uint32_t> results)
{
    if (!isTableActive(board)) {
        return TB_RESULT_FAILED;
    }

    return tb_probe_root(
        board.occupation[PlayerWhite],
        board.occupation[PlayerBlack],
        board.pieces[WhiteKing] | board.pieces[BlackKing],
        board.pieces[WhiteQueen] | board.pieces[BlackQueen],
        board.pieces[WhiteRook] | board.pieces[BlackRook],
        board.pieces[WhiteBishop] | board.pieces[BlackBishop],
        board.pieces[WhiteKnight] | board.pieces[BlackKnight],
        board.pieces[WhitePawn] | board.pieces[BlackPawn],
        board.halfMoves,
        board.castlingRights,
        board.enPessant.has_value() ? *board.enPessant : 0,
        board.player == PlayerWhite,
        results.data());
}

inline void printDtzDebug(const BitBoard& board)
{
    std::array<uint32_t, TB_MAX_MOVES> results;

    const auto res = probeDtz(board, results);

    if (res == TB_RESULT_CHECKMATE) {
        fmt::println("DTZ checkmate");
    } else if (res == TB_RESULT_STALEMATE) {
        fmt::println("DTZ stalemate");
    } else if (res == TB_RESULT_FAILED) {
        fmt::println("DTZ failed");
    } else {
        const auto sortedResults = sortDtzResults(results, TB_GET_WDL(res));
        fmt::println("DTZ moves:");
        for (const uint32_t res : sortedResults) {

            const auto wdl = wdlFromInt(TB_GET_WDL(res));
            const auto from = magic_enum::enum_cast<BoardPosition>(TB_GET_FROM(res));
            const auto to = magic_enum::enum_cast<BoardPosition>(TB_GET_TO(res));

            if (!from.has_value() || !to.has_value())
                continue;

            fmt::println("Move {}->{}, wdl: {} dtz: {}", *from, *to, wdl, TB_GET_DTZ(res));
        }
        fmt::print("\n");
    }
}

/* NOTE: this method is not thread safe (tb_probe_root is declared as non-thread safe)
 * NOTE: this method should only be called once from root */
inline bool generateSyzygyMoves(const BitBoard& board, movegen::ValidMoves& moves)
{
    std::array<uint32_t, TB_MAX_MOVES> results;

    const auto res = probeDtz(board, results);

    if (res == TB_RESULT_CHECKMATE || res == TB_RESULT_STALEMATE) {
        return true;
    } else if (res == TB_RESULT_FAILED) {
        return false;
    }

    const auto sortedResults = sortDtzResults(results, TB_GET_WDL(res));
    for (const uint32_t res : sortedResults) {
        const auto from = magic_enum::enum_cast<BoardPosition>(TB_GET_FROM(res));
        const auto to = magic_enum::enum_cast<BoardPosition>(TB_GET_TO(res));

        if (!from.has_value() || !to.has_value())
            continue;

        const auto attacker = board.getAttackerAtSquare(utils::positionToSquare(*from), board.player);
        const auto target = board.getTargetAtSquare(utils::positionToSquare(*to), board.player);
        if (!attacker.has_value())
            continue;

        const bool isCapture = target.has_value();

        switch (TB_GET_PROMOTES(res)) {
        case TB_PROMOTES_NONE:
            moves.addMove(movegen::Move::create(*from, *to, isCapture));
            break;
        case TB_PROMOTES_QUEEN:
            moves.addMove(movegen::Move::createPromotion(*from, *to, PromotionQueen, isCapture));
            break;
        case TB_PROMOTES_ROOK:
            moves.addMove(movegen::Move::createPromotion(*from, *to, PromotionRook, isCapture));
            break;
        case TB_PROMOTES_BISHOP:
            moves.addMove(movegen::Move::createPromotion(*from, *to, PromotionBishop, isCapture));
            break;
        case TB_PROMOTES_KNIGHT:
            moves.addMove(movegen::Move::createPromotion(*from, *to, PromotionKnight, isCapture));
            break;
        }
    }

    return true;
}

inline Score wdlToScore(WdlResult wdl, uint8_t ply)
{
    switch (wdl) {
    case WdlResultLoss:
        return -s_mateValue + ply;
    case WdlResultDraw:
        return 0;
    case WdlResultWin:
        return s_mateValue - ply;
    case WdlResultFailed:
    case WdlResultTableNotActive:
        break;
    }

    return s_noScore;
}

inline core::TtFlag wdlToTtFlag(WdlResult wdl)
{
    switch (wdl) {
    case WdlResultLoss:
        return core::TtFlag::TtBeta;
    case WdlResultDraw:
        return core::TtFlag::TtExact;
    case WdlResultWin:
        return core::TtFlag::TtAlpha;
    case WdlResultFailed:
    case WdlResultTableNotActive:
        break;
    }

    return core::TtFlag::TtExact;
}

}

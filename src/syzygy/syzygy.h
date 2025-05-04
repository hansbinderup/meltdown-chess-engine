#pragma once

#include "bit_board.h"
#include "fmt/base.h"
#include "movegen/move_types.h"

#include "tbprobe.h"

namespace syzygy {

enum WdlResult {
    WdlResultLoss,
    WdlResultBlessedLoss,
    WdlResultDraw,
    WdlResultCursedWin,
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
    case TB_BLESSED_LOSS:
        return WdlResultBlessedLoss;
    case TB_DRAW:
        return WdlResultDraw;
    case TB_CURSED_WIN:
        return WdlResultCursedWin;
    case TB_WIN:
        return WdlResultWin;
    default:
        break;
    }

    return WdlResultFailed;
}

std::array<Score, 5> s_wdlScores { -2000, -500, 0, 500, 2000 };

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

inline WdlResult probeWdl(const BitBoard& board, Score& score)
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
        board.halfMoves,
        board.castlingRights,
        board.enPessant.has_value() ? *board.enPessant : 0,
        board.player == PlayerWhite);

    if (res != TB_RESULT_FAILED) {
        score = s_wdlScores[TB_GET_WDL(res)];
    }

    return wdlFromInt(res);
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

inline bool generateSyzygyMoves(const BitBoard& board, movegen::ValidMoves& moves, WdlResult& wdl, uint8_t& dtz)
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

        const auto attacker = board.getAttackerAtSquare(helper::positionToSquare(*from), board.player);
        const auto target = board.getTargetAtSquare(helper::positionToSquare(*to), board.player);
        if (!attacker.has_value())
            continue;

        const bool isCapture = target.has_value();

        switch (TB_GET_PROMOTES(res)) {
        case TB_PROMOTES_NONE:
            moves.addMove(movegen::Move::create(*from, *to, *attacker, isCapture));
            break;
        case TB_PROMOTES_QUEEN:
            moves.addMove(movegen::Move::createPromotion(*from, *to, *attacker, PromotionQueen, isCapture));
            break;
        case TB_PROMOTES_ROOK:
            moves.addMove(movegen::Move::createPromotion(*from, *to, *attacker, PromotionRook, isCapture));
            break;
        case TB_PROMOTES_BISHOP:
            moves.addMove(movegen::Move::createPromotion(*from, *to, *attacker, PromotionBishop, isCapture));
            break;
        case TB_PROMOTES_KNIGHT:
            moves.addMove(movegen::Move::createPromotion(*from, *to, *attacker, PromotionKnight, isCapture));
            break;
        }
    }

    if (!sortedResults.empty()) {
        dtz = TB_GET_DTZ(sortedResults.front());
    }

    wdl = wdlFromInt(TB_GET_WDL(res));

    return true;
}

inline Score approximateDtzScore(const BitBoard& board, Score score, uint8_t dtz, WdlResult wdl, uint8_t& tbhit)
{
    switch (wdl) {
    case WdlResultLoss:
        break;
    case WdlResultBlessedLoss:
    case WdlResultDraw:
    case WdlResultCursedWin:
        tbhit = 1;
        return 0;
    case WdlResultWin:
        break;
    case WdlResultFailed:
    case WdlResultTableNotActive:
        return score;
    }

    tbhit = 1;

    /* If we're already at DTZ 0, it implies a forced draw or mate, so no further approximation needed */
    if (dtz == 0) {
        score = s_mateValue; // Forced mate or draw
    } else if (const int piecesLeft = std::popcount(board.occupation[Both]); piecesLeft == 3) {
        /* last piece to be taken so dtm must be dtz */
        score = s_mateValue - dtz;
    } else {
        /* mate but we don't know when - return high score */
        score = s_mateValue / 2;
    }

    return wdl == WdlResultWin ? score : -score;
}

}

#pragma once

#include "bit_board.h"

#include "tbprobe.h"

namespace syzygy {

enum WdlResult {
    WdlResultLoss,
    WdlResultBlessedLoss,
    WdlResultDraw,
    WdlResultCursedWin,
    WdlResultWin,
    WdlResultFailed,
};

namespace {

inline WdlResult wdlFromInt(uint64_t res)
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

}

inline bool init()
{
    constexpr std::string_view table_path = "/home/hans/repos/meltdown-chess-engine/src/syzygy/tables";
    return tb_init(table_path.data());
}

inline void deinit()
{
    tb_free();
}

inline WdlResult probeWdl(const BitBoard& board)
{
    if (std::popcount(board.occupation[Both]) > 5) {
        return WdlResultFailed;
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

    return wdlFromInt(res);
}

}

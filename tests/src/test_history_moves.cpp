#include "core/bit_board.h"
#include "movegen/move_types.h"
#include "search/history_moves.h"
#include "utils/bit_operations.h"

#include <catch2/catch_test_macros.hpp>

using namespace search;
using namespace movegen;

constexpr inline void setPieceAtSquare(BitBoard& board, Piece type, BoardPosition pos)
{
    board.pieces[type] |= utils::positionToSquare(pos);
    board.occupation[board.player] |= utils::positionToSquare(pos);
}

TEST_CASE("HistoryMoves: Updating quiet moves", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, Knight, C3);

    Move quietMove = Move::create(C3, D5, false);
    REQUIRE(historyMoves.get(Knight, D5) == 0);

    historyMoves.update(board, quietMove, 5);
    REQUIRE(historyMoves.get(Knight, D5) == 5);

    historyMoves.update(board, quietMove, 3);
    REQUIRE(historyMoves.get(Knight, D5) == 8);
}

TEST_CASE("HistoryMoves: Ignoring capture moves", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, Pawn, B2);

    Move captureMove = Move::create(B2, C3, true);
    Move quietMove = Move::create(B2, B4, false);

    // only quit moves should be updated
    historyMoves.update(board, captureMove, 4);
    REQUIRE(historyMoves.get(Pawn, C3) == 0);

    historyMoves.update(board, quietMove, 2);
    REQUIRE(historyMoves.get(Pawn, B4) == 2);
}

TEST_CASE("HistoryMoves: Handling missing piece cases", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    Move quietMove = Move::create(E4, G5, false);
    historyMoves.update(board, quietMove, 6);

    // Since there is no piece at E4, the history score should remain 0
    REQUIRE(historyMoves.get(Bishop, G5) == 0);
}

TEST_CASE("HistoryMoves: Reset functionality", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, Queen, D1);
    Move quietMove = Move::create(D1, H5, false);

    historyMoves.update(board, quietMove, 7);
    REQUIRE(historyMoves.get(Queen, H5) == 7);

    historyMoves.reset();
    REQUIRE(historyMoves.get(Queen, H5) == 0);
}

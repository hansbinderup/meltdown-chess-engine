#include "bit_board.h"
#include "evaluation/history_moves.h"
#include "helpers/bit_operations.h"
#include "movegen/move_types.h"

#include <catch2/catch_test_macros.hpp>

using namespace evaluation;
using namespace movegen;

constexpr inline void setPieceAtSquare(BitBoard& board, Piece type, BoardPosition pos)
{
    board.pieces[type] |= helper::positionToSquare(pos);
}

TEST_CASE("HistoryMoves: Updating quiet moves", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, BlackKnight, C3);

    Move quietMove = Move::create(C3, D5, BlackKnight, false);
    REQUIRE(historyMoves.get(BlackKnight, D5) == 0);

    historyMoves.update(board, quietMove, 5);
    REQUIRE(historyMoves.get(BlackKnight, D5) == 5);

    historyMoves.update(board, quietMove, 3);
    REQUIRE(historyMoves.get(BlackKnight, D5) == 8);
}

TEST_CASE("HistoryMoves: Ignoring capture moves", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, BlackPawn, B2);

    Move captureMove = Move::create(B2, C3, BlackPawn, true);
    Move quietMove = Move::create(B2, B4, BlackPawn, false);

    // only quit moves should be updated
    historyMoves.update(board, captureMove, 4);
    REQUIRE(historyMoves.get(BlackPawn, C3) == 0);

    historyMoves.update(board, quietMove, 2);
    REQUIRE(historyMoves.get(BlackPawn, B4) == 2);
}

TEST_CASE("HistoryMoves: Handling missing piece cases", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    Move quietMove = Move::create(E4, G5, BlackBishop, false);
    historyMoves.update(board, quietMove, 6);

    // Since there is no piece at E4, the history score should remain 0
    REQUIRE(historyMoves.get(BlackBishop, G5) == 0);
}

TEST_CASE("HistoryMoves: Reset functionality", "[HistoryMoves]")
{
    HistoryMoves historyMoves;
    BitBoard board;
    board.player = PlayerBlack;

    setPieceAtSquare(board, BlackQueen, D1);
    Move quietMove = Move::create(D1, H5, BlackQueen, false);

    historyMoves.update(board, quietMove, 7);
    REQUIRE(historyMoves.get(BlackQueen, H5) == 7);

    historyMoves.reset();
    REQUIRE(historyMoves.get(BlackQueen, H5) == 0);
}

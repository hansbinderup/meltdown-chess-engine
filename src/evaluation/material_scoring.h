#pragma once

#include "src/bit_board.h"
#include "src/positioning.h"

#include <cstdint>

namespace evaluation {

constexpr int16_t materialScore(const BitBoard& board)
{
    int16_t score = 0;

    // Material scoring
    score += std::popcount(board.pieces[WhitePawn]) * 100;
    score += std::popcount(board.pieces[WhiteKnight]) * 300;
    score += std::popcount(board.pieces[WhiteBishop]) * 350;
    score += std::popcount(board.pieces[WhiteRook]) * 500;
    score += std::popcount(board.pieces[WhiteQueen]) * 1000;
    score += std::popcount(board.pieces[WhiteKing]) * 10000;

    score -= std::popcount(board.pieces[BlackPawn]) * 100;
    score -= std::popcount(board.pieces[BlackKnight]) * 300;
    score -= std::popcount(board.pieces[BlackBishop]) * 350;
    score -= std::popcount(board.pieces[BlackRook]) * 500;
    score -= std::popcount(board.pieces[BlackQueen]) * 1000;
    score -= std::popcount(board.pieces[BlackKing]) * 10000;

    // Positional scoring
    score += positioning::calculatePawnScore(board.pieces[WhitePawn], Player::White);
    score += positioning::calculateRookScore(board.pieces[WhiteRook], Player::White);
    score += positioning::calculateBishopScore(board.pieces[WhiteBishop], Player::White);
    score += positioning::calculateKnightScore(board.pieces[WhiteKnight], Player::White);
    score += positioning::calculateQueenScore(board.pieces[WhiteQueen], Player::White);
    score += positioning::calculateKingScore(board.pieces[WhiteKing], Player::White);

    score -= positioning::calculatePawnScore(board.pieces[BlackPawn], Player::Black);
    score -= positioning::calculateRookScore(board.pieces[BlackRook], Player::Black);
    score -= positioning::calculateBishopScore(board.pieces[BlackBishop], Player::Black);
    score -= positioning::calculateKnightScore(board.pieces[BlackKnight], Player::Black);
    score -= positioning::calculateQueenScore(board.pieces[BlackQueen], Player::Black);
    score -= positioning::calculateKingScore(board.pieces[BlackKing], Player::Black);

    // bishop pairs are worth extra
    if (std::popcount(board.pieces[WhiteBishop]) >= 2)
        score += 150;

    if (std::popcount(board.pieces[BlackBishop]) >= 2)
        score -= 150;

    // knights are worth more if there are many pawns
    if (std::popcount(board.pieces[BlackPawn]) >= 5)
        score += std::popcount(board.pieces[WhiteKnight]) * 50;

    if (std::popcount(board.pieces[WhitePawn]) >= 5)
        score -= std::popcount(board.pieces[BlackKnight]) * 50;

    return board.player == Player::White ? score : -score;
}

}

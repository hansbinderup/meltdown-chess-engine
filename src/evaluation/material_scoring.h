#pragma once

#include "src/bit_board.h"
#include "src/positioning.h"

#include <cstdint>

namespace evaluation {

constexpr int16_t materialScore(const BitBoard& board)
{
    int16_t score = 0;

    // Material scoring
    score += std::popcount(board.whitePawns) * 100;
    score += std::popcount(board.whiteKnights) * 300;
    score += std::popcount(board.whiteBishops) * 350;
    score += std::popcount(board.whiteRooks) * 500;
    score += std::popcount(board.whiteQueens) * 1000;
    score += std::popcount(board.whiteKing) * 10000;

    score -= std::popcount(board.blackPawns) * 100;
    score -= std::popcount(board.blackKnights) * 300;
    score -= std::popcount(board.blackBishops) * 350;
    score -= std::popcount(board.blackRooks) * 500;
    score -= std::popcount(board.blackQueens) * 1000;
    score -= std::popcount(board.blackKing) * 10000;

    // Positional scoring
    score += positioning::calculatePawnScore(board.whitePawns, Player::White);
    score += positioning::calculateRookScore(board.whiteRooks, Player::White);
    score += positioning::calculateBishopScore(board.whiteBishops, Player::White);
    score += positioning::calculateKnightScore(board.whiteKnights, Player::White);
    score += positioning::calculateQueenScore(board.whiteQueens, Player::White);
    score += positioning::calculateKingScore(board.whiteKing, Player::White);

    score -= positioning::calculatePawnScore(board.blackPawns, Player::Black);
    score -= positioning::calculateRookScore(board.blackRooks, Player::Black);
    score -= positioning::calculateBishopScore(board.blackBishops, Player::Black);
    score -= positioning::calculateKnightScore(board.blackKnights, Player::Black);
    score -= positioning::calculateQueenScore(board.blackQueens, Player::Black);
    score -= positioning::calculateKingScore(board.blackKing, Player::Black);

    return board.player == Player::White ? score : -score;
}

}

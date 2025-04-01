#pragma once

#include "evaluation/score.h"

namespace evaluation {

template<size_t T>
using WeightTable = std::array<Score, T>;

#define TERM_LIST(TERM)             \
    TERM(pieceValues, 6)            \
    TERM(doublePawnPenalty, 1)      \
    TERM(isolatedPawnPenalty, 1)    \
    TERM(passedPawnBonus, 8)        \
    TERM(rookOpenFileBonus, 1)      \
    TERM(rookSemiOpenFileBonus, 1)  \
    TERM(queenOpenFileBonus, 1)     \
    TERM(queenSemiOpenFileBonus, 1) \
    TERM(knightMobilityScore, 9)    \
    TERM(bishopMobilityScore, 14)   \
    TERM(rookMobilityScore, 15)     \
    TERM(queenMobilityScore, 28)    \
    TERM(bishopPairScore, 1)        \
    TERM(kingShieldBonus, 9)        \
    TERM(psqtPawns, 64)             \
    TERM(psqtKnights, 64)           \
    TERM(psqtBishops, 64)           \
    TERM(psqtRooks, 64)             \
    TERM(psqtQueens, 64)            \
    TERM(psqtKings, 64)

#define WEIGHT(name, size) \
    WeightTable<size> name;

struct Terms {
    TERM_LIST(WEIGHT)
};

}

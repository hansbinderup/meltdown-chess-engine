#pragma once

#include "evaluation/term_score.h"

namespace evaluation {

template<size_t T>
using WeightTable = std::array<TermScore, T>;

#define TERM_LIST(TERM)                \
    TERM(tempoScore, 1)                \
    TERM(pieceValues, 6)               \
    TERM(doublePawnPenalty, 1)         \
    TERM(isolatedPawnPenalty, 1)       \
    TERM(protectedPawnScore, 8)        \
    TERM(passedPawnBonus, 8)           \
    TERM(freePassedPawnBonus, 8)       \
    TERM(protectedPassedPawnBonus, 8)  \
    TERM(passersOurKingDistance, 8)    \
    TERM(passersTheirKingDistance, 8)  \
    TERM(passerCandidateDefended, 8)   \
    TERM(passerCandidateUndefended, 8) \
    TERM(pawnSquareRuleBonus, 1)       \
    TERM(pawnShieldBonus, 3)           \
    TERM(pawnStormScore, 3)            \
    TERM(pawnPhalanxScore, 1)          \
    TERM(rookOpenFileBonus, 1)         \
    TERM(rookSemiOpenFileBonus, 1)     \
    TERM(rook7thRankBonus, 1)          \
    TERM(queenOpenFileBonus, 1)        \
    TERM(queenSemiOpenFileBonus, 1)    \
    TERM(knightMobilityScore, 9)       \
    TERM(knightOutpostScore, 4)        \
    TERM(knightShelterBonus, 1)        \
    TERM(bishopMobilityScore, 14)      \
    TERM(bishopOutpostScore, 4)        \
    TERM(bishopShelterBonus, 1)        \
    TERM(badBishopScore, 9)            \
    TERM(bishopLongDiagScore, 1)       \
    TERM(rookMobilityScore, 15)        \
    TERM(queenMobilityScore, 28)       \
    TERM(kingVirtualMobilityScore, 28) \
    TERM(kingZone, 15)                 \
    TERM(kingAttackPawn, 1)            \
    TERM(bishopPairScore, 1)           \
    TERM(pawnAttacks, 5)               \
    TERM(knightAttacks, 5)             \
    TERM(bishopAttacks, 5)             \
    TERM(rookAttacks, 5)               \
    TERM(queenAttacks, 5)              \
    TERM(safeChecks, 5)                \
    TERM(unsafeChecks, 5)              \
    TERM(pawnPushThreats, 6)           \
    TERM(weakPawnAttacked, 1)          \
    TERM(weakMinorKingAttacked, 1)     \
    TERM(weakMinorMajorAttacked, 1)    \
    TERM(weakRookKingAttacked, 1)      \
    TERM(rookAttackedByLesser, 1)      \
    TERM(psqtPawns, 64)                \
    TERM(psqtKnights, 64)              \
    TERM(psqtBishops, 64)              \
    TERM(psqtRooks, 64)                \
    TERM(psqtQueens, 64)               \
    TERM(psqtKings, 64)

#define WEIGHT(name, size) \
    WeightTable<size> name;

struct Terms {
    TERM_LIST(WEIGHT)
};

}

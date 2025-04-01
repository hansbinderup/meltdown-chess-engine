#pragma once

#include "evaluation/evaluation_weights.h"
#include <array>
#include <cstddef>

namespace evaluation {

constexpr size_t s_amountPlayers { 2 };

/* FIXME: make one dimensional and add for white / subtract for black */
template<size_t T>
using TraceTable = std::array<std::array<int16_t, s_amountPlayers>, T>;
using SingleTrace = std::array<int16_t, s_amountPlayers>;

struct EvaluationTrace {
    SingleTrace _begin;

    /* START OF TRACES */
    TraceTable<6> pieceValues {};
    SingleTrace doublePawnPenalty;
    SingleTrace isolatedPawnPenalty;
    TraceTable<8> passedPawnBonus;
    SingleTrace rookOpenFileBonus;
    SingleTrace rookSemiOpenFileBonus;
    SingleTrace queenOpenFileBonus;
    SingleTrace queenSemiOpenFileBonus;
    TraceTable<9> knightMobilityScore;
    TraceTable<14> bishopMobilityScore;
    TraceTable<15> rookMobilityScore;
    TraceTable<28> queenMobilityScore;
    SingleTrace bishopPairScore;
    TraceTable<9> kingShieldBonus;

    /* positions */
    TraceTable<s_amountSquares> psqtPawns;
    TraceTable<s_amountSquares> psqtKnights;
    TraceTable<s_amountSquares> psqtBishops;
    TraceTable<s_amountSquares> psqtRooks;
    TraceTable<s_amountSquares> psqtQueens;
    TraceTable<s_amountSquares> psqtKings;

    /* END OF TRACES */

    SingleTrace _end;

    constexpr const auto* begin() const
    {
        return (&_begin) + 1;
    }

    constexpr const auto* end() const
    {
        return (&_end);
    }

    constexpr size_t size() const
    {
        return end() - begin();
    }
};

static_assert(sizeof(EvaluationTrace) == sizeof(Weights), "Tracer and weights does not match!");

constexpr static inline EvaluationTrace s_emptyTrace {};

}


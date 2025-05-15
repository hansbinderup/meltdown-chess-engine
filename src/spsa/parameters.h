#pragma once

#include "evaluation/score.h"
#include <cstdint>

/* Tunable Parameters for SPSA Optimization
 *
 * This list defines all engine parameters that can be tuned using SPSA.
 *
 * Each parameter includes the following fields:
 *   - Name:  The identifier used by the engine, UCI interface, and SPSA tuner.
 *   - Type:  The data type where the parameter is stored.
 *   - Value: The current or tuned value used by the engine.
 *   - Min:   The minimum value allowed during tuning.
 *   - Max:   The maximum value allowed during tuning.
 *   - Step:  The step size used by SPSA for parameter perturbation.
 *
 * These definitions are used to:
 *   - Generate UCI options for tuner to interface with
 *   - Provide input to the SPSA tuning algorithm
 *   - Create either immutable or mutable engine parameters
 *
 * For more details, see: src/spsa/README.md */
#define TUNABLE_LIST(TUNABLE)                                    \
    TUNABLE(fullDepthMove, uint8_t, 4, 0, 12, 1)                 \
    TUNABLE(lmrBaseReduction, uint8_t, 1, 1, 12, 1)              \
    TUNABLE(rfpReductionLimit, uint8_t, 3, 0, 12, 1)             \
    TUNABLE(rfpMargin, Score, 100, 0, 150, 10)                   \
    TUNABLE(rfpEvaluationMargin, Score, 120, 0, 150, 10)         \
    TUNABLE(razorReductionLimit, uint8_t, 3, 0, 12, 1)           \
    TUNABLE(razorMarginShallow, Score, 125, 0, 250, 10)          \
    TUNABLE(razorMarginDeep, Score, 175, 0, 250, 10)             \
    TUNABLE(razorDeepReductionLimit, uint8_t, 2, 0, 12, 1)       \
    TUNABLE(nmpBaseMargin, int8_t, -120, -200, 0, 10)            \
    TUNABLE(nmpMarginFactor, uint8_t, 20, 0, 100, 5)             \
    TUNABLE(nmpReductionBase, uint8_t, 4, 0, 12, 1)              \
    TUNABLE(nmpReductionFactor, uint8_t, 4, 0, 12, 1)            \
    TUNABLE(aspirationWindow, uint8_t, 50, 10, 100, 5)           \
    TUNABLE(timeManIncFrac, uint16_t, 75, 1, 150, 5)             \
    TUNABLE(timeManBaseFrac, uint16_t, 50, 1, 150, 5)            \
    TUNABLE(timeManLimitFrac, uint16_t, 75, 1, 150, 5)           \
    TUNABLE(timeManSoftFrac, uint16_t, 50, 1, 150, 5)            \
    TUNABLE(timeManHardFrac, uint16_t, 300, 100, 500, 20)        \
    TUNABLE(timeManNodeFracBase, uint8_t, 150, 1, 200, 10)       \
    TUNABLE(timeManNodeFracMultiplier, uint8_t, 170, 1, 200, 10) \
    TUNABLE(timeManScoreMargin, uint8_t, 10, 1, 20, 1)           \
    TUNABLE(seePawnValue, int32_t, 100, 50, 1000, 10)            \
    TUNABLE(seeKnightValue, int32_t, 422, 50, 1000, 10)          \
    TUNABLE(seeBishopValue, int32_t, 422, 50, 1000, 10)          \
    TUNABLE(seeRookValue, int32_t, 642, 50, 1000, 10)      \
TUNABLE(seeQueenValue, int32_t, 1015, 50, 1500, 10)

#ifdef SPSA

#include "uci_options.h"
#include <array>

#define TUNABLE_CONSTEXPR(type) static inline type

/* params should be mutable when tuning */
#define MUTABLE_TUNABLE(name, type, value, min, max, step) TUNABLE_CONSTEXPR(type) name = value;

namespace spsa {
TUNABLE_LIST(MUTABLE_TUNABLE)

#define STRINGIFY(x) #x
#define UCI_SPSA_OPTION(name, type, value, min, max, step) \
    ucioption::make<ucioption::spin>(STRINGIFY(name), value, ucioption::Limits { min, max }, [](uint64_t val) { spsa::name = val; }),

static inline auto uciOptions = std::to_array<ucioption::UciOption>({ TUNABLE_LIST(UCI_SPSA_OPTION) });

#define SPSA_INPUT(name, type, value, min, max, step) \
    #name ", int, " #value ".0, " #min ".0, " #max ".0, " #step ".0, 0.002\n"

static inline std::string_view inputsPrint = TUNABLE_LIST(SPSA_INPUT);

}

#else

#define TUNABLE_CONSTEXPR(type) constexpr static inline type

/* params should be immutable when not tuning */
#define IMMUTABLE_TUNABLE(name, type, value, min, max, step) TUNABLE_CONSTEXPR(type) name = value;

namespace spsa {
TUNABLE_LIST(IMMUTABLE_TUNABLE)
}

#endif

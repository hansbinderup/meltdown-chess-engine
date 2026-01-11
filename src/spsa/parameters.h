#pragma once

#include "evaluation/score.h"
#include <cstdint>

#define USE_BASE_LINE_PARAMETERS 1

/* Tunable Parameters for SPSA Optimization
 *
 * This list defines all engine parameters that can be tuned using SPSA.
 *
 * Each parameter includes the following fields:
 *   - Name:        The identifier used by the engine, UCI interface, and SPSA tuner.
 *   - Type:        The data type where the parameter is stored.
 *   - Value:       The current or tuned value used by the engine.
 *   - Min:         The minimum value allowed during tuning.
 *   - Max:         The maximum value allowed during tuning.
 *   - Step:        The step size used by SPSA for parameter perturbation.
 *   - Baseline:    Non-tuned baseline to make adding new parameters easier.
 *
 * These definitions are used to:
 *   - Generate UCI options for tuner to interface with
 *   - Provide input to the SPSA tuning algorithm
 *   - Create either immutable or mutable engine parameters
 *
 * For more details, see: src/spsa/README.md */
#define TUNABLE_LIST(TUNABLE)                                           \
    TUNABLE(fullDepthMove, uint8_t, 2, 1, 12, 1, 4)                     \
    TUNABLE(rfpReductionLimit, uint8_t, 12, 0, 12, 1, 3)                \
    TUNABLE(rfpMargin, Score, 81, 0, 150, 10, 100)                      \
    TUNABLE(rfpEvaluationMargin, Score, 47, 0, 150, 10, 120)            \
    TUNABLE(razorReductionLimit, uint8_t, 3, 0, 12, 1, 3)               \
    TUNABLE(razorMarginShallow, Score, 53, 0, 250, 10, 125)             \
    TUNABLE(razorMarginDeep, Score, 175, 0, 250, 10, 175)               \
    TUNABLE(razorDeepReductionLimit, uint8_t, 3, 0, 12, 1, 2)           \
    TUNABLE(efpBase, Score, 70, 0, 200, 10, 80)                         \
    TUNABLE(efpImproving, Score, 144, 0, 200, 10, 100)                  \
    TUNABLE(efpMargin, Score, 35, 0, 200, 10, 90)                       \
    TUNABLE(efpDepthLimit, uint8_t, 6, 0, 12, 1, 5)                     \
    TUNABLE(nmpBaseMargin, int8_t, -38, -200, 0, 10, -120)              \
    TUNABLE(nmpMarginFactor, uint8_t, 6, 0, 100, 5, 20)                 \
    TUNABLE(nmpReductionBase, uint8_t, 6, 1, 12, 1, 4)                  \
    TUNABLE(nmpReductionFactor, uint8_t, 5, 1, 12, 1, 4)                \
    TUNABLE(iirDepthLimit, uint8_t, 2, 2, 12, 1, 4)                     \
    TUNABLE(lmpDepthLimit, uint8_t, 9, 1, 15, 1, 10)                    \
    TUNABLE(lmpBase, uint64_t, 11, 0, 15, 1, 10)                        \
    TUNABLE(lmpMargin, uint64_t, 3, 1, 10, 1, 3)                        \
    TUNABLE(lmpImproving, uint64_t, 3, 0, 5, 1, 0)                      \
    TUNABLE(seeQuietMargin, uint8_t, 50, 0, 200, 10, 65)                \
    TUNABLE(seeNoisyMargin, uint8_t, 18, 0, 100, 5, 25)                 \
    TUNABLE(seeDepthLimit, uint8_t, 10, 0, 15, 1, 10)                   \
    TUNABLE(aspirationWindow, uint8_t, 81, 10, 100, 5, 80)              \
    TUNABLE(aspirationMinDepth, uint8_t, 4, 1, 10, 1, 4)                \
    TUNABLE(aspirationMaxWindow, uint16_t, 500, 250, 750, 25, 500)      \
    TUNABLE(pawnCorrectionWeight, uint16_t, 404, 250, 750, 25, 512)     \
    TUNABLE(materialCorrectionWeight, uint16_t, 526, 250, 750, 50, 512) \
    TUNABLE(threatCorrectionWeight, uint16_t, 580, 250, 750, 25, 512)   \
    TUNABLE(timeManIncFrac, uint16_t, 122, 1, 150, 5, 75)               \
    TUNABLE(timeManBaseFrac, uint16_t, 42, 1, 150, 5, 50)               \
    TUNABLE(timeManLimitFrac, uint16_t, 80, 1, 150, 5, 75)              \
    TUNABLE(timeManSoftFrac, uint16_t, 62, 1, 150, 5, 50)               \
    TUNABLE(timeManHardFrac, uint16_t, 456, 100, 500, 20, 300)          \
    TUNABLE(timeManNodeFracBase, uint8_t, 140, 1, 200, 10, 150)         \
    TUNABLE(timeManNodeFracMultiplier, uint8_t, 180, 1, 200, 10, 170)   \
    TUNABLE(timeManScoreMargin, uint8_t, 10, 1, 20, 1, 10)              \
    TUNABLE(seePawnValue, int32_t, 120, 50, 200, 5, 100)                \
    TUNABLE(seeKnightValue, int32_t, 286, 200, 500, 10, 300)            \
    TUNABLE(seeBishopValue, int32_t, 328, 200, 500, 10, 300)            \
    TUNABLE(seeRookValue, int32_t, 511, 350, 750, 10, 500)              \
    TUNABLE(seeQueenValue, int32_t, 866, 750, 1150, 10, 900)

#ifdef SPSA

#include "interface/uci_options.h"
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

#if USE_BASE_LINE_PARAMETERS
#define IMMUTABLE_TUNABLE(name, type, value, min, max, step, baseline) TUNABLE_CONSTEXPR(type) name = baseline;
#else
#define IMMUTABLE_TUNABLE(name, type, value, min, max, step, baseline) TUNABLE_CONSTEXPR(type) name = value;
#endif /* USE_BASE_LINE_PARAMETERS */

namespace spsa {
TUNABLE_LIST(IMMUTABLE_TUNABLE)
}

#endif

#pragma once

#include "evaluation/evaluation_weights.h"
#include "evaluation/game_phase.h"
#include "magic_enum/magic_enum.hpp"
#include <cstddef>

namespace tuner {

constexpr size_t s_kPrecision { 10 };
constexpr size_t s_epochs { 10000 };
constexpr size_t s_nTerms { evaluation::s_weights.size() };
/* constexpr size_t s_nTerms { 472 }; */
constexpr size_t s_gamePhases { magic_enum::enum_count<evaluation::gamephase::Phases>() };
constexpr double s_beta1 { 0.9 }; /* momementum */
constexpr double s_beta2 { 0.999 }; /* velocity */
constexpr double s_learningRate { 0.1 };
constexpr size_t s_positions { 7153653 };
constexpr size_t s_threads { 14 };

constexpr size_t s_parameterPrintRate { 5 };
constexpr size_t s_lrStepRate { 250 };
constexpr double s_lrDropRate { 1.0 };

}

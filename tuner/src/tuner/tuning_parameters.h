#pragma once

#include "evaluation/terms.h"
#include "magic_enum/magic_enum.hpp"
#include <cstddef>

namespace tuner {

constexpr size_t s_epochs { 2000 };
constexpr size_t s_gamePhases { magic_enum::enum_count<Phases>() };
constexpr size_t s_nTerms { sizeof(evaluation::Terms) / sizeof(evaluation::TermScore) };
constexpr double s_beta1 { 0.9 }; /* momementum */
constexpr double s_beta2 { 0.999 }; /* velocity */
constexpr size_t s_positions { 7153653 };
constexpr size_t s_threads { 14 };
constexpr size_t s_parameterPrintRate { 50 };

/* these can be configured */
constexpr double s_kValue { 2.25 }; /* modify if the tuning values are too high / low */
constexpr double s_learningRate { 1.0 };
constexpr size_t s_lrStepRate { 250 };
constexpr double s_lrDropRate { 2.0 };

}

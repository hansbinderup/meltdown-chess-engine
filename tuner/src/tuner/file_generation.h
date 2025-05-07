#pragma once

#include "training_data.h"
#include "tuning_parameters.h"

#include <fmt/base.h>
#include <fmt/ostream.h>

#include <fstream>

void prettyPrintToFile(const tuner::GradientArray& params, size_t epochs, double error)
{
    using namespace std::chrono;

    auto now = floor<minutes>(system_clock::now());
    auto t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ofstream file(GENERATED_FILE, std::ios::binary | std::ios::trunc); // overwrite the file
    if (!file) {
        fmt::print(stderr, "Failed to open output file");
        return;
    }

    fmt::print(file, R"cpp(#pragma once

/* ===========================================================
   Auto-Generated Tuning Parameters
   -----------------------------------------------------------
   Generated: {:02}/{:02}/{} at {:02}:{:02}
   -----------------------------------------------------------
   Parameters:
     ▸ K-value          = {:.2f}
     ▸ Learning Rate    = {:.2f}
     ▸ LR Step Rate     = {}
     ▸ LR Drop Rate     = {:.2f}
     ▸ Epochs           = {}
     ▸ Error            = {:.8f}
   -----------------------------------------------------------
   See the 'tuner/' for more information on how to generate
   =========================================================== */

#include "evaluation/terms.h"

namespace evaluation {{

)cpp",
        tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min,
        tuner::s_kValue, tuner::s_learningRate, tuner::s_lrStepRate, tuner::s_lrDropRate, epochs, error);

    fmt::print(file, "\nconstexpr Terms s_terms = {{\n");

    size_t index = 0;
    for (const auto& trace : evaluation::s_traceIterable) {
        fmt::print(file, "    .{} = {{ ", trace.name);
        for (size_t i = 0; i < trace.traces.size(); ++i) {
            fmt::print(file, "TermScore({}, {}), ",
                std::round(params[GamePhaseMg][index]),
                std::round(params[GamePhaseEg][index]));
            ++index;
        }
        fmt::print(file, "}},\n");
    }

    fmt::print(file, "}};\n}}");

    fmt::println("Results have been written to {}", GENERATED_FILE);
}


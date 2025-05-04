#pragma once

#include "tuner/tuning_parameters.h"

#include "bit_board.h"
#include "evaluation/static_evaluation.h"
#include "parsing/fen_parser.h"

#include "fmt/base.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace tuner {

constexpr std::string_view trainingDataPath { TRAINING_DATA_PATH };

struct Term {
    int16_t weight;
    uint16_t index;
};

using GradientArray = std::array<std::array<double, s_nTerms>, s_gamePhases>;
using Terms = std::vector<Term>;
using PFactors = std::array<double, s_gamePhases>;

struct TrainingData {
    double result;
    int16_t phase;
    Terms terms;
    PFactors pFactors;
    evaluation::TermScore eval = evaluation::TermScore(0, 0);
};

Terms createTerms()
{
    Terms terms;

    size_t index = 0;
    for (const auto termInfo : evaluation::s_traceIterable) {
        for (const int16_t trace : termInfo.traces) {
            if (trace != 0) {
                terms.emplace_back(trace, index);
            }

            index++;
        }
    }

    terms.shrink_to_fit();
    return terms;
}

constexpr std::optional<double> resultFromSv(std::string_view sv)
{
    if (sv == "0.0")
        return 0.0;
    else if (sv == "0.5")
        return 0.5;
    else if (sv == "1.0")
        return 1.0;

    return std::nullopt;
}

inline std::optional<double> parseResult(std::string_view sv)
{
    size_t start_pos = sv.rfind('[');
    size_t end_pos = sv.rfind(']');

    if (start_pos != std::string_view::npos && end_pos != std::string_view::npos && end_pos > start_pos) {
        std::string_view extracted = sv.substr(start_pos + 1, end_pos - start_pos - 1);
        return resultFromSv(extracted);
    }

    return std::nullopt;
}

inline uint8_t computePhase(const BitBoard& board)
{
    uint8_t phase = 0;
    for (const auto piece : magic_enum::enum_values<Piece>()) {
        phase += std::popcount(board.pieces[piece]) * s_piecePhaseValues[piece];
    }

    return phase;
}

inline bool unpackTrainingData(std::array<TrainingData, s_positions>& data)
{
    if (!std::filesystem::exists(trainingDataPath)) {
        fmt::println("{} does not exist. Run 'download-training-data.sh' from the 'tuner/scripts' directory", trainingDataPath);
        return false;
    }

    std::ifstream file(trainingDataPath.data());
    if (!file) {
        fmt::println("{} could not be opened. Run 'download-training-data.sh' from the 'tuner/scripts' directory", trainingDataPath);
        return false;
    }

    const size_t printInterval = s_positions / 10;
    size_t count = 0;

    std::string line;
    while (std::getline(file, line)) {
        /* trace should always be reset between each evaluation */
        evaluation::s_trace = {};

        const auto board = parsing::FenParser::parse(line);
        const auto res = parseResult(line);

        if (board.has_value() && res.has_value()) {
            std::ignore = evaluation::staticEvaluation(*board);

            const uint8_t phase = computePhase(*board);
            const PFactors pFactors { phase / 24.0, (24.0 - phase) / 24.0 };

            /* NOTE: must be called after static evaluation to reflect weights of that given eval */
            const auto terms = createTerms();

            TrainingData t = {
                .result = *res,
                .phase = phase,
                .terms = std::move(terms),
                .pFactors = std::move(pFactors),
                /* NOTE: the eval is set to zero to ensure we get a "clean slate" whenever we run a tuning */
                .eval = evaluation::TermScore(0, 0),
            };

            data[count] = std::move(t);
        }

        if (count++ % printInterval == 0) {
            fmt::println("unpacked {:.0f}%..", (100.f / s_positions) * count);
        }

        if (count >= s_positions)
            break;
    }

    return true;
}
}

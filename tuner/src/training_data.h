#pragma once

#include "bit_board.h"
#include "evaluation/game_phase.h"
#include "evaluation/static_evaluation.h"
#include "parsing/fen_parser.h"

#include "fmt/base.h"
#include "src/tuning_parameters.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace tuner {

/* constexpr std::string_view trainingDataPath { "tuner/training-data/lichess-big3-resolved.book" }; */
constexpr std::string_view trainingDataPath { "/home/hans/repos/meltdown-chess-engine/tuner/training-data/lichess-big3-resolved.book" };

using TVector = std::array<std::array<float, s_nTerms>, s_gamePhases>;
using TCoeffs = std::array<float, s_nTerms>;

void initBaseParameters(TVector& params, const evaluation::Weights& weights)
{
    using namespace evaluation::gamephase;

    size_t index = 0;
    for (const auto& term : weights) {
        params[GamePhaseMg][index] = term.mg();
        params[GamePhaseEg][index] = term.eg();

        index++;
    }
}

void initCoefficients(TCoeffs& coeffs, const evaluation::EvaluationTrace& weights)
{
    using namespace evaluation::gamephase;

    size_t index = 0;
    for (const auto& term : weights) {
        coeffs[index] = term[PlayerWhite] - term[PlayerBlack];

        index++;
    }
}

struct TrainingData {
    Player player;
    double result;
    uint8_t phase;
    int32_t seval;
    evaluation::Score eval = evaluation::Score(0, 0);
    TCoeffs baseCoeffs {};
};

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
        phase += std::popcount(board.pieces[piece]) * evaluation::gamephase::s_materialPhaseScore[piece];
    }

    return phase;
}

inline bool unpackTrainingData(std::array<TrainingData, s_positions>& data)
{

    // Check if the file exists
    if (!std::filesystem::exists(trainingDataPath)) {
        fmt::println("{} does not exist. Run 'download-training-data.sh' from the 'tuner' directory", trainingDataPath);
        return false;
    }

    // Open the file for reading
    std::ifstream file(trainingDataPath.data());
    if (!file) {
        fmt::println("{} could not be opened. Run 'download-training-data.sh' from the 'tuner' directory", trainingDataPath);
        return false;
    }

    size_t dataSize = std::ranges::count(std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>(), '\n');

    /* the above will iterate the file and therefore invalidate the read below
     * ensure that we reset the file iterator */
    file.clear();
    file.seekg(0);

    fmt::println("Data contains {} positions", dataSize);

    const size_t printInterval = dataSize / 10;
    size_t count = 0;

    // Read and print each line
    std::string line;
    while (std::getline(file, line)) {
        /* reset trace */
        evaluation::s_trace = evaluation::s_emptyTrace;

        const auto board = parsing::FenParser::parse(line);
        const auto res = parseResult(line);

        if (board.has_value() && res.has_value()) {
            const uint8_t phase = computePhase(*board);
            const int32_t sEval = evaluation::staticEvaluation(*board);
            const evaluation::Score eval = evaluation::staticEvaluationScore(*board);

            TrainingData t = {
                .player = board->player,
                .result = *res,
                .phase = phase,
                .seval = sEval,
                .eval = eval,
            };

            /* NOTE: must be called after static evaluation to reflect weights of that given eval */
            initCoefficients(t.baseCoeffs, evaluation::s_trace);

            data[count] = std::move(t);
        }

        if (count++ % printInterval == 0) {
            fmt::println("unpacked {:.0f}%..", (100.f / dataSize) * count);
        }

        if (count >= s_positions)
            break;
    }

    return true;
}
}

#pragma once

#include "evaluation/game_phase.h"
#include "src/training_data.h"
#include "src/tuning_parameters.h"

#include <cmath>

namespace tuner {

using namespace evaluation::gamephase;

double sigmoid(double K, double E)
{
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

static double staticEvaluationErrors(std::span<tuner::TrainingData> data, double k)
{
    double total = 0.0;

    // clang-format off
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, s_positions / s_threads) reduction(+ : total)
        for (const auto& d : data) {
            total += pow(d.result - sigmoid(k, d.seval), 2);
        }
    }
    // clang-format on

    return total / s_positions;
}

double computeOptimalK(std::span<tuner::TrainingData> entries)
{
    const double rate = 100, delta = 1e-5, deviation_goal = 1e-6;
    double K = 2, deviation = 1;

    size_t epoch = 0;

    while (fabs(deviation) > deviation_goal) {
        double up = staticEvaluationErrors(entries, K + delta);
        double down = staticEvaluationErrors(entries, K - delta);
        deviation = (up - down) / (2 * delta);
        K -= deviation * rate;

        fmt::println("Epoch[{}] K={}", epoch++, K);
    }

    return K;
}

double linearEvaluation(const TrainingData& data, const TVector& params)
{
    int32_t mgScore = data.eval.mg();
    int32_t egScore = data.eval.eg();

    for (size_t i = 0; i < s_nTerms; i++) {
        mgScore += data.baseCoeffs[i] * params[GamePhaseMg][i];
        egScore += data.baseCoeffs[i] * params[GamePhaseEg][i];
    }

    double eval = ((data.phase * mgScore) + egScore * (24 - data.phase)) / 24.f;
    return data.player == PlayerWhite ? eval : -eval;
}

static void updateSingleGradient(const TrainingData& data, TVector& gradient, const TVector& params, double K)
{
    const double E = linearEvaluation(data, params);
    const double S = sigmoid(K, E);
    const double X = (data.result - S) * S * (1 - S);

    const double mgBase = X * data.phase;
    const double egBase = X * (s_middleGamePhase - data.phase);

    for (size_t i = 0; i < s_nTerms; i++) {
        const auto coeff = data.baseCoeffs[i];

        gradient[GamePhaseMg][i] += mgBase * coeff;
        gradient[GamePhaseEg][i] += egBase * coeff;
    }
}

static void computeGradient(std::array<TrainingData, s_positions>& data, TVector& gradient, const TVector& params, double K)
{
    // clang-format off
    #pragma omp parallel shared(gradient)
    {
        TVector local = {};

        #pragma omp for schedule(static, s_positions / s_threads)
        for (auto& d : data) {
            updateSingleGradient(d, local, params, K);
        }

        for (size_t i = 0; i < s_nTerms; i++) {
            gradient[GamePhaseMg][i] += local[GamePhaseMg][i];
            gradient[GamePhaseEg][i] += local[GamePhaseEg][i];
        }
    }
    // clang-format on
}

static double TunedEvaluationErrors(const std::array<TrainingData, s_positions>& data, const TVector& params, double K)
{
    double total = 0.0;

    // clang-format off
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, s_positions / s_threads) reduction(+ : total)
        for (const auto& d : data)
            total += pow(d.result - sigmoid(K, linearEvaluation(d, params)), 2);
    }
    // clang-format on

    return total / (double)data.size();
}

static void runGradientDescent()
{
    std::array<tuner::TrainingData, s_positions> trainingData {};
    bool res = tuner::unpackTrainingData(trainingData);
    if (!res) {
        exit(1);
    }

    const size_t nPositions = trainingData.size();
    /* const double k = tuner::computeOptimalK(trainingData); */
    const double k = 2.25;

    TVector baseParams = {};
    TVector params = {};
    TVector momentum = {};
    TVector velocity = {};

    double rate = s_learningRate;

    initBaseParameters(baseParams, evaluation::s_weights);
    initBaseParameters(params, evaluation::s_weights);

    for (size_t epoch = 0; epoch < s_epochs; epoch++) {
        TVector gradient {};

        computeGradient(trainingData, gradient, params, k);

        for (size_t i = 0; i < s_nTerms; i++) {
            double mg_grad = (-k / 200.0) * gradient[GamePhaseMg][i] / nPositions;
            double eg_grad = (-k / 200.0) * gradient[GamePhaseEg][i] / nPositions;

            momentum[GamePhaseMg][i] = s_beta1 * momentum[GamePhaseMg][i] + (1.0 - s_beta1) * mg_grad;
            momentum[GamePhaseEg][i] = s_beta1 * momentum[GamePhaseEg][i] + (1.0 - s_beta1) * eg_grad;

            velocity[GamePhaseMg][i] = s_beta2 * velocity[GamePhaseMg][i] + (1.0 - s_beta2) * pow(mg_grad, 2);
            velocity[GamePhaseEg][i] = s_beta2 * velocity[GamePhaseEg][i] + (1.0 - s_beta2) * pow(eg_grad, 2);

            params[GamePhaseMg][i] -= rate * momentum[GamePhaseMg][i] / (1e-8 + sqrt(velocity[GamePhaseMg][i]));
            params[GamePhaseEg][i] -= rate * momentum[GamePhaseEg][i] / (1e-8 + sqrt(velocity[GamePhaseEg][i]));
        }

        /* double error = TunedEvaluationErrors(trainingData, params, k); */
        double error = 0;

        if (epoch % s_lrStepRate == 0)
            rate = rate / s_lrDropRate;

        if (epoch % s_parameterPrintRate == 0) {
            fmt::println("Finished epoch: {}, err: {:.8f}", epoch + 1, error);
            for (size_t i = 0; i < s_nTerms; i++) {
                fmt::print("Score({:.2f},{:.2f}) ", params[GamePhaseMg][i], params[GamePhaseEg][i]);
            }
            fmt::print("\n\n");
        }
    }
}
}


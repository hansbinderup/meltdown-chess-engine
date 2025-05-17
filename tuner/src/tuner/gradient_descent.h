#pragma once

#include "tuner/file_generation.h"
#include "tuner/training_data.h"
#include "tuner/tuning_parameters.h"

#include <fmt/chrono.h>

#include <cmath>

namespace tuner {

double sigmoid(double K, double E)
{
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}

double linearEvaluation(const TrainingData& data, const GradientArray& params, evaluation::TermScore score)
{
    int16_t mgScore = score.mg();
    int16_t egScore = score.eg();

    for (size_t i = 0; i < data.terms.size(); i++) {
        const double weight = static_cast<double>(data.terms[i].weight);
        const uint16_t index = data.terms[i].index;

        mgScore += weight * params[GamePhaseMg][index];
        egScore += weight * params[GamePhaseEg][index];
    }

    return ((data.phase * mgScore) + (egScore * (s_middleGamePhase - data.phase))) / static_cast<double>(s_middleGamePhase);
}

static void updateSingleGradient(const TrainingData& data, GradientArray& gradient, const GradientArray& params, double K)
{
    const double E = linearEvaluation(data, params, data.eval);
    const double S = sigmoid(K, E);
    const double X = (data.result - S) * S * (1 - S);

    double mgBase = X * data.pFactors[GamePhaseMg];
    double egBase = X * data.pFactors[GamePhaseEg];

    for (size_t i = 0; i < data.terms.size(); i++) {
        const auto weight = data.terms[i].weight;
        const size_t index = data.terms[i].index;

        gradient[GamePhaseMg][index] += mgBase * weight;
        gradient[GamePhaseEg][index] += egBase * weight;
    }
}

static void computeGradient(std::array<TrainingData, s_positions>& data, GradientArray& gradient, const GradientArray& params, double K)
{
    // clang-format off
    #pragma omp parallel shared(gradient)
    {
        GradientArray local = {};

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

static double TunedEvaluationErrors(std::array<TrainingData, s_positions>& data, const GradientArray& params, double K)
{
    double total = 0.0;

    // clang-format off
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, s_positions / s_threads) reduction(+ : total)
        for ( auto& d : data) {
            total += pow(d.result - sigmoid(K, linearEvaluation(d, params, d.eval)), 2);
        }
    }
    // clang-format on

    return total / (double)data.size();
}

static std::chrono::seconds estimateTimeLeft(const std::chrono::duration<double> timeSpent, size_t epoch)
{
    using namespace std::chrono;

    const auto timeSpentEachEpoch = timeSpent / epoch;
    return duration_cast<seconds>((s_epochs - epoch) * timeSpentEachEpoch);
}

static void runGradientDescentTuning()
{
    using namespace std::chrono;

    std::array<tuner::TrainingData, s_positions> trainingData {};
    bool res = tuner::unpackTrainingData(trainingData);
    if (!res) {
        exit(1);
    }

    GradientArray params = {};
    GradientArray momentum = {};
    GradientArray velocity = {};

    const auto startTime = steady_clock::now();

    for (size_t epoch = 1; epoch < s_epochs + 1; epoch++) {
        GradientArray gradient {};

        computeGradient(trainingData, gradient, params, s_kValue);

        for (size_t i = 0; i < s_nTerms; i++) {
            const double mg_grad = (-s_kValue / 200.0) * gradient[GamePhaseMg][i] / s_positions;
            const double eg_grad = (-s_kValue / 200.0) * gradient[GamePhaseEg][i] / s_positions;

            momentum[GamePhaseMg][i] = s_beta1 * momentum[GamePhaseMg][i] + (1.0 - s_beta1) * mg_grad;
            momentum[GamePhaseEg][i] = s_beta1 * momentum[GamePhaseEg][i] + (1.0 - s_beta1) * eg_grad;

            velocity[GamePhaseMg][i] = s_beta2 * velocity[GamePhaseMg][i] + (1.0 - s_beta2) * pow(mg_grad, 2);
            velocity[GamePhaseEg][i] = s_beta2 * velocity[GamePhaseEg][i] + (1.0 - s_beta2) * pow(eg_grad, 2);

            params[GamePhaseMg][i] -= s_learningRate * momentum[GamePhaseMg][i] / (1e-8 + sqrt(velocity[GamePhaseMg][i]));
            params[GamePhaseEg][i] -= s_learningRate * momentum[GamePhaseEg][i] / (1e-8 + sqrt(velocity[GamePhaseEg][i]));
        }

        double error = TunedEvaluationErrors(trainingData, params, s_kValue);

        const duration<double> timeSpent = steady_clock::now() - startTime;
        const auto timeLeft = estimateTimeLeft(timeSpent, epoch);
        const double progress = 100.0 / s_epochs * epoch;

        fmt::println("Epoch: {} [{:.2f}%] | err: {:.8f} | elapsed: {:%M:%S} | eta: {:%M:%S}",
            epoch, progress, error, duration_cast<seconds>(timeSpent), timeLeft);

        if (epoch % s_parameterPrintRate == 0) {
            prettyPrintToFile(params, epoch, error);
        }
    }
}
}


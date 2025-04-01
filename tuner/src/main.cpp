#include "fmt/base.h"

#include "src/gradient_descent.h"

// https://github.com/AndyGrant/Ethereal/blob/master/src/tuner.h
// https://github.com/TerjeKir/weiss/blob/95b0951bcca19e64f6b4dc81e3270f3a67b64f8f/src/tuner/tuner.c

int main()
{
    fmt::println("\n##########################\n"
                 "      MELTDOWN TUNER      \n"
                 "##########################\n\n"
                 "Threads: {}\n"
                 "Positions: {}\n"
                 "Terms: {}\n"
                 "Learning rate: {:.2f}\n\n",
        tuner::s_threads, tuner::s_positions, tuner::s_nTerms, tuner::s_learningRate);

    tuner::runGradientDescent();

    return 0;
}

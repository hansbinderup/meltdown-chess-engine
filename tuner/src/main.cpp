#include "fmt/base.h"
#include "tuner/gradient_descent.h"

int main()
{
    fmt::println("\n##########################\n"
                 "      MELTDOWN TUNER      \n"
                 "##########################\n\n"
                 "Threads: {}\n"
                 "Positions: {}\n"
                 "Terms: {}\n"
                 "Epochs: {}\n"
                 "Learning rate: {:.2f}\n\n",
        tuner::s_threads, tuner::s_positions, tuner::s_nTerms, tuner::s_epochs, tuner::s_learningRate);

    tuner::runGradientDescentTuning();

    return 0;
}

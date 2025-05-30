#include "interface/uci_handler.h"
#include "tools/bench.h"
#include "version/version.h"

#include "fmt/base.h"

int main(int argc, char** argv)
{
    const auto args = std::span(argv, argc);
    for (const auto arg : args) {
        if (std::strcmp(arg, "bench") == 0) {
            evaluation::Evaluator evaluator {};
            tools::Bench::run(evaluator);
            return 0;
        }
    }

    fmt::print(
        "============================\n"
        "          MELTDOWN          \n"
        "        Chess Engine        \n"
        "============================\n\n");

    fmt::print("Engine:      Meltdown\n"
               "Authors:     Run 'authors'\n"
               "Github:      hansbinderup/meltdown-chess-engine\n"
               "Version:     {}\n"
               "Build hash:  {}\n"
               "Build type:  {}\n"
               "Builtin:     {}\n\n",
        s_meltdownVersion, s_meltdownBuildHash, s_meltdownBuildType, s_meltdownBuiltinFeature);

#if defined(TUNING) || defined(SPSA)
    fmt::println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n"
                 "WARNING: THIS IS A TUNING BUILD!\n"
                 "THIS BUILD IS ONLY MEANT FOR TUNING THE ENGINE\n\n"
                 "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif

    UciHandler::run();

    return 0;
}

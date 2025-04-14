#include "engine/bench.h"
#include "uci_handler.h"
#include "version/version.h"

#include "fmt/base.h"

int main(int argc, char** argv)
{
    const auto args = std::span(argv, argc);
    for (const auto arg : args) {
        if (std::strcmp(arg, "bench") == 0) {
            engine::Bench::run();
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

    UciHandler::run();

    return 0;
}

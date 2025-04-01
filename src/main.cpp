
#include "uci_handler.h"
#include "version/version.h"

#include "fmt/base.h"

int main(int argc, char** argv)
{
    std::ignore = argc;
    std::ignore = argv;

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

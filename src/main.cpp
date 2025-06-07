#include "interface/uci_handler.h"
#include "tools/bench.h"

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

    interface::printEngineInfo();
    UciHandler::run();

    return 0;
}

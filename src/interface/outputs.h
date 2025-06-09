#pragma once

#include "core/bit_board.h"
#include "core/time_manager.h"
#include "search/searcher.h"
#include "version.h"

#include <fmt/color.h>

#include <memory>

#ifdef _WIN32
/* FIXME: for now disable pretty print for windows builds */
static const bool s_prettyPrintSupported = false;
#else
#include <unistd.h>
static const bool s_prettyPrintSupported = isatty(fileno(stdout)) != 0;
#endif

namespace {

static inline bool s_isPrettyPrintEnabled = s_prettyPrintSupported;

inline void printSearchInfoUci(std::shared_ptr<search::Searcher> searcher, const BitBoard& board, Score score, uint8_t currentDepth, uint64_t nodes, uint64_t tbHits)
{
    const auto timeDiff = TimeManager::timeElapsedMs().count();

    const auto adjustedScore = searcher->approxDtzScore(board, score);
    const uint16_t hashFull = core::TranspositionTable::getHashFull();

    fmt::print("info score {} time {} depth {} seldepth {} nodes {} hashfull {}{}{} pv ",
        ScorePrint(adjustedScore),
        timeDiff,
        currentDepth,
        searcher->getSelDepth(),
        nodes,
        hashFull,
        NpsPrint(nodes, timeDiff),
        TbHitPrint(tbHits));

    fmt::println("{}", fmt::join(searcher->getPvTable(), " "));
}

inline void printSearchInfoPretty(std::shared_ptr<search::Searcher> searcher, const BitBoard& board, Score score, uint8_t currentDepth, uint64_t nodes, uint64_t tbHits)
{
    using namespace fmt;
    using namespace std::chrono_literals;

    const auto timeDiff = TimeManager::timeElapsedMs();
    const auto knps = nodes / (timeDiff.count() + 1);

    const auto adjustedScoreCp = searcher->approxDtzScore(board, score) / 100.f;
    const auto mateScore = scoreMateDistance(score);
    const auto scoreColor = score < 0 ? fg(color::red) : fg(color::lawn_green);

    const auto hashFull = core::TranspositionTable::getHashFull() / 10.0;
    const auto selDepth = searcher->getSelDepth();

    std::string scoreBuffer;
    if (mateScore) {
        if (*mateScore > 0)
            scoreBuffer = format("+{}M", *mateScore);
        else
            scoreBuffer = format("{}M", *mateScore);
    } else {
        scoreBuffer = format("{:.2f}", adjustedScoreCp);
    }

    std::string timeBuffer;
    if (timeDiff < 1s) {
        /* already ms */
        timeBuffer = format("{}ms", timeDiff.count());
    } else if (timeDiff < 1min) {
        timeBuffer = format("{:.2f}s", timeDiff.count() / 1000.f);
    } else {
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(timeDiff);
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeDiff - minutes);
        timeBuffer = format("{:>2}m {:02}s", minutes.count(), seconds.count());
    }

    std::string nodesBuffer;
    if (nodes > 1'000'000'000) {
        nodesBuffer = format("{:.2f}b nodes", nodes / 1'000'000'000.f);
    } else if (nodes > 1'000'000) {
        nodesBuffer = format("{:.2f}m nodes", nodes / 1'000'000.f);
    } else if (nodes > 1'000) {
        nodesBuffer = format("{:.2f}k nodes", nodes / 1'000.f);
    } else {
        nodesBuffer = format("{} nodes", nodes);
    }

    std::string npsBuffer;
    if (knps > 1'000) {
        npsBuffer = format("{:.2f}mnps", knps / 1000.f);
    } else {
        npsBuffer = format("{}knps", knps);
    }

    std::string tbHitsBuffer;
    if (tbHits) {
        tbHitsBuffer = format("{} tbhits ║", tbHits);
    }

    println(
        "{:>3}/{:<3} ║ {:>6} ║ {:>7} ║ {:>5.1f}% tt ║ {:>13} ║ {:>8} ║ {}PV: {}",
        styled(currentDepth, fg(color::light_sky_blue)),
        styled(selDepth, fg(color::light_blue)),
        styled(scoreBuffer, scoreColor),
        styled(timeBuffer, fg(color::yellow)),
        hashFull,
        styled(nodesBuffer, fg(color::light_gray)),
        styled(npsBuffer, fg(color::light_gray)),
        styled(tbHitsBuffer, fg(color::light_gray)),
        styled(join(searcher->getPvTable(), " "), fg(color::dim_gray)));
}

void printHeader()
{
    if (s_isPrettyPrintEnabled) {
        constexpr int width = 28;

        auto printLine = [&]() {
            fmt::print("╔{0:═^{1}}╗\n", "", width);
        };

        auto printFooter = [&]() {
            fmt::print("╚{0:═^{1}}╝\n", "", width);
        };

        auto printCentered = [&](std::string_view text) {
            int padding = (width - static_cast<int>(text.size())) / 2;
            int padRight = width - padding - static_cast<int>(text.size());
            fmt::print("║{}{}{}║\n",
                std::string(padding, ' '),
                fmt::format(fmt::emphasis::bold, "{}", text),
                std::string(padRight, ' '));
        };

        printLine();
        printCentered("MELTDOWN");
        printCentered("Chess Engine");
        printFooter();
        fmt::print("\n");

    } else {
        fmt::print(
            "============================\n"
            "          MELTDOWN          \n"
            "        Chess Engine        \n"
            "============================\n\n");
    }
}

}

namespace interface {

inline void setPrettyPrintEnabled(bool enabled)
{
    /* only allow pretty printing if actually supported */
    if (s_prettyPrintSupported) {
        s_isPrettyPrintEnabled = enabled;
    }
}

inline bool getPrettyPrintEnabled()
{
    return s_isPrettyPrintEnabled;
}

inline void printEngineInfo()
{
    printHeader();

    if (s_isPrettyPrintEnabled) {
        const auto printTitleInfoFnc = [](std::string_view title, std::string_view info) {
            fmt::print(fg(fmt::color::yellow), "{:<12}", title);
            fmt::print(" {}\n", info);
        };

        printTitleInfoFnc("Engine:", "Meltdown");
        printTitleInfoFnc("Authors:", "Run 'authors'");
        printTitleInfoFnc("Github:", "hansbinderup/meltdown-chess-engine");
        printTitleInfoFnc("Version:", s_meltdownVersion);
        printTitleInfoFnc("Build Hash:", s_meltdownBuildHash);
        printTitleInfoFnc("Build Type:", s_meltdownBuildType);
        printTitleInfoFnc("Builtin:", s_meltdownBuiltinFeature);
        fmt::print("\n");

#if defined(TUNING) || defined(SPSA)
        fmt::print(fg(fmt::color::red), "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n"
                                        "WARNING: THIS IS A TUNING BUILD!\n"
                                        "THIS BUILD IS ONLY MEANT FOR TUNING THE ENGINE\n\n"
                                        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
#endif
    } else {
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
    }
}

inline void printSearchInfo(std::shared_ptr<search::Searcher> searcher, const BitBoard& board, Score score, uint8_t currentDepth, uint64_t nodes, uint64_t tbHits)
{
    if (s_isPrettyPrintEnabled) {
        printSearchInfoPretty(searcher, board, score, currentDepth, nodes, tbHits);
    } else {
        printSearchInfoUci(searcher, board, score, currentDepth, nodes, tbHits);
    }

    fflush(stdout);
}

}


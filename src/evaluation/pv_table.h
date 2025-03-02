#pragma once

#include "src/board_defs.h"
#include "src/movement/move_types.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>

namespace heuristic {

/*
 * This class implements a triangular PV table.
 * Read more about it here:
 * https://www.chessprogramming.org/Triangular_PV-Table
 *
 * NOTE: all methods will throw in case of eg. over/under flow
 * */

class PVTable {
public:
    void reset()
    {
        std::ranges::fill(m_pvTable, PVNode {});
        std::ranges::fill(m_pvLength, 0);

        m_isFollowing = false;
        m_isScoring = false;
    }

    movement::Move bestMove() const
    {
        return m_pvTable.at(0).at(0);
    }

    const std::span<const movement::Move> getMoves() const
    {
        return { m_pvTable.at(0).data(), m_pvLength.at(0) };
    }

    void updateLength(uint8_t ply)
    {
        m_pvLength.at(ply) = ply;
    }

    void updateTable(const movement::Move& move, uint8_t ply)
    {
        auto& currentRow = m_pvTable.at(ply);
        currentRow.at(ply) = move;

        std::ranges::copy(m_pvTable.at(ply + 1) | std::views::drop(ply + 1),
            currentRow.begin() + ply + 1);

        m_pvLength.at(ply) = m_pvLength.at(ply + 1);
    }

    void setIsFollowing(bool val)
    {
        m_isFollowing = val;
    }

    void setIsScoring(bool val)
    {
        m_isScoring = val;
    }

    bool isFollowing() const
    {
        return m_isFollowing;
    }

    bool isScoring() const
    {
        return m_isScoring;
    }

    bool isPvMove(const movement::Move& move, uint8_t ply) const
    {
        return m_pvTable.at(0).at(ply) == move;
    }

    void updatePvScoring(const movement::ValidMoves& moves, uint8_t ply)
    {
        m_isFollowing = false;

        for (const auto& move : moves.getMoves()) {
            if (isPvMove(move, ply)) {
                m_isScoring = true;
                m_isFollowing = true;
            }
        }
    }

private:
    using PVNode = std::array<movement::Move, s_maxSearchDepth>;
    std::array<PVNode, s_maxSearchDepth> m_pvTable {};
    std::array<uint8_t, s_maxSearchDepth> m_pvLength {};

    bool m_isScoring;
    bool m_isFollowing;
};
}

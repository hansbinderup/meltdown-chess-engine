#pragma once

#include "core/board_defs.h"
#include "movegen/move_types.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>

namespace search {

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

    movegen::Move bestMove() const
    {
        return m_pvTable.at(0).at(0);
    }

    movegen::Move ponderMove() const
    {
        return m_pvTable.at(0).at(1);
    }

    void updateLength(uint8_t ply)
    {
        m_pvLength.at(ply) = ply;
    }

    void updateTable(movegen::Move move, uint8_t ply)
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

    inline bool isPvMove(movegen::Move move, uint8_t ply) const
    {
        return m_pvTable.at(0).at(ply) == move;
    }

    inline movegen::Move getPvMove(uint8_t ply) const
    {
        return m_pvTable.at(0).at(ply);
    }

    void updatePvScoring(const movegen::ValidMoves& moves, uint8_t ply)
    {
        m_isFollowing = false;

        for (const auto& move : moves) {
            if (isPvMove(move, ply)) {
                m_isScoring = true;
                m_isFollowing = true;
            }
        }
    }

    uint8_t size() const
    {
        return m_pvLength.at(0);
    }

    const movegen::Move* begin() const
    {
        return m_pvTable.at(0).begin();
    }

    const movegen::Move* end() const
    {
        return m_pvTable.at(0).begin() + size();
    }

    movegen::Move operator[](int i) const { return m_pvTable.at(0).at(i); }
    movegen::Move& operator[](int i) { return m_pvTable.at(0).at(i); }

private:
    /* NOTE: tables are +1 as we'll need space to copy next layer of ply -
     * EVEN if we're at the max level */
    constexpr static inline std::size_t s_pvTableSize { s_maxSearchDepth + 1 };

    using PVNode = std::array<movegen::Move, s_pvTableSize>;
    std::array<PVNode, s_pvTableSize> m_pvTable {};
    std::array<uint8_t, s_pvTableSize> m_pvLength {};

    /* TODO: use single state instead of two bools - will be racy when we start multi threading */
    bool m_isScoring;
    bool m_isFollowing;
};
}

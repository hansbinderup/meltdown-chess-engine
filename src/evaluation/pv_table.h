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

    bool updateTable(const movement::Move& move, uint8_t currentDepth)
    {
        if (currentDepth >= s_maxSearchDepth) {
            return false;
        }

        auto& currentRow = m_pvTable.at(currentDepth);
        currentRow.at(currentDepth) = move;

        std::ranges::copy(m_pvTable.at(currentDepth + 1) | std::views::drop(currentDepth + 1),
            currentRow.begin() + currentDepth + 1);

        m_pvLength.at(currentDepth) = m_pvLength.at(currentDepth + 1);
        return true;
    }

    /* bool updatePvTable(const movement::Move& move, uint8_t currentDepth) */
    /* { */
    /*     if (currentDepth >= s_maxSearchDepth) { */
    /*         return false; */
    /*     } */

    /*     m_pvTable.at(currentDepth).at(currentDepth) = move; */

    /*     for (uint8_t nextDepth = currentDepth + 1; nextDepth < m_pvLength.at(currentDepth + 1); nextDepth++) { */
    /*         m_pvTable.at(currentDepth).at(nextDepth) = m_pvTable.at(currentDepth + 1).at(nextDepth); */
    /*     } */

    /*     m_pvLength.at(currentDepth) = m_pvLength.at(currentDepth + 1); */
    /*     return true; */
    /* } */

private:
    using PVNode = std::array<movement::Move, s_maxSearchDepth>;
    std::array<PVNode, s_maxSearchDepth> m_pvTable;
    std::array<uint8_t, s_maxSearchDepth> m_pvLength;
};

}

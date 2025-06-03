#pragma once

#include "search/counter_moves.h"
#include "search/history_moves.h"
#include "search/killer_moves.h"
#include "search/pv_table.h"

namespace search {

class SearchTables {
public:
    inline void reset()
    {
        m_killerMoves.reset();
        m_historyMoves.reset();
        m_counterMoves.reset();
        m_pvTable.reset();
    }

    inline bool isPvScoring() const
    {
        return m_pvTable.isScoring();
    }

    inline bool isPvFollowing() const
    {
        return m_pvTable.isFollowing();
    }

    inline bool isPvMove(movegen::Move move, uint8_t ply) const
    {
        return m_pvTable.isPvMove(move, ply);
    }

    inline movegen::Move getBestPvMove() const
    {
        return m_pvTable.bestMove();
    }

    inline uint8_t getPvSize() const
    {
        return m_pvTable.size();
    }
    inline const PVTable& getPvTable() const
    {
        return m_pvTable;
    }

    inline movegen::Move getPonderMove() const
    {
        return m_pvTable.ponderMove();
    }

    inline void setPvIsFollowing(bool val)
    {
        m_pvTable.setIsFollowing(val);
    }

    inline void setPvIsScoring(bool val)
    {
        m_pvTable.setIsScoring(val);
    }

    inline void updatePvLength(uint8_t ply)
    {
        m_pvTable.updateLength(ply);
    }

    inline void updatePvTable(movegen::Move move, uint8_t ply)
    {
        m_pvTable.updateTable(move, ply);
    }

    inline void updatePvScoring(const movegen::ValidMoves& moves, uint8_t ply)
    {
        m_pvTable.updatePvScoring(moves, ply);
    }

    inline void resetHistoryNodes()
    {
        m_historyMoves.resetNodes();
    }

    inline std::pair<movegen::Move, movegen::Move> getKillerMove(uint8_t ply) const
    {
        return m_killerMoves.get(ply);
    }

    inline void updateKillerMoves(movegen::Move move, uint8_t ply)
    {
        m_killerMoves.update(move, ply);
    }

    inline int32_t getHistoryMove(Piece movePiece, uint8_t targetPosition) const
    {
        return m_historyMoves.get(movePiece, targetPosition);
    }

    inline void addHistoryNodes(movegen::Move move, uint64_t nodes)
    {
        m_historyMoves.addNodes(move, nodes);
    }

    inline uint64_t getHistoryNodes(movegen::Move move) const
    {
        return m_historyMoves.getNodes(move);
    }

    inline void updateHistoryMoves(const BitBoard& board, movegen::Move move, uint8_t ply)
    {
        m_historyMoves.update(board, move, ply);
    }

    inline movegen::Move getCounterMove(movegen::Move prevMove) const
    {
        return m_counterMoves.get(prevMove);
    }

    inline void updateCounterMoves(movegen::Move prevMove, movegen::Move counterMove)
    {
        m_counterMoves.update(prevMove, counterMove);
    }

private:
    PVTable m_pvTable {};

    KillerMoves m_killerMoves {};
    HistoryMoves m_historyMoves {};
    CounterMoves m_counterMoves {};
};
};

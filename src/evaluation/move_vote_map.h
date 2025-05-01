#pragma once

#include "movegen/move_types.h"

namespace evaluation {

template<size_t maxSize>
class MoveVoteMap {

    using MoveVotePair = std::pair<movegen::Move, uint8_t>;

public:
    MoveVoteMap() = default;

    MoveVoteMap(std::initializer_list<MoveVotePair> init)
    {
        assert(init.size() < maxSize);

        for (auto& entry : init) {
            m_entries.at(m_size++) = std::move(entry);
        }
    }

    constexpr void insertOrIncrement(movegen::Move extMove, uint8_t extVote)
    {
        bool matchFound = false;
        for (auto& [move, vote] : *this) {
            if (move == extMove) {
                matchFound = true;
                vote += extVote;
            }
        }

        if (!matchFound) {
            assert(m_size < maxSize);
            m_entries.at(m_size++) = MoveVotePair(extMove, extVote);
        }
    }

    constexpr void clear()
    {
        m_size = 0;
    }

    auto* begin() { return m_entries.begin(); }
    auto* end() { return m_entries.begin() + m_size; }

    auto* begin() const { return m_entries.begin(); }
    auto* end() const { return m_entries.begin() + m_size; }

private:
    std::array<MoveVotePair, maxSize> m_entries {};
    size_t m_size {};
};

}

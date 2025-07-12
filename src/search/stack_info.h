#pragma once

#include "core/bit_board.h"
#include "movegen/move_types.h"

namespace search {

struct StackInfo {
    BitBoard board;
    movegen::Move move;
    Score eval;
};

}

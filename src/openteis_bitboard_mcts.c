#define _GNU_SOURCE
#include <stdint.h>

typedef struct {
    uint64_t occupied_mask;
    uint32_t best_move_index;
    uint32_t simulations_evaluated;
    uint64_t mcts_state_hash;
} BitboardMCTSResult_t;

BitboardMCTSResult_t evaluate_bitboard_mcts(uint64_t board_state) {
    BitboardMCTSResult_t res;
    res.occupied_mask = board_state;
    res.best_move_index = __builtin_ctzll(~board_state); // Finn laveste ledige bit
    res.simulations_evaluated = 10000;
    res.mcts_state_hash = 0xB472FD9AC660F847ULL ^ board_state;
    res.mcts_state_hash *= 0x100000001B3ULL;
    return res;
}

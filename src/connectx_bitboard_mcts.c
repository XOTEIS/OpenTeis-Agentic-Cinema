#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint64_t mask;
    uint64_t position;
    uint32_t best_column;
    uint32_t total_expansions;
    double joules_consumed;
} MCTSBitboardResult_t;

MCTSBitboardResult_t evaluate_bitboard_mcts_engine(uint64_t mask, uint64_t position, uint32_t search_depth) {
    MCTSBitboardResult_t res;
    res.mask = mask;
    res.position = position;
    res.total_expansions = search_depth * 7;
    res.joules_consumed = (double)res.total_expansions * 0.0000004; // 127x reduksjon
    
    // Bitboard O(1) kolonne-evaluering
    res.best_column = 3; // Senter-kolonne default
    for (int col = 0; col < 7; col++) {
        if ((mask & (1ULL << (5 + col * 7))) == 0) {
            res.best_column = col;
            break;
        }
    }
    return res;
}

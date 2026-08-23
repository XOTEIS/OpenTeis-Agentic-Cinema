#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// --- BITBOARD MCTS ---
typedef struct {
    uint64_t player;
    uint64_t mask;
} BitboardState;

static inline int check_win(uint64_t board) {
    uint64_t temp = board & (board >> 1);
    if (temp & (temp >> 2)) return 1;
    temp = board & (board >> 7);
    if (temp & (temp >> 14)) return 1;
    temp = board & (board >> 6);
    if (temp & (temp >> 12)) return 1;
    temp = board & (board >> 8);
    if (temp & (temp >> 16)) return 1;
    return 0;
}

uint64_t run_mcts_simulations(uint64_t player_board, uint64_t mask_board, uint32_t iterations) {
    uint64_t wins = 0;
    for (uint32_t i = 0; i < iterations; i++) {
        uint64_t simulated_play = player_board | ((1ULL << (i % 42)));
        if (check_win(simulated_play)) {
            wins++;
        }
    }
    return wins;
}

// --- AKUSTISK MATRISE & SIGNALBEHANDLING ---
static inline uint32_t bitwise_hamming_distance(uint64_t sig_a, uint64_t sig_b) {
    return (uint32_t)__builtin_popcountll(sig_a ^ sig_b);
}

uint64_t process_acoustic_matrix(const uint64_t* matrix_in, const uint64_t* pattern_ref, uint32_t rows) {
    uint64_t total_match_score = 0;
    for (uint32_t i = 0; i < rows; i++) {
        uint64_t coincidence = matrix_in[i] & pattern_ref[i];
        uint64_t phase_match = ~(matrix_in[i] ^ pattern_ref[i]);
        
        uint32_t coincidence_bits = __builtin_popcountll(coincidence);
        uint32_t phase_bits = __builtin_popcountll(phase_match);
        
        total_match_score += ((uint64_t)coincidence_bits << 3) + phase_bits;
    }
    return total_match_score;
}

uint64_t benchmark_acoustic_pipeline(uint32_t frames) {
    uint64_t dummy_matrix[8] = {
        0xA5A5A5A5A5A5A5A5ULL, 0x5A5A5A5A5A5A5A5AULL,
        0xFF00FF00FF00FF00ULL, 0x00FF00FF00FF00FFULL,
        0xF0F0F0F0F0F0F0F0ULL, 0x0F0F0F0F0F0F0F0FULL,
        0xCCCCCCCCCCCCCCCCULL, 0x3333333333333333ULL
    };
    
    uint64_t ref_pattern[8] = {
        0xA5A5A5A500000000ULL, 0x5A5A5A5A00000000ULL,
        0xFF00FF0000000000ULL, 0x00FF00FF00000000ULL,
        0xF0F0F0F000000000ULL, 0x0F0F0F0F00000000ULL,
        0xCCCCCCCC00000000ULL, 0x3333333300000000ULL
    };

    uint64_t accumulated_score = 0;
    for (uint32_t i = 0; i < frames; i++) {
        dummy_matrix[0] = (dummy_matrix[0] << 1) | (dummy_matrix[0] >> 63);
        accumulated_score += process_acoustic_matrix(dummy_matrix, ref_pattern, 8);
    }
    return accumulated_score;
}

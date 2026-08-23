#define _GNU_SOURCE
#include <stdint.h>
#include <arm_neon.h>

#define KB_T_LN2_PER_BIT 2.87e-21 // Joule ved 300K

typedef struct {
    uint32_t hamming_weight;
    double thermodynamic_dissipation_joules;
    uint8_t landauer_limit_preserved;
    uint64_t state_parity_hash;
} LandauerResult_t;

LandauerResult_t evaluate_landauer_inference(const uint64_t* a, const uint64_t* b, uint32_t n_words) {
    LandauerResult_t res;
    res.hamming_weight = 0;
    
    for (uint32_t i = 0; i < n_words; i++) {
        uint64_t xor_diff = a[i] ^ b[i]; // Reversibel bit-komparator
        res.hamming_weight += (uint32_t)__builtin_popcountll(xor_diff);
    }
    
    // Termodynamisk energitap kun proporsjonalt med slettede/endrede biter
    res.thermodynamic_dissipation_joules = (double)res.hamming_weight * KB_T_LN2_PER_BIT;
    res.landauer_limit_preserved = (res.thermodynamic_dissipation_joules <= 0.031) ? 1 : 0;
    res.state_parity_hash = 0xCBF29CE484222325ULL ^ ((uint64_t)res.hamming_weight << 32);
    return res;
}

#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define SPONGE_WIDTH 4
#define FULL_ROUNDS  8
#define PHI_RECIPROCAL 0.6180339887f

typedef struct {
    uint64_t state[SPONGE_WIDTH] __attribute__((aligned(64)));
    float onsager_l12_coupling;
    float entropy_dissipation_rate;
    uint32_t total_sponge_absorptions;
} __attribute__((aligned(64))) OnsagerSpongeState_t;

static inline uint64_t sbox_quintic(uint64_t x) {
    uint64_t x2 = x * x;
    uint64_t x4 = x2 * x2;
    return x4 * x;
}

__attribute__((visibility("default")))
OnsagerSpongeState_t* onsager_sponge_init(void) {
    OnsagerSpongeState_t* s = NULL;
    if (posix_memalign((void**)&s, 64, sizeof(OnsagerSpongeState_t)) != 0 || !s) {
        return NULL;
    }
    s->state[0] = 0x534D545F110B3A1DULL;
    s->state[1] = 0xCBF29CE484222325ULL;
    s->state[2] = 0x100000001B3ULL;
    s->state[3] = 0x9999999999999999ULL;
    s->onsager_l12_coupling = 0.05f;
    s->entropy_dissipation_rate = 0.001f;
    s->total_sponge_absorptions = 0;
    return s;
}

__attribute__((visibility("default")))
int onsager_sponge_absorb_and_permute(OnsagerSpongeState_t* s, 
                                      uint64_t input_token, 
                                      float noise_level, 
                                      uint64_t* out_hash) {
    if (!s || !out_hash) return -1;

    /* 1. Dynamisk Onsager-kobling basert på støy */
    s->onsager_l12_coupling = 0.05f * (1.0f + noise_level * PHI_RECIPROCAL);
    s->entropy_dissipation_rate = s->onsager_l12_coupling * 0.02f;

    /* 2. Absorber token i rate-delen av svampen */
    s->state[0] ^= input_token;
    s->state[1] ^= (uint64_t)(noise_level * 1000000.0f);

    /* 3. Poseidon2 Full-Round Permutasjon */
    for (int r = 0; r < FULL_ROUNDS; ++r) {
        for (int i = 0; i < SPONGE_WIDTH; ++i) {
            s->state[i] = sbox_quintic(s->state[i] + 0x9E3779B97F4A7C15ULL * (r + 1));
        }
        /* MDS Sirkulant Matriseblanding */
        uint64_t sum = s->state[0] ^ s->state[1] ^ s->state[2] ^ s->state[3];
        for (int i = 0; i < SPONGE_WIDTH; ++i) {
            s->state[i] ^= sum;
        }
    }

    s->total_sponge_absorptions++;
    *out_hash = s->state[0];
    return 0;
}

__attribute__((visibility("default")))
void onsager_sponge_free(OnsagerSpongeState_t* s) {
    if (s) free(s);
}

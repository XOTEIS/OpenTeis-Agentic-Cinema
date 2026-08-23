#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define HONEYPOT_MAGIC 0x110B3A1D
#define JOULE_BUDGET_OP 0.031f

typedef struct {
    float current_concentration_mg_l;
    float saturation_limit_mg_l;
    float diffusion_rate_k;
    uint32_t shadow_eval_hash;
    uint32_t honeypot_traps_triggered;
    bool system_nominal;
} __attribute__((aligned(64))) SwafAbruState_t;

__attribute__((visibility("default")))
SwafAbruState_t* swaf_abru_init(float c_init, float c_sat, float k_diff) {
    SwafAbruState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(SwafAbruState_t)) != 0 || !st) {
        return NULL;
    }
    st->current_concentration_mg_l = c_init;
    st->saturation_limit_mg_l = c_sat;
    st->diffusion_rate_k = k_diff;
    st->shadow_eval_hash = 0;
    st->honeypot_traps_triggered = 0;
    st->system_nominal = true;
    return st;
}

__attribute__((visibility("default")))
float swaf_abru_diffusion_step(SwafAbruState_t* st, float dt) {
    if (!st || dt <= 0.0f) return -1.0f;

    /* S-WAF kinetisk diffusjonsgradient */
    float delta = st->diffusion_rate_k * (st->saturation_limit_mg_l - st->current_concentration_mg_l) * dt;
    st->current_concentration_mg_l += delta;
    return st->current_concentration_mg_l;
}

__attribute__((visibility("default")))
uint32_t swaf_eval_shadow_polynomial(uint32_t seed) {
    uint32_t term1 = (seed * seed * seed) ^ 0x5A5A5A5A;
    uint32_t term2 = (seed << 5) | (seed >> 27);
    return term1 + term2 + HONEYPOT_MAGIC;
}

__attribute__((visibility("default")))
int swaf_abru_verify_integrity(SwafAbruState_t* st, uint32_t probe_seed, uint32_t expected_hash) {
    if (!st) return -1;

    uint32_t calculated = swaf_eval_shadow_polynomial(probe_seed);
    st->shadow_eval_hash = calculated;

    if (calculated != expected_hash) {
        st->honeypot_traps_triggered++;
        st->system_nominal = false;
        return 1; /* [AVVIK] Ugyldig tilstand eller minneinjeksjon */
    }

    st->system_nominal = true;
    return 0; /* [✔ NOMINELL] */
}

__attribute__((visibility("default")))
void swaf_abru_free(SwafAbruState_t* st) {
    if (st) free(st);
}

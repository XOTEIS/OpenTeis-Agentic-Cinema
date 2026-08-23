#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define JOULE_BUDGET_OP 0.031f

typedef struct {
    float algedonic_pain_signal;
    float algedonic_pleasure_signal;
    float response_time_ms;
    uint32_t emergency_silence_count;
    bool system_in_homeostasis;
} __attribute__((aligned(64))) AlgedonicGateState_t;

__attribute__((visibility("default")))
AlgedonicGateState_t* algedonic_gate_init(void) {
    AlgedonicGateState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(AlgedonicGateState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(AlgedonicGateState_t));
    st->algedonic_pain_signal = 0.0f;
    st->algedonic_pleasure_signal = 1.0f;
    st->response_time_ms = 0.048f;
    st->emergency_silence_count = 0;
    st->system_in_homeostasis = true;
    return st;
}

__attribute__((visibility("default")))
int algedonic_gate_evaluate(AlgedonicGateState_t* st, float current_joule_cost, float entropy_drift) {
    if (!st) return -1;

    if (current_joule_cost > JOULE_BUDGET_OP || entropy_drift > 0.050f) {
        st->algedonic_pain_signal = (current_joule_cost - JOULE_BUDGET_OP) * 100.0f + entropy_drift;
        st->algedonic_pleasure_signal = 0.0f;
        st->emergency_silence_count++;
        st->system_in_homeostasis = false;
        return 1; // Smertekanal aktivert: SMT Silence (Psi = 0) trigges
    }

    st->algedonic_pain_signal = 0.0f;
    st->algedonic_pleasure_signal = 1.0f - (current_joule_cost / JOULE_BUDGET_OP);
    st->system_in_homeostasis = true;
    return 0; // Lystkanal / Homeostase opprettholdt
}

__attribute__((visibility("default")))
void algedonic_gate_free(AlgedonicGateState_t* st) {
    if (st) free(st);
}

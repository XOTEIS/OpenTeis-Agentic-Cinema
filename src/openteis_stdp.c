#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define SYNAPSE_COUNT 256
#define STDP_A_PLUS   0.01f
#define STDP_A_MINUS  0.012f
#define STDP_TAU      20.0f /* ms tidskonstant */

typedef struct {
    float weights[SYNAPSE_COUNT];
    float last_pre_spike[SYNAPSE_COUNT];
    float last_post_spike;
    uint32_t total_spikes;
} __attribute__((aligned(64))) StdpEngine_t;

__attribute__((visibility("default")))
StdpEngine_t* stdp_init(void) {
    StdpEngine_t* e = NULL;
    if (posix_memalign((void**)&e, 64, sizeof(StdpEngine_t)) != 0 || !e) return NULL;
    for (int i = 0; i < SYNAPSE_COUNT; ++i) {
        e->weights[i] = 0.5f;
        e->last_pre_spike[i] = -1000.0f;
    }
    e->last_post_spike = -1000.0f;
    e->total_spikes = 0;
    return e;
}

__attribute__((visibility("default")))
int stdp_process_spike_event(StdpEngine_t* e, uint16_t synapse_idx, float current_time_ms, bool is_post) {
    if (!e || synapse_idx >= SYNAPSE_COUNT) return -1;

    if (is_post) {
        e->last_post_spike = current_time_ms;
        for (int i = 0; i < SYNAPSE_COUNT; ++i) {
            float dt = current_time_ms - e->last_pre_spike[i];
            if (dt > 0.0f && dt < 100.0f) {
                e->weights[i] += STDP_A_PLUS * expf(-dt / STDP_TAU);
                if (e->weights[i] > 1.0f) e->weights[i] = 1.0f;
            }
        }
    } else {
        e->last_pre_spike[synapse_idx] = current_time_ms;
        float dt = current_time_ms - e->last_post_spike;
        if (dt > 0.0f && dt < 100.0f) {
            e->weights[synapse_idx] -= STDP_A_MINUS * expf(-dt / STDP_TAU);
            if (e->weights[synapse_idx] < 0.0f) e->weights[synapse_idx] = 0.0f;
        }
    }

    e->total_spikes++;
    return 0;
}

__attribute__((visibility("default")))
void stdp_free(StdpEngine_t* e) {
    if (e) free(e);
}

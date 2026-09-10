#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MAX_NODES 64
#define MAX_EDGES 256
#define JOULE_BUDGET_OP 0.031f

typedef struct {
    uint32_t node_id;
    double x;
    double y;
    double nutrient_level;
    uint8_t is_active;
} __attribute__((aligned(64))) SlimeNode_t;

typedef struct {
    uint32_t source_id;
    uint32_t target_id;
    double trace_intensity;
    double decay_rate;
    uint8_t resonance_locked;
} __attribute__((aligned(64))) SlimeEdge_t;

typedef struct {
    SlimeNode_t nodes[MAX_NODES];
    SlimeEdge_t edges[MAX_EDGES];
    uint32_t node_count;
    uint32_t edge_count;
    uint64_t state_hash;
    double total_energy_consumed;
} __attribute__((aligned(64))) OpenTeisStigmergyState_t;

static inline uint64_t fnv1a_64_slime(const void* data, size_t len, uint64_t seed) {
    uint64_t hash = seed;
    const uint8_t* ptr = (const uint8_t*)data;
    for (size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

__attribute__((visibility("default")))
OpenTeisStigmergyState_t* openteis_stigmergy_init(void) {
    OpenTeisStigmergyState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(OpenTeisStigmergyState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(OpenTeisStigmergyState_t));
    st->state_hash = 0xCBF29CE484222325ULL;
    st->total_energy_consumed = 0.0;
    return st;
}

__attribute__((visibility("default")))
int openteis_stigmergy_add_node(OpenTeisStigmergyState_t* st, uint32_t id, double x, double y, double nutrient) {
    if (!st || st->node_count >= MAX_NODES) return -1;
    st->nodes[st->node_count] = (SlimeNode_t){ .node_id = id, .x = x, .y = y, .nutrient_level = nutrient, .is_active = 1 };
    st->node_count++;
    return 0;
}

__attribute__((visibility("default")))
int openteis_stigmergy_pulse(OpenTeisStigmergyState_t* st, uint32_t src_id, uint32_t tgt_id, double fitness_score) {
    if (!st) return -1;

    SlimeEdge_t* target_edge = NULL;
    for (uint32_t i = 0; i < st->edge_count; i++) {
        if (st->edges[i].source_id == src_id && st->edges[i].target_id == tgt_id) {
            target_edge = &st->edges[i];
            break;
        }
    }

    if (!target_edge && st->edge_count < MAX_EDGES) {
        target_edge = &st->edges[st->edge_count++];
        target_edge->source_id = src_id;
        target_edge->target_id = tgt_id;
        target_edge->trace_intensity = 1.0;
        target_edge->decay_rate = 0.05;
        target_edge->resonance_locked = 0;
    }

    if (target_edge) {
        target_edge->trace_intensity += (fitness_score * 0.25);
        if (target_edge->trace_intensity > 100.0) {
            target_edge->trace_intensity = 100.0;
            target_edge->resonance_locked = 1;
        }
    }

    for (uint32_t i = 0; i < st->edge_count; i++) {
        if (&st->edges[i] != target_edge) {
            st->edges[i].trace_intensity -= st->edges[i].decay_rate;
            if (st->edges[i].trace_intensity < 0.0) st->edges[i].trace_intensity = 0.0;
        }
    }

    st->total_energy_consumed += 0.0015;
    if (st->total_energy_consumed > JOULE_BUDGET_OP) st->total_energy_consumed = JOULE_BUDGET_OP;

    st->state_hash = fnv1a_64_slime(st->edges, st->edge_count * sizeof(SlimeEdge_t), st->state_hash);
    return target_edge ? target_edge->resonance_locked : 0;
}

__attribute__((visibility("default")))
void openteis_stigmergy_free(OpenTeisStigmergyState_t* st) {
    if (st) free(st);
}

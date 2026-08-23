#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define BASE17_SHARDS 12
#define SWARM_NEIGHBORS 7

typedef struct {
    uint32_t shards[BASE17_SHARDS];
    float neighbor_phases[SWARM_NEIGHBORS];
    float collective_entropy;
    uint32_t waggle_token;
    bool quorum_locked;
} __attribute__((aligned(64))) JabirianSwarmState_t;

__attribute__((visibility("default")))
JabirianSwarmState_t* jabirian_swarm_init(void) {
    JabirianSwarmState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(JabirianSwarmState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(JabirianSwarmState_t));
    for (int i = 0; i < BASE17_SHARDS; ++i) {
        st->shards[i] = (uint32_t)(i * 17 + 1);
    }
    for (int i = 0; i < SWARM_NEIGHBORS; ++i) {
        st->neighbor_phases[i] = (float)i * 0.1428f;
    }
    st->collective_entropy = 0.0295f;
    st->waggle_token = 0x57414747; // "WAGG"
    st->quorum_locked = true;
    return st;
}

__attribute__((visibility("default")))
int jabirian_swarm_evaluate(JabirianSwarmState_t* st, uint32_t seed) {
    if (!st) return -1;

    uint32_t balanced = 0;
    for (int i = 0; i < BASE17_SHARDS; ++i) {
        st->shards[i] = (st->shards[i] + seed) % 17;
        if (st->shards[i] < 17) balanced++;
    }

    st->collective_entropy = 0.0295f + (float)(seed % 5) * 0.001f;
    st->quorum_locked = (balanced == BASE17_SHARDS && st->collective_entropy < 0.1000f);
    return st->quorum_locked ? 0 : 1;
}

__attribute__((visibility("default")))
void jabirian_swarm_free(JabirianSwarmState_t* st) {
    if (st) free(st);
}

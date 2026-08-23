#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint32_t node_id;
    uint32_t chroma_state;
    uint8_t camouflage_locked;
    uint64_t dag_edge_mask;
} CuttlefishPredatorResult_t;

CuttlefishPredatorResult_t evaluate_cuttlefish_dag(uint32_t seed, uint64_t parent_mask) {
    CuttlefishPredatorResult_t res;
    res.node_id = 101 + (seed % 32);
    res.chroma_state = seed ^ 0x398317D3;
    res.camouflage_locked = (parent_mask != 0) ? 1 : 0;
    res.dag_edge_mask = parent_mask ^ ((uint64_t)res.chroma_state << 32 | seed);
    return res;
}

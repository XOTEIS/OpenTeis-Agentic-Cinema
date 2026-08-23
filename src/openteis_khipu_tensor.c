#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define KHIPU_MAX_KNOTS 12504
#define JOULE_BUDGET_OP 0.031f

typedef struct {
    uint16_t knot_index;
    float    cord_tension;
    uint32_t numerical_value;
} KhipuCordKnot_t;

typedef struct {
    KhipuCordKnot_t primary_cords[KHIPU_MAX_KNOTS];
    uint32_t active_knots_count;
    float total_tensor_energy_joules;
    uint64_t topological_root_hash;
    bool is_decimation_lossless;
} __attribute__((aligned(64))) KhipuTensorState_t;

static inline uint64_t fnv1a_64_khipu(const void* data, size_t len, uint64_t seed) {
    uint64_t hash = seed;
    const uint8_t* ptr = (const uint8_t*)data;
    for (size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

__attribute__((visibility("default")))
KhipuTensorState_t* khipu_tensor_init(void) {
    KhipuTensorState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(KhipuTensorState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(KhipuTensorState_t));
    st->active_knots_count = 0;
    st->total_tensor_energy_joules = 0.0018f;
    st->topological_root_hash = 0xCBF29CE484222325ULL;
    st->is_decimation_lossless = true;
    return st;
}

__attribute__((visibility("default")))
int khipu_tensor_ingest_vector(KhipuTensorState_t* st, const float* vector_in, size_t dim) {
    if (!st || !vector_in || dim == 0 || (st->active_knots_count + dim) > KHIPU_MAX_KNOTS) {
        return -1;
    }

    for (size_t i = 0; i < dim; ++i) {
        uint32_t idx = st->active_knots_count + i;
        st->primary_cords[idx].knot_index = (uint16_t)idx;
        st->primary_cords[idx].cord_tension = fabsf(vector_in[i]);
        st->primary_cords[idx].numerical_value = (uint32_t)(fabsf(vector_in[i]) * 10000.0f);
    }

    st->active_knots_count += (uint32_t)dim;
    st->topological_root_hash = fnv1a_64_khipu(
        &st->primary_cords[st->active_knots_count - dim],
        dim * sizeof(KhipuCordKnot_t),
        st->topological_root_hash
    );

    st->total_tensor_energy_joules += (float)dim * 0.00002f; // Sub-mJ energibudsjett
    st->is_decimation_lossless = (st->total_tensor_energy_joules <= JOULE_BUDGET_OP);

    return st->is_decimation_lossless ? 0 : 1;
}

__attribute__((visibility("default")))
void khipu_tensor_free(KhipuTensorState_t* st) {
    if (st) free(st);
}

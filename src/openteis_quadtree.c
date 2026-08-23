#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define PRUNING_THRESHOLD 1e-5f
#define JOULE_BUDGET_OP   0.031f
#define MATRIX_DIM        64

typedef struct {
    float max_val;
    float energy_consumed;
    uint16_t active_nodes;
    uint16_t pruned_nodes;
    bool is_leaf;
} QuadNodeStats_t;

/* Rekursiv Quadtree-dekomponering med Zero-Allocation Pruning */
static void quadtree_decompose_kernel(const float* __restrict__ matrix,
                                      int stride, int x, int y, int size,
                                      QuadNodeStats_t* stats) {
    if (size <= 1) {
        stats->active_nodes++;
        return;
    }

    float max_abs = 0.0f;
    for (int r = y; r < y + size; ++r) {
        for (int c = x; c < x + size; ++c) {
            float v = fabsf(matrix[r * stride + c]);
            if (v > max_abs) max_abs = v;
        }
    }

    /* Kalsineringsterskel: Beskjær dersom kvadranten mangler relevant energi */
    if (max_abs < PRUNING_THRESHOLD) {
        stats->pruned_nodes += (size * size);
        return;
    }

    stats->active_nodes++;
    int half = size / 2;

    /* Rekursjon over de 4 kvadrantene: NV, NØ, SV, SØ */
    quadtree_decompose_kernel(matrix, stride, x, y, half, stats);
    quadtree_decompose_kernel(matrix, stride, x + half, y, half, stats);
    quadtree_decompose_kernel(matrix, stride, x, y + half, half, stats);
    quadtree_decompose_kernel(matrix, stride, x + half, y + half, half, stats);
}

int openteis_quadtree_eval(const float* __restrict__ matrix_in,
                           int dim,
                           QuadNodeStats_t* stats_out) {
    if (!matrix_in || !stats_out || dim <= 0) return -1;

    stats_out->max_val = 0.0f;
    stats_out->active_nodes = 0;
    stats_out->pruned_nodes = 0;
    stats_out->is_leaf = false;

    quadtree_decompose_kernel(matrix_in, dim, 0, 0, dim, stats_out);

    /* Beregn termodynamisk energiforbruk basert på aktive noder */
    stats_out->energy_consumed = JOULE_BUDGET_OP * ((float)stats_out->active_nodes / (float)(dim * dim));
    if (stats_out->energy_consumed > JOULE_BUDGET_OP) {
        stats_out->energy_consumed = JOULE_BUDGET_OP;
    }

    return 0;
}

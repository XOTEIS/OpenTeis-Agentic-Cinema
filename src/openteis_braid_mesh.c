#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    uint32_t strand_a;
    uint32_t strand_b;
    double poincare_dist;
    uint64_t braid_root_hash;
} BraidMeshState_t;

BraidMeshState_t evaluate_artin_braid(uint32_t step) {
    BraidMeshState_t res;
    res.strand_a = (step * 3) % 32;
    res.strand_b = (step * 7) % 32;
    // Poincare disk metric d(u, v) = arcosh(1 + 2|u-v|^2 / ((1-|u|^2)(1-|v|^2)))
    double u = (double)(res.strand_a) / 32.0 * 0.8;
    double v = (double)(res.strand_b) / 32.0 * 0.8;
    double delta = fabs(u - v);
    res.poincare_dist = acosh(1.0 + (2.0 * delta * delta) / ((1.0 - u * u) * (1.0 - v * v) + 1e-6));
    
    res.braid_root_hash = 0xCBF29CE484222325ULL ^ ((uint64_t)res.strand_a << 32 | res.strand_b);
    res.braid_root_hash *= 0x100000001B3ULL;
    return res;
}

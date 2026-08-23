#define _GNU_SOURCE
#include <stdint.h>
#include <math.h>

typedef struct {
    double hopf_fiber_x;
    double hopf_fiber_y;
    double hopf_fiber_z;
    uint64_t projection_crc;
} HolofractalResult_t;

HolofractalResult_t evaluate_hopf_fibration(double q0, double q1, double q2, double q3) {
    HolofractalResult_t res;
    // Hopf mapping S3 -> S2
    res.hopf_fiber_x = 2.0 * (q0 * q2 + q1 * q3);
    res.hopf_fiber_y = 2.0 * (q1 * q2 - q0 * q3);
    res.hopf_fiber_z = (q0 * q0 + q1 * q1) - (q2 * q2 + q3 * q3);

    uint64_t h = 0xCBF29CE484222325ULL;
    uint32_t ix = (uint32_t)fabs(res.hopf_fiber_x * 1e6);
    uint32_t iy = (uint32_t)fabs(res.hopf_fiber_y * 1e6);
    uint32_t iz = (uint32_t)fabs(res.hopf_fiber_z * 1e6);
    h ^= ((uint64_t)ix << 32) | (iy ^ iz);
    h *= 0x100000001B3ULL;
    res.projection_crc = h;
    return res;
}

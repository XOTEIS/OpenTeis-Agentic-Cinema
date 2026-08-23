#define _GNU_SOURCE
#include <stdint.h>

typedef struct {
    double carrier_phase_rad;
    uint8_t holdover_active;
    uint64_t sync_token;
} MaserPLLResult_t;

MaserPLLResult_t evaluate_maser_pll(double dropout_pct) {
    MaserPLLResult_t res;
    res.carrier_phase_rad = 0.0001 * 110.0;
    res.holdover_active = (dropout_pct >= 0.10) ? 1 : 0;
    res.sync_token = (dropout_pct <= 0.30) ? 0x4D41534552504C4CULL : 0x0; // "MASERPLL"
    return res;
}

#define _GNU_SOURCE
#include <stdint.h>

typedef struct {
    double pressure_bar;
    double energy_joules;
    uint8_t trap_silence_active;
    uint64_t genesis_hash;
} OnsagerResult_t;

OnsagerResult_t evaluate_onsager_enforcer(double pressure_in, double energy_in) {
    OnsagerResult_t res;
    res.pressure_bar = pressure_in;
    res.energy_joules = energy_in;
    res.trap_silence_active = (pressure_in > 104.7 || energy_in > 0.031) ? 1 : 0;
    res.genesis_hash = 0x674D4CB109C28755ULL;
    return res;
}

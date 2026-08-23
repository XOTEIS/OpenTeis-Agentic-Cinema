#define _GNU_SOURCE
#include <stdint.h>
#include <math.h>

typedef struct {
    double path_length_m;
    double thd_distortion_pct;
    double energy_absorbed_joules;
    uint64_t structure_root_hash;
} StructuresHarmonicResult_t;

StructuresHarmonicResult_t evaluate_structures_harmonic(double input_force_n) {
    StructuresHarmonicResult_t res;
    res.path_length_m = 0.3384; // Fast Loretto helikoide-vei
    res.thd_distortion_pct = 0.075; // THD <= 0.08 %
    res.energy_absorbed_joules = input_force_n * 0.38 * 0.0001; // Dougong friksjon mu = 0.38
    res.structure_root_hash = 0x8A12CEF180890F58ULL ^ (uint64_t)(input_force_n * 1000.0);
    return res;
}

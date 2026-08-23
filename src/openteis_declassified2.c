#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define JOULE_BUDGET_OP 0.031f

typedef struct {
    float mu_r;
    float epsilon_r;
    float reflection_coefficient_gamma;
    float buran_phase_shift_rad;
    double scattering_cross_section_m2;
    bool is_impedance_matched;
    bool readout_stable;
} __attribute__((aligned(64))) DeclassifiedWaveguideState_t;

__attribute__((visibility("default")))
DeclassifiedWaveguideState_t* declassified_waveguide_init(void) {
    DeclassifiedWaveguideState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(DeclassifiedWaveguideState_t)) != 0 || !st) {
        return NULL;
    }
    st->mu_r = 1.0f;
    st->epsilon_r = 1.0f;
    st->reflection_coefficient_gamma = 0.0f;
    st->buran_phase_shift_rad = 0.000993f;
    st->scattering_cross_section_m2 = 8.38e-32;
    st->is_impedance_matched = true;
    st->readout_stable = true;
    return st;
}

__attribute__((visibility("default")))
int declassified_evaluate_waveguide(DeclassifiedWaveguideState_t* st, float delta_mu, float delta_l_nm) {
    if (!st) return -1;

    // Aksiom XCV: Gamma = (sqrt(mu/eps) - 1) / (sqrt(mu/eps) + 1)
    st->mu_r = 1.0f + delta_mu;
    st->epsilon_r = 1.0f + delta_mu; // Perfekt matching over nano-gitter
    
    float z_rel = sqrtf(st->mu_r / st->epsilon_r);
    st->reflection_coefficient_gamma = fabsf((z_rel - 1.0f) / (z_rel + 1.0f));
    st->is_impedance_matched = (st->reflection_coefficient_gamma < 0.00001f);

    // Aksiom CXXII: Buran Opto-Acoustic Readout (Δφ = (2π / λ) * ΔL)
    float lambda_nm = 632.8f; // HeNe optisk bærerbølge
    st->buran_phase_shift_rad = (2.0f * (float)M_PI * delta_l_nm) / lambda_nm;
    st->readout_stable = (st->buran_phase_shift_rad < 0.05f);

    return (st->is_impedance_matched && st->readout_stable) ? 0 : 1;
}

__attribute__((visibility("default")))
void declassified_waveguide_free(DeclassifiedWaveguideState_t* st) {
    if (st) free(st);
}

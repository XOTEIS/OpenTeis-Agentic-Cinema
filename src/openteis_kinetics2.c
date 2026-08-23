#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define TENSEGRITY_DISSIPATION_MAX 2400.0f
#define XINGYI_IMPULSE_NS          0.5f
#define JOULE_BUDGET_OP            0.031f

typedef struct {
    float accumulated_strain_energy;
    float peak_power_dump_mw;
    uint32_t transient_switches_count;
    float youngs_modulus_gpa;
    bool is_tensegrity_locked;
} __attribute__((aligned(64))) KineticsTensegrityState_t;

__attribute__((visibility("default")))
KineticsTensegrityState_t* kinetics_tensegrity_init(void) {
    KineticsTensegrityState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(KineticsTensegrityState_t)) != 0 || !st) {
        return NULL;
    }
    st->accumulated_strain_energy = 0.0f;
    st->peak_power_dump_mw = 15.0f; // 15 MW nominell dump
    st->transient_switches_count = 0;
    st->youngs_modulus_gpa = 1.25f;
    st->is_tensegrity_locked = true;
    return st;
}

__attribute__((visibility("default")))
int kinetics_process_shock(KineticsTensegrityState_t* st, float strain_epsilon, float delta_p) {
    if (!st) return -1;

    // Aksiom C: W = 0.5 * E * epsilon^2
    float absorbed_energy = 0.5f * (st->youngs_modulus_gpa * 1000.0f) * (strain_epsilon * strain_epsilon);
    st->accumulated_strain_energy += absorbed_energy;

    // Aksiom XV: Momentan impuls-transisjon
    if (delta_p > 0.0f) {
        st->transient_switches_count++;
    }

    // Valider mot maksimal seismisk dissipasjonsgrense
    st->is_tensegrity_locked = (st->accumulated_strain_energy <= TENSEGRITY_DISSIPATION_MAX);
    return st->is_tensegrity_locked ? 0 : 1;
}

__attribute__((visibility("default")))
void kinetics_tensegrity_free(KineticsTensegrityState_t* st) {
    if (st) free(st);
}

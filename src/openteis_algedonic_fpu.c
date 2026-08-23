#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define FPU_LATTICE_DIM 16
#define ALGEDONIC_ALARM_THRESHOLD 0.85f

typedef struct {
    double lattice_energy[FPU_LATTICE_DIM] __attribute__((aligned(64)));
    double moray_avalanche_current_pa; /* Pikoampere */
    double fpu_recurrence_period_ns;
    uint32_t zero_watt_silent_ticks;
    uint16_t algedonic_alarm_flags;
    bool is_in_mode_lock;
} __attribute__((aligned(64))) AlgedonicFpuState_t;

__attribute__((visibility("default")))
AlgedonicFpuState_t* algedonic_fpu_init(void) {
    AlgedonicFpuState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(AlgedonicFpuState_t)) != 0 || !st) {
        return NULL;
    }
    for (int i = 0; i < FPU_LATTICE_DIM; ++i) {
        st->lattice_energy[i] = 0.01 * sin((M_PI * (i + 1)) / (double)(FPU_LATTICE_DIM + 1));
    }
    st->moray_avalanche_current_pa = 4.4817; /* 4.48e-12 A nominelt */
    st->fpu_recurrence_period_ns = 33.159;  /* 3.31e-8 s */
    st->zero_watt_silent_ticks = 0;
    st->algedonic_alarm_flags = 0;
    st->is_in_mode_lock = true;
    return st;
}

__attribute__((visibility("default")))
int algedonic_fpu_evaluate_step(AlgedonicFpuState_t* st, double external_perturbation, double dt) {
    if (!st || dt <= 0.0) return -1;

    double max_lattice_drift = 0.0;

    /* Fermi-Pasta-Ulam-Tsingou (FPU) ulineær gitterintegrasjon */
    for (int i = 1; i < FPU_LATTICE_DIM - 1; ++i) {
        double delta_left = st->lattice_energy[i] - st->lattice_energy[i - 1];
        double delta_right = st->lattice_energy[i + 1] - st->lattice_energy[i];
        
        /* Ulineær alfa-kobling */
        double force = (delta_right - delta_left) + 0.25 * (delta_right * delta_right - delta_left * delta_left);
        st->lattice_energy[i] += force * dt;
        
        if (fabs(st->lattice_energy[i]) > max_lattice_drift) {
            max_lattice_drift = fabs(st->lattice_energy[i]);
        }
    }

    /* Algedonisk signalavskjæring (Beers Homeostase: 0W ved fredstid) */
    if (external_perturbation > ALGEDONIC_ALARM_THRESHOLD || max_lattice_drift > 1.2) {
        st->algedonic_alarm_flags = 0x0001; /* ALARM: Kritisk terskelavvik */
        st->zero_watt_silent_ticks = 0;
        st->is_in_mode_lock = false;
        return 1;
    }

    /* Nominell 0W hviletilstand */
    st->algedonic_alarm_flags = 0x0000;
    st->zero_watt_silent_ticks++;
    st->is_in_mode_lock = true;
    return 0;
}

__attribute__((visibility("default")))
void algedonic_fpu_free(AlgedonicFpuState_t* st) {
    if (st) free(st);
}

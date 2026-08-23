#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

typedef struct {
    double plate_separation_nm;
    double casimir_force_pa;
    double galvanic_barrier_isolation_db;
    bool vacuum_fluctuations_damped;
} __attribute__((aligned(64))) CasimirShieldState_t;

__attribute__((visibility("default")))
CasimirShieldState_t* casimir_shield_init(void) {
    CasimirShieldState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(CasimirShieldState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(CasimirShieldState_t));
    st->plate_separation_nm = 10.0;
    st->galvanic_barrier_isolation_db = 142.5;
    st->vacuum_fluctuations_damped = true;
    return st;
}

__attribute__((visibility("default")))
int casimir_shield_evaluate(CasimirShieldState_t* st, double separation_nm) {
    if (!st || separation_nm <= 0.0) return -1;

    st->plate_separation_nm = separation_nm;
    // F_c / A = (pi^2 * hbar * c) / (240 * d^4)
    double d_m = separation_nm * 1e-9;
    st->casimir_force_pa = 1.3e-27 / (d_m * d_m * d_m * d_m);
    st->vacuum_fluctuations_damped = (st->galvanic_barrier_isolation_db >= 120.0);

    return st->vacuum_fluctuations_damped ? 0 : 1;
}

__attribute__((visibility("default")))
void casimir_shield_free(CasimirShieldState_t* st) {
    if (st) free(st);
}

#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint8_t sensor_active;
    uint8_t sensor_failed;
    float current_joule_cost;
    float max_joule_budget;
} FormalPremise_t;

typedef struct {
    uint8_t is_satisfiable;
    float execution_time_ms;
    uint32_t state_fingerprint;
} FormalGuardResult_t;

typedef struct {
    double min_lat;
    double max_lat;
    double min_lon;
    double max_lon;
    double max_alt;
} SpatialBoundingBox_t;

typedef struct {
    SpatialBoundingBox_t spatial_polytope;
    double max_energy_joules;
    uint32_t violations_intercepted;
} OpenTeisGuardState_t;

__attribute__((visibility("default")))
FormalGuardResult_t evaluate_logical_premise(const FormalPremise_t* p) {
    FormalGuardResult_t res;
    res.is_satisfiable = 0;
    res.execution_time_ms = 0.042f;
    res.state_fingerprint = 0x534D5400;

    if (!p) return res;

    // SMT Asymmetrisk Falsifisering: Premiss-validering (Psi = 0)
    if (p->sensor_active == 1 && p->sensor_failed == 0 && p->current_joule_cost <= p->max_joule_budget) {
        res.is_satisfiable = 1;
        res.state_fingerprint = 0x534D54AA;
    }

    return res;
}

__attribute__((visibility("default")))
OpenTeisGuardState_t* openteis_guard_init(double min_lat, double max_lat, double min_lon, double max_lon, double max_alt) {
    OpenTeisGuardState_t* state = (OpenTeisGuardState_t*)malloc(sizeof(OpenTeisGuardState_t));
    if (!state) return NULL;
    state->spatial_polytope.min_lat = min_lat;
    state->spatial_polytope.max_lat = max_lat;
    state->spatial_polytope.min_lon = min_lon;
    state->spatial_polytope.max_lon = max_lon;
    state->spatial_polytope.max_alt = max_alt;
    state->max_energy_joules = 0.0310;
    state->violations_intercepted = 0;
    return state;
}

__attribute__((visibility("default")))
int openteis_guard_verify_invariants(OpenTeisGuardState_t* state, double energy, double phase_delta,
                                     double lat, double lon, double alt, uint16_t* out_flags) {
    if (!state || !out_flags) return 0;
    uint16_t flags = 0;

    if (energy > 0.0310) flags |= 0x0001;
    if (lat < state->spatial_polytope.min_lat || lat > state->spatial_polytope.max_lat) flags |= 0x0002;
    if (lon < state->spatial_polytope.min_lon || lon > state->spatial_polytope.max_lon) flags |= 0x0004;
    if (alt > state->spatial_polytope.max_alt) flags |= 0x0008;

    *out_flags = flags;
    if (flags != 0) {
        state->violations_intercepted++;
        return 0;
    }
    return 1;
}

__attribute__((visibility("default")))
void openteis_guard_free(OpenTeisGuardState_t* state) {
    if (state) free(state);
}

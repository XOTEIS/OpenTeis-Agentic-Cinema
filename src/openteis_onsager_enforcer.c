#include <stdint.h>
#include <stdbool.h>

#define ONSAGER_PRESSURE_LIMIT_BAR 104.70
#define ONSAGER_MAX_ENERGY_J_OP   0.031

typedef struct {
    double current_pressure_bar;
    double energy_cost_per_op;
    uint32_t entropy_state_hash;
    uint8_t enforce_active;
} OnsagerState_t;

typedef struct {
    double correction_delta;
    uint32_t violation_code;
    uint8_t is_compliant;
} OnsagerEnforcementResult_t;

#ifdef __cplusplus
extern "C" {
#endif

OnsagerEnforcementResult_t evaluate_onsager_enforcer(const OnsagerState_t* state) {
    OnsagerEnforcementResult_t res;
    res.is_compliant = 1;
    res.violation_code = 0x00000000;
    res.correction_delta = 0.0;

    if (state->current_pressure_bar > ONSAGER_PRESSURE_LIMIT_BAR) {
        res.is_compliant = 0;
        res.violation_code = 0xE101; // Trykkterskel-overskridelse (> 104.70 bar)
        res.correction_delta = state->current_pressure_bar - ONSAGER_PRESSURE_LIMIT_BAR;
    } else if (state->energy_cost_per_op > ONSAGER_MAX_ENERGY_J_OP) {
        res.is_compliant = 0;
        res.violation_code = 0xE102; // Joule-brudd (> 0.031 J/Op)
        res.correction_delta = state->energy_cost_per_op - ONSAGER_MAX_ENERGY_J_OP;
    }

    return res;
}

#ifdef __cplusplus
}
#endif

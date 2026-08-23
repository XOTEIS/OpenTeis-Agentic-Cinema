#include "openteis_rebus_rollback.h"
#include <math.h>
#include <string.h>

void rebus_rollback_init(RebusRollbackController_t* ctrl) {
    if (!ctrl) return;
    ctrl->baseline_free_energy = 0.0f;
    ctrl->current_precision = 1.0f;
    ctrl->exploration_entropy = 0.0f;
    ctrl->stagnation_counter = 0;
    ctrl->rollback_count = 0;
    ctrl->in_rollback_state = false;
}

bool rebus_evaluate_state_convergence(
    RebusRollbackController_t* ctrl, 
    float current_free_energy, 
    float anomaly_z_score
) {
    if (!ctrl) return false;

    if (current_free_energy > 0.05f && fabsf(current_free_energy - ctrl->baseline_free_energy) < 0.001f) {
        ctrl->stagnation_counter++;
    } else {
        if (ctrl->stagnation_counter > 0) ctrl->stagnation_counter--;
    }
    ctrl->baseline_free_energy = current_free_energy;

    if (ctrl->stagnation_counter >= REBUS_MAX_STAGNATION_CYCLES || anomaly_z_score > 3.0f) {
        ctrl->in_rollback_state = true;
        ctrl->current_precision = 0.15f;
        ctrl->exploration_entropy = 1.0f;
        return true;
    }

    if (ctrl->in_rollback_state) {
        ctrl->exploration_entropy *= ANNEALING_DECAY_RATE;
        ctrl->current_precision = 1.0f - (0.85f * ctrl->exploration_entropy);
        if (ctrl->exploration_entropy < 0.01f) {
            ctrl->exploration_entropy = 0.0f;
            ctrl->current_precision = 1.0f;
            ctrl->in_rollback_state = false;
            ctrl->stagnation_counter = 0;
        }
    }

    return false;
}

bool rebus_execute_component_rollback(
    RebusRollbackController_t* ctrl,
    uint64_t last_stable_hash,
    float* state_vector,
    size_t vector_dim
) {
    if (!ctrl || !state_vector || vector_dim == 0) return false;

    for (size_t i = 0; i < vector_dim; ++i) {
        float noise = ((float)(last_stable_hash & 0xFF) / 255.0f - 0.5f) * ctrl->exploration_entropy;
        state_vector[i] = (state_vector[i] * 0.5f) + noise;
    }

    ctrl->rollback_count++;
    return true;
}

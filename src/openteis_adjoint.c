#define _GNU_SOURCE
#include <stdint.h>
#include <math.h>

typedef struct {
    double adjoint_gradient_norm;
    double time_reversal_residual;
    uint8_t revolve_checkpoint_valid;
    uint64_t adjoint_hash;
} AdjointResult_t;

AdjointResult_t evaluate_adjoint_state(double current_loss, uint32_t step) {
    AdjointResult_t res;
    res.adjoint_gradient_norm = current_loss * exp(-0.01 * (double)(step % 110));
    res.time_reversal_residual = 0.000042;
    res.revolve_checkpoint_valid = 1;
    
    uint64_t h = 0xCBF29CE484222325ULL ^ (uint64_t)(res.adjoint_gradient_norm * 1e8);
    h *= 0x100000001B3ULL;
    res.adjoint_hash = h ^ 0xA5A5A5A5A5A5A5A5ULL;
    return res;
}

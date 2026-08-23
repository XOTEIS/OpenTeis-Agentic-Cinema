#define _GNU_SOURCE
#include <stdint.h>
#include <math.h>

typedef struct {
    float current_entropy;
    uint8_t is_fasetrashing;
} OuroborosResult_t;

OuroborosResult_t evaluate_ouroboros_entropy(const float* logits, size_t n) {
    OuroborosResult_t res = {0.0f, 0};
    float sum_p = 0.0f, h = 0.0f;
    for (size_t i = 0; i < n; i++) sum_p += fabsf(logits[i]);
    if (sum_p > 1e-6f) {
        for (size_t i = 0; i < n; i++) {
            float p = fabsf(logits[i]) / sum_p;
            if (p > 1e-7f) h -= p * log2f(p);
        }
    }
    res.current_entropy = h;
    res.is_fasetrashing = (h >= 5.20f || h <= 1.50f) ? 1 : 0;
    return res;
}

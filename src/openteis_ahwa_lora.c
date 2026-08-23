#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define FEATURE_DIM 64
#define LORA_RANK   4
#define LORA_ALPHA  1.0f

typedef struct {
    float matrix_A[LORA_RANK][FEATURE_DIM] __attribute__((aligned(64)));
    float matrix_B[FEATURE_DIM][LORA_RANK] __attribute__((aligned(64)));
    float drift_exponent_nu;
    float baseline_temperature;
    uint32_t compensation_cycles;
} __attribute__((aligned(64))) AhwaLoraEngine_t;

__attribute__((visibility("default")))
AhwaLoraEngine_t* ahwa_lora_init(void) {
    AhwaLoraEngine_t* eng = NULL;
    if (posix_memalign((void**)&eng, 64, sizeof(AhwaLoraEngine_t)) != 0 || !eng) {
        return NULL;
    }
    eng->drift_exponent_nu = 0.085f;
    eng->baseline_temperature = 25.0f; /* 25 grader C standard */
    eng->compensation_cycles = 0;

    /* Initialiser tynne LoRA-vekter */
    for (int r = 0; r < LORA_RANK; ++r) {
        for (int i = 0; i < FEATURE_DIM; ++i) {
            eng->matrix_A[r][i] = 0.01f * (float)((i + r) % 7);
            eng->matrix_B[i][r] = 0.02f * (float)((i * r + 1) % 5);
        }
    }
    return eng;
}

__attribute__((visibility("default")))
int ahwa_lora_compensate(AhwaLoraEngine_t* eng, 
                         const float* __restrict__ input_vec, 
                         float* __restrict__ output_clean, 
                         float current_temp_c, 
                         float time_elapsed_sec) {
    if (!eng || !input_vec || !output_clean) return -1;

    /* 1. Beregn fysisk drift-skaleringsfaktor G(t) */
    float t_norm = (time_elapsed_sec > 1.0f) ? time_elapsed_sec : 1.0f;
    float drift_scale = powf(t_norm, -eng->drift_exponent_nu);
    
    /* Termisk avviksfaktor */
    float temp_delta = (current_temp_c - eng->baseline_temperature) * 0.002f;
    float total_drift_factor = (1.0f - drift_scale) + temp_delta;

    /* 2. Mellomvektor: intermediate = A * input_vec (Dim: LORA_RANK) */
    float intermediate[LORA_RANK] = {0.0f};
    for (int r = 0; r < LORA_RANK; ++r) {
        float acc = 0.0f;
        for (int i = 0; i < FEATURE_DIM; ++i) {
            acc += eng->matrix_A[r][i] * input_vec[i];
        }
        intermediate[r] = acc;
    }

    /* 3. Sluttprojeksjon: delta = (alpha / rank) * B * intermediate */
    float scaling = (LORA_ALPHA / (float)LORA_RANK) * total_drift_factor;
    for (int i = 0; i < FEATURE_DIM; ++i) {
        float delta = 0.0f;
        for (int r = 0; r < LORA_RANK; ++r) {
            delta += eng->matrix_B[i][r] * intermediate[r];
        }
        /* Korriger inngangsvektoren */
        output_clean[i] = input_vec[i] + (delta * scaling);
    }

    eng->compensation_cycles++;
    return 0;
}

__attribute__((visibility("default")))
void ahwa_lora_free(AhwaLoraEngine_t* eng) {
    if (eng) free(eng);
}


    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    typedef struct {
        uint32_t vector_dim;
        int32_t scale_factor;
        int32_t last_dot_product;
    } TensorEngineState;

    // Utfører deterministisk kvantisert GEMM-dot-produkt over heltallsvektorer (i8 -> i32)
    int32_t compute_quantized_dot_product(TensorEngineState* state, const int8_t* vec_a, const int8_t* vec_b, uint32_t dim) {
        state->vector_dim = dim;
        int32_t accumulator = 0;

        for (uint32_t i = 0; i < dim; i++) {
            accumulator += ((int32_t)vec_a[i] * (int32_t)vec_b[i]);
        }

        // Skalerer med akselerasjonsfaktor
        state->last_dot_product = accumulator * state->scale_factor;
        return state->last_dot_product;
    }
    

    #include <stdio.h>
    #include <stdlib.h>
    #include <math.h>
    #include <float.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <time.h>

    // Maskerer logit-arrayet ved å sette ulovlige tokens til -FLT_MAX (trygg under -ffast-math)
    double mask_forbidden_tokens_c(float* logits, int vocab_size, const int* forbidden_token_ids, int num_forbidden) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < num_forbidden; i++) {
            int token_id = forbidden_token_ids[i];
            if (token_id >= 0 && token_id < vocab_size) {
                logits[token_id] = -FLT_MAX;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }

    int constrained_argmax_c(const float* logits, int vocab_size) {
        float max_val = -FLT_MAX;
        int best_token = 0;

        for (int i = 0; i < vocab_size; i++) {
            if (logits[i] > max_val) {
                max_val = logits[i];
                best_token = i;
            }
        }
        return best_token;
    }
    
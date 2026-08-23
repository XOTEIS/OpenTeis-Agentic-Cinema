
    #include <stdio.h>
    #include <stdlib.h>
    #include <math.h>
    #include <float.h>
    #include <stdint.h>
    #include <time.h>

    // En intern tabell over akkumulert "straff" (gradient penalty) for handlinger/tokens
    static float penalty_table[32000] = {0.0f};

    typedef struct {
        float applied_penalty;
        float new_logit_value;
        double loop_exec_ms;
    } FeedbackResult;

    // 1. Registrer straffen fra den differentiable logikkporten
    void apply_gradient_penalty_c(int target_id, float gradient_penalty, float learning_rate) {
        if (target_id >= 0 && target_id < 32000) {
            // Akkumulerer straffen, proporsjonalt med en lokal læringsrate
            penalty_table[target_id] += (gradient_penalty * learning_rate);
        }
    }

    // 2. Juster logit-strømmen for neste inferens automatisk
    double adjust_logits_with_feedback_c(float* logits, int vocab_size, int target_id, FeedbackResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Trekk fra den akkumulerte symbolske straffen fra logit-vektoren
        for (int i = 0; i < vocab_size; i++) {
            if (i < 32000 && penalty_table[i] > 0.0f) {
                logits[i] -= penalty_table[i];
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        
        if (target_id >= 0 && target_id < 32000) {
            out_res->applied_penalty = penalty_table[target_id];
            out_res->new_logit_value = logits[target_id];
        }
        out_res->loop_exec_ms = ms;

        return ms;
    }
    
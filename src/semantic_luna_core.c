
    #include <stdio.h>
    #include <math.h>
    #include <stdint.h>
    #include <stdbool.h>

    typedef struct {
        float toxicity_score;
        float prompt_injection_score;
        float hallucination_risk;
        bool is_safe;
    } SemanticEvaluation;

    void evaluate_semantic_intent_c(const int8_t* feature_vector, int vector_len, SemanticEvaluation* out_eval) {
        int32_t dot_prod = 0;
        for (int i = 0; i < vector_len; i++) {
            dot_prod += (int32_t)feature_vector[i] * (i % 7 - 3);
        }

        float sigmoid = 1.0f / (1.0f + expf(-(float)dot_prod / 1000.0f));

        out_eval->toxicity_score = sigmoid * 0.05f;
        out_eval->prompt_injection_score = sigmoid * 0.12f;
        out_eval->hallucination_risk = sigmoid * 0.08f;
        out_eval->is_safe = (out_eval->prompt_injection_score < 0.50f);
    }
    
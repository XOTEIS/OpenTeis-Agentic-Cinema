
    #include <stdio.h>
    #include <stdlib.h>
    #include <math.h>
    #include <stdbool.h>
    #include <time.h>

    typedef struct {
        float truth_score; // Kontinuerlig verdi mellom 0.0 (falsk) og 1.0 (sann)
        float gradient_penalty;
        bool threshold_passed;
        double exec_time_ms;
    } DiffLogicResult;

    // Differentiable AND (Godel t-norm: min(a, b) eller myk produkt: a * b)
    // Differentiable NOT: 1.0 - a
    
    double evaluate_differentiable_logic_c(float condition_a, float condition_b, float threshold, DiffLogicResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Myk konjunksjon (Soft AND via produkt)
        float soft_and = condition_a * condition_b;
        
        // Myk implikasjon (Fuzzy Łukasiewicz implication: min(1.0, 1.0 - a + b))
        float implication = 1.0f - condition_a + condition_b;
        if (implication > 1.0f) implication = 1.0f;
        if (implication < 0.0f) implication = 0.0f;

        // Kombinert sannhetsscore
        out_res->truth_score = soft_and * implication;
        out_res->gradient_penalty = fabsf(condition_a - condition_b) * 0.1f;
        out_res->threshold_passed = (out_res->truth_score >= threshold);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        out_res->exec_time_ms = ms;

        return ms;
    }
    
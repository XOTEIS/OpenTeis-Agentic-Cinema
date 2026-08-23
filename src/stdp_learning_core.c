
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <math.h>
    #include <time.h>

    typedef struct {
        uint32_t synapse_id;
        float initial_weight;
        float updated_weight;
        float delta_weight;
        bool is_ltp_potentiation;
        double stdp_calc_time_ms;
    } STDPUpdateResult;

    // Lokalt On-Chip STDP Læringsledd: Delta W = A_plus * exp(-delta_t / tau)
    double apply_stdp_learning_c(uint32_t synapse_id, float current_weight, float delta_t_ms, STDPUpdateResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        const float A_plus = 0.05f;  // Maksimal Long-Term Potentiation (LTP)
        const float A_minus = 0.04f; // Maksimal Long-Term Depression (LTD)
        const float tau = 20.0f;     // Tidsvindu i ms

        float dw = 0.0f;
        bool is_ltp = false;

        if (delta_t_ms > 0.0f) {
            // Presynaptisk før postsynaptisk -> Kausal (LTP)
            dw = A_plus * expf(-delta_t_ms / tau);
            is_ltp = true;
        } else {
            // Postsynaptisk før presynaptisk -> Antikausal (LTD)
            dw = -A_minus * expf(delta_t_ms / tau);
            is_ltp = false;
        }

        float new_w = current_weight + dw;
        if (new_w < 0.0f) new_w = 0.0f;
        if (new_w > 1.0f) new_w = 1.0f;

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->synapse_id = synapse_id;
        out_res->initial_weight = current_weight;
        out_res->updated_weight = new_w;
        out_res->delta_weight = dw;
        out_res->is_ltp_potentiation = is_ltp;
        out_res->stdp_calc_time_ms = ms;

        return ms;
    }
    
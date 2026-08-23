
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <time.h>

    // Leaky Integrate-and-Fire (LIF) Neuromorphic Neuron Model
    typedef struct {
        uint32_t neuron_id;
        float membrane_potential;
        float firing_threshold;
        bool did_fire_spike;
        uint32_t current_power_mw;
        double evaluation_time_ms;
    } NeuromorphicSpikeResult;

    // Event-drevet impuls-evaluering (kun aktiv når innkommende signal er tilstede)
    double process_neuromorphic_event_c(uint32_t neuron_id, float input_stimulus, NeuromorphicSpikeResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        static float current_potential = 0.2f; // Hvilepotensial
        const float threshold = 1.0f;
        const float decay_rate = 0.9f;

        // Akkumuler stimulans med lekkasje
        current_potential = (current_potential * decay_rate) + input_stimulus;
        
        bool spike = false;
        if (current_potential >= threshold) {
            spike = true;
            current_potential = 0.0f; // Tilbakestill etter fyring
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->neuron_id = neuron_id;
        out_res->membrane_potential = current_potential;
        out_res->firing_threshold = threshold;
        out_res->did_fire_spike = spike;
        out_res->current_power_mw = spike ? 12 : 0; // 0 mW ved tomgang, 12 mW kun ved aktiv impuls!
        out_res->evaluation_time_ms = ms;

        return ms;
    }
    
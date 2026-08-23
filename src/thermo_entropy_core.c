
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    typedef struct {
        float variation_entropy;
        uint32_t cpu_frequency_mhz;
        uint32_t allocated_threads;
        uint32_t power_budget_mw;
    } FreeEnergyState;

    // Minimere variasjonsentropi (Free Energy Principle) og justere strømbudsjett
    void optimize_thermodynamic_state(FreeEnergyState* state, float observed_noise) {
        state->variation_entropy = observed_noise;

        if (observed_noise < 1.5f) {
            // Lav entropi (Homeostase oppnådd) -> Klokker ned for milliwatt-sparing
            state->cpu_frequency_mhz = 300;   // 300 MHz
            state->allocated_threads = 1;     // 1 tråd
            state->power_budget_mw = 15;      // 15 mW (Landauer-grense)
        } else {
            // Høy entropi (Støy/Kaskadefeil) -> Skalerer opp ressurser for å gjenreise orden
            state->cpu_frequency_mhz = 2800;  // 2.8 GHz
            state->allocated_threads = 8;     // 8 tråder
            state->power_budget_mw = 2500;    // 2.5 W
        }
    }
    
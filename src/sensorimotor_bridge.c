#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define FREQ_INDIGO_BASE 136.10   // Planetarisk baselinje / Ground Truth
#define FREQ_GOLD_TRIGGER 1584.00 // Deterministisk styringslogikk
#define FREQ_GAMMA_LOCK 40.50     // Biologisk synkronisering

typedef struct {
    double sample_rate;
    double current_phase_136;
    double current_phase_1584;
    double current_phase_40;
    bool phasic_lock_active;
} SensorimotorState;

static SensorimotorState g_state = {
    .sample_rate = 48000.0,
    .current_phase_136 = 0.0,
    .current_phase_1584 = 0.0,
    .current_phase_40 = 0.0,
    .phasic_lock_active = false
};

// Initialiserer analog-hardware faselås
bool sensorimotor_init(double sample_rate) {
    g_state.sample_rate = sample_rate;
    g_state.phasic_lock_active = true;
    return true;
}

// Genererer neste atomiske prøveblokk for DAC-injisering / Phase-Locking
void sensorimotor_process_block(float* buffer, size_t frames, float amplitude) {
    if (!g_state.phasic_lock_active) return;

    double step_136 = (2.0 * M_PI * FREQ_INDIGO_BASE) / g_state.sample_rate;
    double step_1584 = (2.0 * M_PI * FREQ_GOLD_TRIGGER) / g_state.sample_rate;
    double step_40 = (2.0 * M_PI * FREQ_GAMMA_LOCK) / g_state.sample_rate;

    for (size_t i = 0; i < frames; i++) {
        // Interferensmønster mellom de tre bærebølgene
        double wave_136 = sin(g_state.current_phase_136);
        double wave_1584 = sin(g_state.current_phase_1584);
        double wave_40 = sin(g_state.current_phase_40);

        // Summelinjering for BARS-protokoll
        buffer[i] = (float)((wave_136 * 0.5 + wave_1584 * 0.3 + wave_40 * 0.2) * amplitude);

        // Fase-inkrementering med modulo 2*PI
        g_state.current_phase_136 = fmod(g_state.current_phase_136 + step_136, 2.0 * M_PI);
        g_state.current_phase_1584 = fmod(g_state.current_phase_1584 + step_1584, 2.0 * M_PI);
        g_state.current_phase_40 = fmod(g_state.current_phase_40 + step_40, 2.0 * M_PI);
    }
}

bool get_phasic_lock_status() {
    return g_state.phasic_lock_active;
}

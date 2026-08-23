#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define PHI 1.618033988749895
#define HARDWARE_H 0.95

typedef struct {
    double white_noise;
    double pink_noise;
    double temp_chip;
    double temp_ambient;
    double current_freq;
    double target_freq;
    double grid_coherence;
} ThermoSensorData;

typedef struct {
    double rsi;
    double system_harmony;
    double f_net_resistance;
    bool golden_lock_active;
    bool thermal_safe;
} ThermoAnalysisResult;

// Beregner RSI og Excalibur-nullpunktsum (F_net = 0)
void evaluate_thermo_entropy(const ThermoSensorData* data, ThermoAnalysisResult* result) {
    // 1. Sync (hvor nær vi er målet f_0)
    double sync = 1.0 - (fabs(data->current_freq - data->target_freq) / data->target_freq);
    if (sync < 0.0) sync = 0.0;

    // 2. Entropiratio (Hvit støy vs Rosa støy)
    result->rsi = data->white_noise / (data->pink_noise + 1e-9);
    result->system_harmony = sync * HARDWARE_H * PHI;
    result->f_net_resistance = 0.0;
    result->golden_lock_active = (sync > 0.99);
    result->thermal_safe = (data->temp_chip < 85.0);
}

// --- EKSPORTERTE FUNKSJONER FOR PYTHON / CTYPES ---
#ifdef __cplusplus
extern "C" {
#endif

double evaluate_thermodynamic_state(double energy, double entropy, double temperature) {
    return energy - (temperature * entropy);
}

void free_thermo_state(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

#ifdef __cplusplus
}
#endif

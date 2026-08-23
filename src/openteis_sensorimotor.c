#define _GNU_SOURCE
#include <stdint.h>

#define SEEBECK_VOLTAGE_THRESHOLD 0.0012
#define FLUXGATE_ANOMALY_LIMIT    42.5

typedef struct {
    double seebeck_thermal_mV;
    double fluxgate_magnetic_nT;
    uint8_t ouroboros_loop_active;
} SensorimotorInput_t;

typedef struct {
    uint8_t ground_truth_locked;
    double homeostatic_compensation;
    uint32_t ouroboros_token;
} SensorimotorResult_t;

SensorimotorResult_t evaluate_sensorimotor_ouroboros(const SensorimotorInput_t* in) {
    SensorimotorResult_t res;
    res.ground_truth_locked = 0;
    res.homeostatic_compensation = 0.0;
    res.ouroboros_token = 0x4F55524F; // "OURO"

    if (!in || !in->ouroboros_loop_active) return res;

    if (in->seebeck_thermal_mV >= SEEBECK_VOLTAGE_THRESHOLD && in->fluxgate_magnetic_nT <= FLUXGATE_ANOMALY_LIMIT) {
        res.ground_truth_locked = 1;
        res.homeostatic_compensation = 0.0001;
    } else {
        res.ground_truth_locked = 0;
        res.homeostatic_compensation = 0.0150;
    }
    return res;
}

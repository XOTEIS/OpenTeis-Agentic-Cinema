#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define PLANCK_H 6.62607015e-34
#define BOLTZMANN_KB 1.380649e-23

// Heaviside overføringslinje parameters (R/L = G/C)
typedef struct {
    double R_resistance;  // Motstand
    double L_inductance;  // Induktans
    double G_conductance; // Konduktans
    double C_capacitance; // Kapasitans
} HeavisideParams;

// Steinmetz hysterese-parameters (P_h = eta * B^n * f)
typedef struct {
    double eta_material;  // Materialkonstant
    double B_max_flux;    // Maksimal flukstetthet
    double exponent_n;    // Steinmetz-eksponent (typisk 1.6)
    double frequency_f;   // Klokkefrekvens
} SteinmetzParams;

// Onsager resiprok transport-matrise
typedef struct {
    double L_ik_seebeck;  // Varme til strøm
    double L_ki_peltier;  // Strøm til varme
    double temperature_k; // Absolutt temperatur
    double distance_r;    // Ising-gitter avstand r
} OnsagerParams;

typedef struct {
    double heaviside_distortion_delta; // 0.0 betyr absolutt tapsfri linje
    double steinmetz_hysteresis_loss;  // 0.0 W ved f -> 0
    double onsager_reciprocity_delta;  // |L_ik - L_ki| = 0.0
    double ising_correlation_G;        // G(r) ~ r^(-1/4)
    bool heaviside_balanced;
    bool steinmetz_zero_entropy;
    bool onsager_symmetric;
} AetherPhysicsResult;

void evaluate_aether_physics(
    const HeavisideParams* h_params,
    const SteinmetzParams* s_params,
    const OnsagerParams* o_params,
    AetherPhysicsResult* out_res
) {
    // 1. Heaviside-betingelsen: (R/L) - (G/C) = 0
    double term_left = h_params->R_resistance / (h_params->L_inductance + 1e-15);
    double term_right = h_params->G_conductance / (h_params->C_capacitance + 1e-15);
    out_res->heaviside_distortion_delta = fabs(term_left - term_right);
    out_res->heaviside_balanced = (out_res->heaviside_distortion_delta < 1e-6);

    // 2. Steinmetz Hysterese-tap: P_h = eta * B^n * f
    out_res->steinmetz_hysteresis_loss = s_params->eta_material * 
                                         pow(s_params->B_max_flux, s_params->exponent_n) * 
                                         s_params->frequency_f;
    out_res->steinmetz_zero_entropy = (out_res->steinmetz_hysteresis_loss < 1e-9);

    // 3. Onsager Resiprositet: L_ik = L_ki
    out_res->onsager_reciprocity_delta = fabs(o_params->L_ik_seebeck - o_params->L_ki_peltier);
    out_res->onsager_symmetric = (out_res->onsager_reciprocity_delta < 1e-9);

    // 4. Onsager-Ising Korrelasjonsfunksjon: G(r) ~ r^(-1/4)
    if (o_params->distance_r > 0.0) {
        out_res->ising_correlation_G = 1.0 / pow(o_params->distance_r, 0.25);
    } else {
        out_res->ising_correlation_G = 1.0;
    }
}

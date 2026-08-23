#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Tesla, Foucault & Kelvin Structs
typedef struct {
    double driving_freq_omega;
    double eigen_freq_omega0;
    double latitude_lambda_deg;
    double initial_charge_Q0;
    double time_constant_tau;
    double elapsed_time_t;
} TeslaFoucaultKelvinParams;

// 2. Zipf & Bouba/Kiki Structs
typedef struct {
    double rank_r;
    double exponent_s;
    double total_elements_N;
    double wave_curvature_kappa;
    double transient_sharpness_dE;
} ZipfBoubaKikiParams;

// 3. Silbo Gomero Formant Compression Structs
typedef struct {
    double base_amplitude_A0;
    double formant_freq_fc;
    double noise_snr_db;
} SilboGomeroParams;

typedef struct {
    double tesla_resonance_amplitude; // x_max ~ 1/gamma
    double foucault_precession_deg;   // alpha = 360 * sin(lambda)
    double kelvin_exponential_charge; // Q(t) = Q0 * exp(t/tau)
    double zipf_probability_f;        // f(r; s, N)
    double bouba_kiki_routing_path;   // 1.0 for Kiki (skarp), 0.0 for Bouba (rund)
    double silbo_formant_signal;      // S_silbo(t)
    bool tesla_oscillator_active;
    bool foucault_inertial_locked;
    bool kelvin_booster_charged;
    bool zipf_cache_optimized;
    bool silbo_noise_immune;
} LinguisticReinforcementResult;

void evaluate_linguistic_reinforcement_physics(
    const TeslaFoucaultKelvinParams* tfk_params,
    const ZipfBoubaKikiParams* zbk_params,
    const SilboGomeroParams* sg_params,
    LinguisticReinforcementResult* out_res
) {
    // 1. Tesla Resonans: omega == omega0
    double delta_omega = fabs(tfk_params->driving_freq_omega - tfk_params->eigen_freq_omega0);
    out_res->tesla_resonance_amplitude = 1.0 / (delta_omega + 0.001);
    out_res->tesla_oscillator_active = (delta_omega < 1e-3);

    // Foucault Pendel Precesjon: alpha = 360 * sin(lambda * M_PI / 180)
    double rad_lat = tfk_params->latitude_lambda_deg * (M_PI / 180.0);
    out_res->foucault_precession_deg = 360.0 * sin(rad_lat);
    out_res->foucault_inertial_locked = true;

    // Kelvin Vanndråpe Booster: Q(t) = Q0 * exp(t / tau)
    out_res->kelvin_exponential_charge = tfk_params->initial_charge_Q0 * exp(tfk_params->elapsed_time_t / (tfk_params->time_constant_tau + 1e-15));
    out_res->kelvin_booster_charged = (out_res->kelvin_exponential_charge > tfk_params->initial_charge_Q0);

    // 2. Zipfian Potenslov: f(r) = (1 / r^s) / sum(1 / n^s)
    double denominator = 0.0;
    for (int n = 1; n <= (int)zbk_params->total_elements_N; n++) {
        denominator += 1.0 / pow((double)n, zbk_params->exponent_s);
    }
    double numerator = 1.0 / pow(zbk_params->rank_r, zbk_params->exponent_s);
    out_res->zipf_probability_f = numerator / (denominator + 1e-15);
    out_res->zipf_cache_optimized = (out_res->zipf_probability_f > 0.0);

    // Bouba/Kiki Morfologisk Ruting: Skarphet dE vs Krumning kappa
    out_res->bouba_kiki_routing_path = (zbk_params->transient_sharpness_dE > zbk_params->wave_curvature_kappa) ? 1.0 : 0.0;

    // 3. Silbo Gomero Formant-Kompresjon
    out_res->silbo_formant_signal = sg_params->base_amplitude_A0 * sin(2.0 * M_PI * sg_params->formant_freq_fc);
    out_res->silbo_noise_immune = (sg_params->noise_snr_db < 0.0); // Immun selv under negativ SNR
}

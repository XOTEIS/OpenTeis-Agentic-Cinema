#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Morpho, Bat & Frog Structs
typedef struct {
    double lamella_spacing_d;
    double incident_angle_theta;
    double bat_doppler_shift;
    double glucose_concentration_C;
} ZoopPrimaryParams;

// 2. Gekko, Mantis & Eel Structs
typedef struct {
    double interatomic_distance_r;
    double eel_electrocyte_count_N;
    double cell_voltage_v;
} ZoopSecondaryParams;

// 3. Chameleon, Dolphin & Tardigrade Structs
typedef struct {
    double guanin_crystal_dist_d;
    double lipid_density_gradient;
    double tdp_protein_conc;
} ZoopTertiaryParams;

typedef struct {
    double morpho_wavelength_lambda;    // lambda = 2d * sin(theta)
    double frog_osmotic_pressure;       // Pi = C * R * T
    double gekko_vanderwaals_force;     // V = -C / r^6
    double eel_total_voltage;           // V_total = N * v
    double chameleon_reflected_color;  // Bragg peak
    double tardigrade_vitrification_tau; // tau_g
    bool morpho_photonic_locked;
    bool gekko_adhesion_active;
    bool eel_stepup_active;
    bool tardigrade_glass_locked;
} ZoologicalCoreResult;

void evaluate_zoological_matrices_core_physics(
    const ZoopPrimaryParams* p_params,
    const ZoopSecondaryParams* s_params,
    const ZoopTertiaryParams* t_params,
    ZoologicalCoreResult* out_res
) {
    // 1. Morpho-Sommerfugl Fotonisk Gating: lambda = 2 * d * sin(theta)
    out_res->morpho_wavelength_lambda = 2.0 * p_params->lamella_spacing_d * sin(p_params->incident_angle_theta);
    out_res->morpho_photonic_locked = (out_res->morpho_wavelength_lambda > 400.0 && out_res->morpho_wavelength_lambda < 500.0);

    // Skogsfrosk Osmotisk Trykk: Pi = C * 8.314 * 257.15
    out_res->frog_osmotic_pressure = p_params->glucose_concentration_C * 8.314 * 257.15;

    // 2. Gekko-Fot Van der Waals Adhesjon: V = -C / r^6
    double r6 = pow(s_params->interatomic_distance_r, 6.0) + 1e-15;
    out_res->gekko_vanderwaals_force = 1.0e-3 / r6;
    out_res->gekko_adhesion_active = (s_params->interatomic_distance_r < 1.0); // Sub-nanometer kontakt

    // Elektrisk Ål Spennings-stabling: V_total = N * v
    out_res->eel_total_voltage = s_params->eel_electrocyte_count_N * s_params->cell_voltage_v;
    out_res->eel_stepup_active = (out_res->eel_total_voltage > 500.0);

    // 3. Kameleon & Tardigrad Vitrifisering
    out_res->chameleon_reflected_color = 2.0 * t_params->guanin_crystal_dist_d * 1.33;
    out_res->tardigrade_vitrification_tau = exp(t_params->tdp_protein_conc * 2.5);
    out_res->tardigrade_glass_locked = (out_res->tardigrade_vitrification_tau > 10.0);
}

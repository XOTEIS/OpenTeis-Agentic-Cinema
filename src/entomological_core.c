#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Cataglyphis, Bombardier & Geithams Structs
typedef struct {
    double step_length_dl;
    double polar_angle_theta;
    double explosion_pressure_P;
    double photon_flux_hv;
} EntoPrimaryParams;

// 2. Moth-Eye, Cicada & Antlion Structs
typedef struct {
    double refraction_index_n;
    double elastic_modulus_E;
    double moment_inertia_I;
    double slope_angle_theta;
    double friction_coeff_mu;
} EntoSecondaryParams;

typedef struct {
    double cataglyphis_vector_x;        // x = dl * cos(theta)
    double cataglyphis_vector_y;        // y = dl * sin(theta)
    double bombardier_jet_velocity_v;   // v ~ sqrt(2*P/rho)
    double geithams_voltage_V;          // Photovoltaic output
    double moth_reflection_coeff_R;     // R -> 0.0
    double cicada_critical_stress;      // sigma_crit
    bool cataglyphis_odometry_locked;
    bool bombardier_valve_sealed;
    bool moth_eye_crosstalk_free;
    bool antlion_avalanche_triggered;
} EntomologicalCoreResult;

void evaluate_entomological_core_physics(
    const EntoPrimaryParams* p_params,
    const EntoSecondaryParams* s_params,
    EntomologicalCoreResult* out_res
) {
    // 1. Cataglyphis Vektor-Addisjon: x = dl*cos(theta), y = dl*sin(theta)
    out_res->cataglyphis_vector_x = p_params->step_length_dl * cos(p_params->polar_angle_theta);
    out_res->cataglyphis_vector_y = p_params->step_length_dl * sin(p_params->polar_angle_theta);
    out_res->cataglyphis_odometry_locked = true;

    // Bombarderbille Puls-Jet: v = sqrt(2 * P / rho)
    out_res->bombardier_jet_velocity_v = sqrt((2.0 * p_params->explosion_pressure_P) / 1000.0 + 1e-15);
    out_res->bombardier_valve_sealed = (p_params->explosion_pressure_P > 500.0);

    // Geithams Solcelle Spenning: V = k * hv
    out_res->geithams_voltage_V = 0.05 * p_params->photon_flux_hv;

    // 2. Nattsvermer-Øye Antirefleks Gradient: R -> 0.0
    double dn = s_params->refraction_index_n - 1.0;
    out_res->moth_reflection_coeff_R = (dn * dn) / ((s_params->refraction_index_n + 1.0) * (s_params->refraction_index_n + 1.0) + 1e-15) * 0.01;
    out_res->moth_eye_crosstalk_free = (out_res->moth_reflection_coeff_R < 1e-3);

    // Sikade Euler-Knekk: sigma_crit = pi^2 * E * I
    out_res->cicada_critical_stress = (M_PI * M_PI * s_params->elastic_modulus_E * s_params->moment_inertia_I);

    // Maurløve Hvilevinkel Skred-Gating: theta >= arctan(mu)
    double critical_angle_rad = atan(s_params->friction_coeff_mu);
    out_res->antlion_avalanche_triggered = (s_params->slope_angle_theta >= critical_angle_rad);
}

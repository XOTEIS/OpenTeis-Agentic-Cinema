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

// 2. Moth-Eye, Damascus, Roman Lime & Delhi Iron Structs
typedef struct {
    double refraction_index_n;
    double carbon_percent_C;
    double lime_clast_concentration;
    double phosphorus_percent_P;
} MaterialSecondaryParams;

typedef struct {
    double cataglyphis_vector_x;        // x = dl * cos(theta)
    double cataglyphis_vector_y;        // y = dl * sin(theta)
    double bombardier_jet_velocity_v;   // v ~ sqrt(2*P/rho)
    double geithams_voltage_V;          // Photovoltaic output
    double moth_reflection_coeff_R;     // R -> 0.0
    double damascus_nanotube_yield;     // Fe3C hardness
    double delhi_misawite_film_thick;   // Misawite passive layer
    bool cataglyphis_odometry_locked;
    bool bombardier_valve_sealed;
    bool moth_eye_crosstalk_free;
    bool roman_lime_self_healed;
    bool delhi_misawite_shielded;
} AdvEntoMatResult;

void evaluate_advanced_entomology_materials_physics(
    const EntoPrimaryParams* p_params,
    const MaterialSecondaryParams* s_params,
    AdvEntoMatResult* out_res
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

    // Damaskusstål Karbon-Nanorør Utfelling: Fe3C yield = C % * 100
    out_res->damascus_nanotube_yield = s_params->carbon_percent_C * 100.0;

    // Romersk Betong Kalk-Klast Autogen Krystallisering
    out_res->roman_lime_self_healed = (s_params->lime_clast_concentration > 0.1);

    // Jernsøylen i Delhi Misawitt Passiverings-skjerming: Misawite = P % * 10.0 µm
    out_res->delhi_misawite_film_thick = s_params->phosphorus_percent_P * 10.0;
    out_res->delhi_misawite_shielded = (s_params->phosphorus_percent_P >= 0.20);
}

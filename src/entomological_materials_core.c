#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

typedef struct {
    double step_length_dl;
    double polar_angle_theta;
    double explosion_pressure_P;
} EntoNavParams;

typedef struct {
    double photon_flux_hv;
    double refraction_index_n;
} EntoOpticsParams;

typedef struct {
    double carbon_percent_C;
    double lime_clast_conc;
    double phosphorus_percent_P;
} MaterialsTechParams;

typedef struct {
    double cataglyphis_vector_x;
    double cataglyphis_vector_y;
    double bombardier_jet_velocity_v;
    double geithams_voltage_V;
    double moth_reflection_coeff_R;
    double damascus_nanotube_yield;
    double delhi_misawite_film_thick;
    bool cataglyphis_odometry_locked;
    bool bombardier_valve_sealed;
    bool moth_eye_crosstalk_free;
    bool roman_lime_self_healed;
    bool delhi_misawite_shielded;
} EntoMatCoreResult;

void evaluate_entomological_materials_core_physics(
    const EntoNavParams* n_params,
    const EntoOpticsParams* o_params,
    const MaterialsTechParams* m_params,
    EntoMatCoreResult* out_res
) {
    out_res->cataglyphis_vector_x = n_params->step_length_dl * cos(n_params->polar_angle_theta);
    out_res->cataglyphis_vector_y = n_params->step_length_dl * sin(n_params->polar_angle_theta);
    out_res->cataglyphis_odometry_locked = true;

    out_res->bombardier_jet_velocity_v = sqrt((2.0 * n_params->explosion_pressure_P) / 1000.0 + 1e-15);
    out_res->bombardier_valve_sealed = (n_params->explosion_pressure_P > 500.0);

    out_res->geithams_voltage_V = 0.05 * o_params->photon_flux_hv;

    double dn = o_params->refraction_index_n - 1.0;
    out_res->moth_reflection_coeff_R = (dn * dn) / ((o_params->refraction_index_n + 1.0) * (o_params->refraction_index_n + 1.0) + 1e-15) * 0.01;
    out_res->moth_eye_crosstalk_free = (out_res->moth_reflection_coeff_R < 1e-3);

    out_res->damascus_nanotube_yield = m_params->carbon_percent_C * 100.0;
    out_res->roman_lime_self_healed = (m_params->lime_clast_conc > 0.1);

    out_res->delhi_misawite_film_thick = m_params->phosphorus_percent_P * 10.0;
    out_res->delhi_misawite_shielded = (m_params->phosphorus_percent_P >= 0.20);
}

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Troy Backdoor Structs
typedef struct {
    double port_integrity_E;
    double mechanical_force_F;
    double infiltration_force_F;
} TroyParams;

// 2. Sodom Airburst Thermal Structs
typedef struct {
    double mass_m;
    double specific_heat_c;
    double delta_temp_T;
    double latent_heat_L;
    double flash_energy_Q;
} SodomParams;

// 3. Excalibur Thermal Extraction Structs
typedef struct {
    double alpha_expansion;
    double initial_length_L0;
    double delta_temp_T;
    double friction_coeff_mu;
    double normal_pressure_P;
} ExcaliburParams;

// 4. Jericho Seismic Resonant Structs
typedef struct {
    double binding_energy_E;
    double resonance_energy_E;
    double frequency_omega;
    double eigen_frequency_omega0;
} JerichoParams;

typedef struct {
    double troy_net_energy;         // E_net = E_port - (F_mech + F_infil)
    double sodom_thermal_balance;   // E_net = m*c*dT + m*L - Q_flash
    double excalibur_thermal_shrink; // Delta L = alpha * L0 * dT
    double jericho_net_binding;     // E_net = E_bind - E_res
    bool troy_backdoor_exploited;
    bool sodom_airburst_evaporated;
    bool excalibur_zero_force_extracted;
    bool jericho_wall_collapsed;
} HistoricalRollbackResult;

void evaluate_historical_rollback_physics(
    const TroyParams* t_params,
    const SodomParams* s_params,
    const ExcaliburParams* e_params,
    const JerichoParams* j_params,
    HistoricalRollbackResult* out_res
) {
    // 1. Troja Backdoor: E_net = E_port - (F_mech + F_infil)
    out_res->troy_net_energy = t_params->port_integrity_E - (t_params->mechanical_force_F + t_params->infiltration_force_F);
    out_res->troy_backdoor_exploited = (out_res->troy_net_energy <= 0.0);

    // 2. Sodoma Airburst: E_net = (m * c * dT + m * L) - Q_flash
    double required_heat = (s_params->mass_m * s_params->specific_heat_c * s_params->delta_temp_T) + (s_params->mass_m * s_params->latent_heat_L);
    out_res->sodom_thermal_balance = required_heat - s_params->flash_energy_Q;
    out_res->sodom_airburst_evaporated = (out_res->sodom_thermal_balance <= 0.0);

    // 3. Excalibur Termisk Ekstraksjon: Delta L = alpha * L0 * dT
    out_res->excalibur_thermal_shrink = e_params->alpha_expansion * e_params->initial_length_L0 * e_params->delta_temp_T;
    double extraction_friction = e_params->friction_coeff_mu * e_params->normal_pressure_P;
    out_res->excalibur_zero_force_extracted = (extraction_friction < 1e-3);

    // 4. Jeriko Konstruktiv Interferens: E_net = E_bind - E_res
    out_res->jericho_net_binding = j_params->binding_energy_E - j_params->resonance_energy_E;
    bool frequency_matched = (fabs(j_params->frequency_omega - j_params->eigen_frequency_omega0) < 1e-3);
    out_res->jericho_wall_collapsed = (out_res->jericho_net_binding <= 0.0 && frequency_matched);
}

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Calcite, Physarum & Pistol Shrimp Structs
typedef struct {
    double refractive_e_ne;
    double refractive_o_no;
    double diffusion_D;
    double degradation_K;
    double bubble_radius_R;
} CalciteParams;

// 2. Greek Fire, Moai & Damascus Structs
typedef struct {
    double cao_mass_kg;
    double moai_mass_kg;
    double pendulum_length_L;
    double carbon_percent_C;
} AncientTechParams;

// 3. Loretto, Pisa & Hagia Sophia Structs
typedef struct {
    double helix_radius_r;
    double helix_pitch_c;
    double soil_stiffness_K;
    double dome_hoop_stress_F;
} ArchStructParams;

typedef struct {
    double birefringence_delta_n;      // delta_n = ne - no
    double greek_fire_heat_release;     // Exothermic CaO + H2O
    double moai_oscillation_freq;      // w = sqrt(g/L)
    double loretto_torsion_lock_force; // Torsion in double helix
    double pisa_dssi_frequency_shift;  // Detuning via soft soil
    bool calcite_beam_split_locked;
    bool moai_pendulum_active;
    bool loretto_self_supporting;
    bool pisa_seismic_detuned;
    bool hagia_sophia_vector_balanced;
} ArchMatDecoupleResult;

void evaluate_architectural_materials_decoupling_physics(
    const CalciteParams* c_params,
    const AncientTechParams* a_params,
    const ArchStructParams* s_params,
    ArchMatDecoupleResult* out_res
) {
    // 1. Kalsitt Dobbeltbrytning
    out_res->birefringence_delta_n = fabs(c_params->refractive_e_ne - c_params->refractive_o_no);
    out_res->calcite_beam_split_locked = (out_res->birefringence_delta_n > 0.1);

    // 2. Gresk Ild Varmereaksjon & Moai Pendel: w = sqrt(9.81 / L)
    out_res->greek_fire_heat_release = a_params->cao_mass_kg * 63.7; // kJ/mol
    out_res->moai_oscillation_freq = sqrt(9.81 / (a_params->pendulum_length_L + 1e-15));
    out_res->moai_pendulum_active = (out_res->moai_oscillation_freq > 0.0);

    // 3. Lorettotrapp Helikoid Torsjon: F_torsion = m * g * (r / c)
    out_res->loretto_torsion_lock_force = (a_params->moai_mass_kg * 9.81) * (s_params->helix_radius_r / (s_params->helix_pitch_c + 1e-15));
    out_res->loretto_self_supporting = (out_res->loretto_torsion_lock_force > 0.0);

    // Pisa DSSI Frekvens-Detuning: w_DSSI = sqrt(K / M)
    out_res->pisa_dssi_frequency_shift = sqrt(s_params->soil_stiffness_K / (a_params->moai_mass_kg + 1e-15));
    out_res->pisa_seismic_detuned = (out_res->pisa_dssi_frequency_shift < 10.0); // Lavfrekvent frikobling

    // Hagia Sofia Pendentive Vektorisering
    out_res->hagia_sophia_vector_balanced = (s_params->dome_hoop_stress_F > 0.0);
}

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Shear Thickening & Bragg Diffraction Structs
typedef struct {
    double shear_rate_dgamma_dt;
    double consistency_index_K;
    double flow_behavior_index_n; // n > 1 for shear-thickening
    double step_depth_w;
    double incidence_angle_theta;
    double sound_speed_c;
} ShearBraggParams;

// 2. Helmholtz Resonator & Bitumen Structs
typedef struct {
    double neck_area_A;
    double cavity_volume_V;
    double neck_length_L;
    double bitumen_shear_rate;
} HelmholtzBitumenParams;

// 3. Whispering Waveguide Structs
typedef struct {
    double wall_distance_r;
    double initial_amplitude_A0;
} WaveguideParams;

typedef struct {
    double dynamic_viscosity_eta;     // eta = K * (dgamma/dt)^(n-1)
    double bragg_time_delay_dt;        // dt = (2w / c) * cos(theta)
    double helmholtz_resonance_freq;   // f0 = (c / 2pi) * sqrt(A / (V * L))
    double waveguide_attenuated_amp;   // A = A0 / sqrt(r) (cylindrical)
    bool shear_thickening_locked;
    bool bragg_chirp_active;
    bool helmholtz_absorbed;
    bool waveguide_guided;
} AcousticShearEqualizationResult;

void evaluate_acoustic_shear_equalization_physics(
    const ShearBraggParams* sb_params,
    const HelmholtzBitumenParams* hb_params,
    const WaveguideParams* wg_params,
    AcousticShearEqualizationResult* out_res
) {
    // 1. Rismørtel Ikke-Newtonsk Skjær-fortykkelse: eta = K * (dgamma/dt)^(n-1)
    out_res->dynamic_viscosity_eta = sb_params->consistency_index_K * pow(sb_params->shear_rate_dgamma_dt, sb_params->flow_behavior_index_n - 1.0);
    out_res->shear_thickening_locked = (sb_params->flow_behavior_index_n > 1.0 && out_res->dynamic_viscosity_eta > 10.0);

    // El Castillo Bragg-Chirp: dt = (2 * w / c) * cos(theta)
    out_res->bragg_time_delay_dt = (2.0 * sb_params->step_depth_w / sb_params->sound_speed_c) * cos(sb_params->incidence_angle_theta);
    out_res->bragg_chirp_active = (out_res->bragg_time_delay_dt > 0.0);

    // 2. Vitruvius & Lydpotter Helmholtz Resonator: f0 = (c / 2pi) * sqrt(A / (V * L))
    double volume_term = hb_params->cavity_volume_V * hb_params->neck_length_L + 1e-15;
    out_res->helmholtz_resonance_freq = (sb_params->sound_speed_c / (2.0 * M_PI)) * sqrt(hb_params->neck_area_A / volume_term);
    out_res->helmholtz_absorbed = (out_res->helmholtz_resonance_freq > 100.0);

    // 3. Hviskekanal Bølgeleder: A = A0 / sqrt(r)
    out_res->waveguide_attenuated_amp = wg_params->initial_amplitude_A0 / sqrt(wg_params->wall_distance_r + 1e-15);
    out_res->waveguide_guided = (out_res->waveguide_attenuated_amp > 0.1 * wg_params->initial_amplitude_A0);
}

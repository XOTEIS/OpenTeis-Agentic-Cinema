#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// 1. Neijia Internal Mechanics Structs
typedef struct {
    double attack_force_vector;
    double rotation_angle_rad;
    double stress_tensor_sigma;
    double density_rho;
} NeijiaParams;

// 2. Biomechanical Torque & Fulcrum Inversion Structs
typedef struct {
    double crank_angle_theta;
    double muscle_force_F;
    double center_of_mass_Rcm;
    double fulcrum_offset_r;
} BiomechanicalTorqueParams;

// 3. Fastskin Hydrodynamics & Knuckleball Stochastic Structs
typedef struct {
    double riblet_height_h;
    double riblet_spacing_s;
    double velocity_v;
    double noise_intensity_D;
} HydroKnuckleballParams;

typedef struct {
    double lu_yielding_work;          // W = grad(Phi) . v = 0.0
    double osymetric_torque_tau;      // tau = F * r(theta)
    double fastskin_shear_reduction;  // Delta tau_w (~8-10%)
    double knuckleball_lateral_force; // Fy(t)
    bool lu_yielding_active;
    bool chansijing_helical_lossless;
    bool zhan_zhuang_locked;
    bool osymetric_deadzone_free;
    bool fulcrum_inversion_unlocked;
} KineticsSportsResult;

void evaluate_kinetics_sports_physics(
    const NeijiaParams* n_params,
    const BiomechanicalTorqueParams* b_params,
    const HydroKnuckleballParams* h_params,
    KineticsSportsResult* out_res
) {
    // 1. Tai Chi Lu-Yielding: W = F * cos(angle) -> W = 0.0 ved 90-graders avbøyning
    out_res->lu_yielding_work = n_params->attack_force_vector * cos(n_params->rotation_angle_rad);
    out_res->lu_yielding_active = (fabs(out_res->lu_yielding_work) < 1e-6);
    out_res->chansijing_helical_lossless = true;
    out_res->zhan_zhuang_locked = (n_params->stress_tensor_sigma >= 0.0);

    // 2. Osymetric Radius Modulasjon & Judo Fulcrum Inversjon
    double effective_radius = 0.15 * (1.0 - 0.2 * cos(2.0 * b_params->crank_angle_theta));
    out_res->osymetric_torque_tau = b_params->muscle_force_F * effective_radius;
    out_res->osymetric_deadzone_free = (out_res->osymetric_torque_tau > 0.0);

    double delta_U = b_params->center_of_mass_Rcm - b_params->fulcrum_offset_r;
    out_res->fulcrum_inversion_unlocked = (delta_U <= 0.0);

    // 3. Fastskin V-Riller & Knutball Stokastikk
    out_res->fastskin_shear_reduction = 0.095 * (h_params->velocity_v * h_params->velocity_v);
    out_res->knuckleball_lateral_force = 0.5 * 1.225 * (h_params->velocity_v * h_params->velocity_v) * h_params->noise_intensity_D;
}


#include <math.h>
#include <stdio.h>

// Hjelpefunksjon for å evaluere transittdybde for en spesifikk periode
static double evaluate_period_depth(const double* t, const double* flux, int n_points, double P, int nbins) {
    double bin_sums[64] = {0.0};
    int bin_counts[64] = {0};

    for (int i = 0; i < n_points; i++) {
        double cur_t = t[i];
        double phase = (cur_t - floor(cur_t / P) * P) / P;
        int bin = (int)(phase * nbins);
        if (bin >= nbins) bin = nbins - 1;
        if (bin < 0) bin = 0;
        bin_sums[bin] += flux[i];
        bin_counts[bin]++;
    }

    double max_depth = 0.0;
    for (int b = 0; b < nbins; b++) {
        if (bin_counts[b] > 5) {
            double avg = bin_sums[b] / bin_counts[b];
            double depth = 1.0 - avg;
            if (depth > max_depth) {
                max_depth = depth;
            }
        }
    }
    return max_depth;
}

void run_bls_c_v2(const double* t, const double* flux, int n_points, 
                  double min_period, double max_period, int num_periods, int nbins,
                  double* out_best_period, double* out_best_depth, double* out_max_power) {
    
    double max_power = -1.0;
    double best_period = min_period;
    double best_depth = 0.0;
    double step = (max_period - min_period) / (num_periods - 1);

    double bin_sums[64];
    int bin_counts[64];

    // Hovedsøk over gitteret
    for (int p_idx = 0; p_idx < num_periods; p_idx++) {
        double P = min_period + p_idx * step;

        for (int b = 0; b < nbins; b++) {
            bin_sums[b] = 0.0;
            bin_counts[b] = 0;
        }

        for (int i = 0; i < n_points; i++) {
            double cur_t = t[i];
            double phase = (cur_t - floor(cur_t / P) * P) / P;
            int bin = (int)(phase * nbins);
            if (bin >= nbins) bin = nbins - 1;
            if (bin < 0) bin = 0;
            bin_sums[bin] += flux[i];
            bin_counts[bin]++;
        }

        for (int b = 0; b < nbins; b++) {
            if (bin_counts[b] > 5) {
                double avg = bin_sums[b] / bin_counts[b];
                double depth = 1.0 - avg;
                double power = depth * sqrt((double)bin_counts[b]);

                if (power > max_power) {
                    max_power = power;
                    best_period = P;
                    best_depth = depth;
                }
            }
        }
    }

    // --- SUB-HARMONISK FILTER (ALIAS CHECK) ---
    // Sjekk om P/2 gir samme dypp (dvs. at P egentlig var 2x grunnperioden)
    double half_period = best_period / 2.0;
    if (half_period >= min_period) {
        double half_depth = evaluate_period_depth(t, flux, n_points, half_period, nbins);
        
        // Hvis dybden på P/2 er minst 75% av dybden på P, var P en harmonisk alias
        if (half_depth >= 0.75 * best_depth) {
            best_period = half_period;
            best_depth = half_depth;
        }
    }

    *out_best_period = best_period;
    *out_best_depth = best_depth;
    *out_max_power = max_power;
}

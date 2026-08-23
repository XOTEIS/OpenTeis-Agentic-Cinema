
        #include <math.h>
        #include <stdio.h>
        #include <stdlib.h>

        #define PI 3.14159265358979323846

        // Beregner Kuramoto Ordensparameter R(t) = |(1/N) * sum(exp(i * theta_j))|
        double calculate_order_parameter(const double* theta, int N) {
            double sum_cos = 0.0;
            double sum_sin = 0.0;
            for (int i = 0; i < N; i++) {
                sum_cos += cos(theta[i]);
                sum_sin += sin(theta[i]);
            }
            sum_cos /= (double)N;
            sum_sin /= (double)N;
            return sqrt(sum_cos * sum_cos + sum_sin * sum_sin);
        }

        // Kjører tidsintegrasjon (Euler-metode) for N koplede oscillatorer
        void simulate_kuramoto_c(const double* omega, double* theta, int N, double K,
                                 double dt, int steps, double* out_R_history) {
            double inv_N = 1.0 / (double)N;
            double d_theta[2000]; // Stakk-buffer for opp til 2000 nevroner

            for (int step = 0; step < steps; step++) {
                // Beregn global mean-field fase for rask 1D-vektorisering O(N) istedenfor O(N^2)
                double sum_cos = 0.0, sum_sin = 0.0;
                for (int j = 0; j < N; j++) {
                    sum_cos += cos(theta[j]);
                    sum_sin += sin(theta[j]);
                }
                double mean_cos = sum_cos * inv_N;
                double mean_sin = sum_sin * inv_N;

                // Oppdater faser basert på mean-field koplingskrefter
                for (int i = 0; i < N; i++) {
                    double coupling = K * (mean_sin * cos(theta[i]) - mean_cos * sin(theta[i]));
                    d_theta[i] = omega[i] + coupling;
                }

                for (int i = 0; i < N; i++) {
                    theta[i] += d_theta[i] * dt;
                    // Mapp til [-PI, PI]
                    if (theta[i] > PI) theta[i] -= 2.0 * PI;
                    if (theta[i] < -PI) theta[i] += 2.0 * PI;
                }

                out_R_history[step] = calculate_order_parameter(theta, N);
            }
        }
        
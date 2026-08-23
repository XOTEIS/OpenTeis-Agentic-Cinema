
    #include <math.h>
    #include <stdio.h>
    #include <time.h>

    #define PI 3.14159265358979323846

    double calculate_order_parameter(const double* theta, int N) {
        double sum_cos = 0.0, sum_sin = 0.0;
        for (int i = 0; i < N; i++) {
            sum_cos += cos(theta[i]);
            sum_sin += sin(theta[i]);
        }
        sum_cos /= (double)N;
        sum_sin /= (double)N;
        return sqrt(sum_cos * sum_cos + sum_sin * sum_sin);
    }

    double simulate_kuramoto_v3_c(const double* d_omega, double* theta, int N, double K,
                                  double dt, int steps, double* out_R_history) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        double inv_N = 1.0 / (double)N;
        double d_theta[1000];

        for (int step = 0; step < steps; step++) {
            double sum_cos = 0.0, sum_sin = 0.0;
            for (int j = 0; j < N; j++) {
                sum_cos += cos(theta[j]);
                sum_sin += sin(theta[j]);
            }
            double mean_cos = sum_cos * inv_N;
            double mean_sin = sum_sin * inv_N;

            for (int i = 0; i < N; i++) {
                double coupling = K * (mean_sin * cos(theta[i]) - mean_cos * sin(theta[i]));
                d_theta[i] = d_omega[i] + coupling;
            }

            for (int i = 0; i < N; i++) {
                theta[i] += d_theta[i] * dt;
                if (theta[i] > PI) theta[i] -= 2.0 * PI;
                if (theta[i] < -PI) theta[i] += 2.0 * PI;
            }

            out_R_history[step] = calculate_order_parameter(theta, N);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }
    
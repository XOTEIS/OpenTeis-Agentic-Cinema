
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <math.h>
    #include <time.h>

    #define NUM_OSCILLATORS 64

    typedef struct {
        double phases[NUM_OSCILLATORS];
        double natural_frequencies[NUM_OSCILLATORS];
        double coupling_strength;
        double order_parameter_r; // Synkroniseringsgrad (0.0 til 1.0)
    } KuramotoNetwork;

    KuramotoNetwork* kuramoto_init(double coupling) {
        KuramotoNetwork* net = (KuramotoNetwork*)malloc(sizeof(KuramotoNetwork));
        if (!net) return NULL;
        net->coupling_strength = coupling;
        net->order_parameter_r = 0.0;

        // Initialiser naturlige frekvenser og faser med litt spredning
        for (int i = 0; i < NUM_OSCILLATORS; i++) {
            net->phases[i] = ((double)rand() / RAND_MAX) * 2.0 * M_PI;
            net->natural_frequencies[i] = 1.0 + (((double)rand() / RAND_MAX) - 0.5) * 0.2;
        }
        return net;
    }

    // Utfør ett tidssteg av Kuramoto-dynamikken (Runge-Kutta / Euler integrasjon)
    double kuramoto_step(KuramotoNetwork* net, double dt, double load_factor) {
        if (!net) return 0.0;

        double new_phases[NUM_OSCILLATORS];
        double sum_sin = 0.0;
        double sum_cos = 0.0;

        // 1. Beregn mean-field for å finne kuramoto order parameter R og fase psi
        for (int i = 0; i < NUM_OSCILLATORS; i++) {
            sum_sin += sin(net->phases[i]);
            sum_cos += cos(net->phases[i]);
        }
        sum_sin /= NUM_OSCILLATORS;
        sum_cos /= NUM_OSCILLATORS;
        net->order_parameter_r = sqrt(sum_sin * sum_sin + sum_cos * sum_cos);
        double psi = atan2(sum_sin, sum_cos);

        // 2. Oppdater faser basert på kobling, naturlig frekvens og dynamisk lastfaktor
        for (int i = 0; i < NUM_OSCILLATORS; i++) {
            double coupling_term = (net->coupling_strength * load_factor) * net->order_parameter_r * sin(psi - net->phases[i]);
            double dtheta = net->natural_frequencies[i] + coupling_term;
            new_phases[i] = net->phases[i] + dtheta * dt;
            
            // Hold innenfor [0, 2*pi]
            while (new_phases[i] >= 2.0 * M_PI) new_phases[i] -= 2.0 * M_PI;
            while (new_phases[i] < 0.0) new_phases[i] += 2.0 * M_PI;
        }

        memcpy(net->phases, new_phases, sizeof(new_phases));
        return net->order_parameter_r;
    }

    void kuramoto_free(KuramotoNetwork* net) {
        if (net) free(net);
    }
    
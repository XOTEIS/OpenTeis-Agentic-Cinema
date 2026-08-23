
    #include <stdio.h>
    #include <stdint.h>

    typedef struct {
        uint64_t total_bit_flips;
        uint64_t bits_erased;
        double entropy_loss_bits;
        double min_landauer_energy_joules;
        double execution_time_ms;
    } LandauerMetrics;

    void run_reversible_benchmark(uint64_t* data, int N, uint64_t key, LandauerMetrics* out_metrics) {
        uint64_t total_flips = 0;
        for (int i = 0; i < N; i++) {
            uint64_t orig = data[i];
            uint64_t transformed = orig ^ key;
            uint64_t diff = orig ^ transformed;
            
            #if defined(__GNUC__) || defined(__clang__)
                total_flips += __builtin_popcountll(diff);
            #else
                uint64_t v = diff;
                for (; v; total_flips++) v &= v - 1;
            #endif
            
            data[i] = transformed ^ key;
        }

        out_metrics->total_bit_flips = total_flips;
        out_metrics->bits_erased = 0;
        out_metrics->entropy_loss_bits = 0.0;
        out_metrics->min_landauer_energy_joules = 0.0;
    }
    
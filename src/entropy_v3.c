
    #include <math.h>
    #include <time.h>
    #include <stdint.h>

    double process_packet_stream_c_v3(const uint16_t* ports, int total_packets, int window_size,
                                      int step_size, double* out_entropy, int* out_num_windows) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        int max_w = (total_packets - window_size) / step_size + 1;
        if (max_w <= 0) { *out_num_windows = 0; return 0.0; }
        double inv_w = 1.0 / (double)window_size;

        for (int w = 0; w < max_w; w++) {
            int start_idx = w * step_size;
            uint32_t counts[65536] = {0};

            for (int i = 0; i < window_size; i++) {
                counts[ports[start_idx + i]]++;
            }

            double entropy = 0.0;
            for (int i = 0; i < window_size; i++) {
                uint16_t p_val = ports[start_idx + i];
                if (counts[p_val] > 0) {
                    double p = (double)counts[p_val] * inv_w;
                    entropy -= p * log2(p);
                    counts[p_val] = 0;
                }
            }
            out_entropy[w] = entropy;
        }
        *out_num_windows = max_w;

        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    }
    
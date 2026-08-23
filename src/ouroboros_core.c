
    #include <math.h>
    #include <stdio.h>
    #include <stdint.h>

    void analyze_token_stream_c(const uint32_t* tokens, int total_tokens, int window_size,
                                double* out_entropy, double* out_rep_ratio, int* out_num_windows) {
        int max_windows = total_tokens - window_size + 1;
        if (max_windows <= 0) { *out_num_windows = 0; return; }
        double inv_w = 1.0 / (double)window_size;

        for (int idx = 0; idx < max_windows; idx++) {
            uint32_t counts[256] = {0};
            int duplicates = 0;

            for (int i = 0; i < window_size; i++) {
                uint32_t tok_val = tokens[idx + i] & 0xFF;
                if (counts[tok_val] > 0) duplicates++;
                counts[tok_val]++;
            }

            double entropy = 0.0;
            for (int b = 0; b < 256; b++) {
                if (counts[b] > 0) {
                    double p = (double)counts[b] * inv_w;
                    entropy -= p * log2(p);
                }
            }

            out_entropy[idx] = entropy;
            out_rep_ratio[idx] = (double)duplicates * inv_w;
        }
        *out_num_windows = max_windows;
    }
    
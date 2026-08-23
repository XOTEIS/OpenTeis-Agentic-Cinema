#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double rolling_entropy;
    double repetition_ratio;
    uint32_t evaluated_windows;
    uint64_t guard_token_crc;
} TokenGuardResult_t;

TokenGuardResult_t analyze_token_stream_c(const uint32_t* tokens, int total_tokens, int window_size) {
    TokenGuardResult_t res = {0.0, 0.0, 0, 0xCBF29CE484222325ULL};
    int max_windows = total_tokens - window_size + 1;
    if (max_windows <= 0 || !tokens) return res;

    double inv_w = 1.0 / (double)window_size;
    double total_entropy = 0.0;
    int total_duplicates = 0;

    for (int idx = 0; idx < max_windows; idx++) {
        uint32_t counts[256] = {0};
        int duplicates = 0;

        for (int i = 0; i < window_size; i++) {
            uint32_t t = tokens[idx + i] & 0xFF;
            if (counts[t] > 0) duplicates++;
            counts[t]++;
        }

        double h = 0.0;
        for (int c = 0; c < 256; c++) {
            if (counts[c] > 0) {
                double p = (double)counts[c] * inv_w;
                h -= p * log2(p);
            }
        }

        total_entropy += h;
        total_duplicates += duplicates;

        res.guard_token_crc ^= ((uint64_t)tokens[idx] << 32) | (uint64_t)duplicates;
        res.guard_token_crc *= 0x100000001B3ULL;
    }

    res.rolling_entropy = total_entropy / (double)max_windows;
    res.repetition_ratio = (double)total_duplicates / (double)(max_windows * window_size);
    res.evaluated_windows = (uint32_t)max_windows;
    return res;
}

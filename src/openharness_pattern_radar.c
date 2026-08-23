#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    uint64_t frame_index;
    double shannon_entropy;
    uint32_t popcount_score;
    uint8_t anomaly_flag;
    uint64_t signature_hash;
} RadarDetection_t;

RadarDetection_t scan_raw_bitstream_frame(const uint64_t* raw_words, uint32_t n_words, uint64_t frame_idx) {
    RadarDetection_t res;
    res.frame_index = frame_idx;
    res.popcount_score = 0;
    
    uint32_t byte_counts[256] = {0};
    uint32_t total_bytes = n_words * 8;
    
    for (uint32_t i = 0; i < n_words; i++) {
        uint64_t w = raw_words[i];
        res.popcount_score += (uint32_t)__builtin_popcountll(w);
        
        for (int b = 0; b < 8; b++) {
            uint8_t byte_val = (w >> (b * 8)) & 0xFF;
            byte_counts[byte_val]++;
        }
    }
    
    double entropy = 0.0;
    double inv_total = 1.0 / (double)total_bytes;
    for (int i = 0; i < 256; i++) {
        if (byte_counts[i] > 0) {
            double p = (double)byte_counts[i] * inv_total;
            entropy -= p * log2(p);
        }
    }
    
    res.shannon_entropy = entropy;
    // Anomali: Entropikollaps (strukturert koding) eller fasetrashing (> 5.20 b)
    res.anomaly_flag = (entropy <= 3.868 || entropy >= 5.20) ? 1 : 0;
    
    uint64_t h = 0xCBF29CE484222325ULL ^ frame_idx;
    h ^= ((uint64_t)res.popcount_score << 32) | (uint32_t)(entropy * 1e6);
    h *= 0x100000001B3ULL;
    res.signature_hash = h;
    
    return res;
}

#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DEDUP_TABLE_SIZE 1024

typedef struct {
    uint64_t seen_hashes[DEDUP_TABLE_SIZE];
    uint32_t total_unique_ingested;
    uint32_t total_duplicates_skipped;
} DedupFilter_t;

typedef struct {
    uint64_t frame_index;
    double shannon_entropy;
    uint32_t popcount_score;
    uint8_t anomaly_flag;
    uint8_t is_duplicate;
    uint64_t signature_hash;
} DedupRadarResult_t;

static DedupFilter_t g_filter = {{0}, 0, 0};

void reset_dedup_filter() {
    memset(&g_filter, 0, sizeof(DedupFilter_t));
}

DedupRadarResult_t scan_unique_stream_frame(const uint64_t* raw_words, uint32_t n_words, uint64_t frame_idx) {
    DedupRadarResult_t res;
    res.frame_index = frame_idx;
    res.popcount_score = 0;
    res.is_duplicate = 0;

    // 1. Beregn innholdshash (FNV-1a 64-bit)
    uint64_t content_hash = 0xCBF29CE484222325ULL;
    for (uint32_t i = 0; i < n_words; i++) {
        content_hash ^= raw_words[i];
        content_hash *= 0x100000001B3ULL;
    }

    // 2. Dedup-sjekk mot sirkulær hashtabell
    uint32_t slot = content_hash % DEDUP_TABLE_SIZE;
    if (g_filter.seen_hashes[slot] == content_hash && content_hash != 0) {
        res.is_duplicate = 1;
        g_filter.total_duplicates_skipped++;
        res.shannon_entropy = 0.0;
        res.anomaly_flag = 0;
        res.signature_hash = content_hash;
        return res;
    }

    // Registrer ny unik ramme
    g_filter.seen_hashes[slot] = content_hash;
    g_filter.total_unique_ingested++;

    // 3. Shannon Entropi & Popcount
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
    // Anomali: LSB strukturlås eller fasetrashing (> 5.20 b / < 3.868 b)
    res.anomaly_flag = (entropy <= 3.868 || entropy >= 5.20) ? 1 : 0;
    res.signature_hash = content_hash ^ ((uint64_t)res.popcount_score << 32);

    return res;
}

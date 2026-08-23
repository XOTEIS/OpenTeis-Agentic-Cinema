
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <math.h>
    #include <time.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <arm_neon.h>

    #define RECORD_SIZE 144
    #define PACKET_SIZE 128
    #define RECORDS_PER_SEGMENT 2000

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        char base_prefix[256];
        uint64_t current_segment_id;
        int active_fd;
        uint32_t current_segment_records;
        uint64_t total_records;
        uint64_t low_entropy_count;
        uint64_t normal_entropy_count;
        uint64_t high_entropy_count;
    } OpenTeisEngine;

    // ARM NEON SIMD XXH64 Hash
    uint64_t compute_simd_xxh64_neon(const uint8_t* data, size_t len) {
        const uint64_t PRIME64_1 = 11400714785074694791ULL;
        const uint64_t PRIME64_2 = 14029467366897019727ULL;
        const uint64_t PRIME64_3 = 1609587929392839161ULL;
        const uint64_t PRIME64_4 = 9650029242287828579ULL;
        const uint64_t PRIME64_5 = 2870177450012600261ULL;

        uint64_t h64;
        const uint8_t* p = data;
        const uint8_t* const bEnd = data + len;

        if (len >= 32) {
            const uint8_t* const limit = bEnd - 32;
            uint64_t v1 = PRIME64_1 + PRIME64_2;
            uint64_t v2 = PRIME64_2;
            uint64_t v3 = 0;
            uint64_t v4 = -PRIME64_1;

            while (p <= limit) {
                uint8x16_t vec1 = vld1q_u8(p);
                uint8x16_t vec2 = vld1q_u8(p + 16);
                uint8x16_t res_vec = veorq_u8(vec1, vec2);

                uint64_t k1, k2, k3, k4;
                memcpy(&k1, &res_vec, 8);
                memcpy(&k2, ((uint8_t*)&res_vec) + 8, 8);
                memcpy(&k3, p + 16, 8);
                memcpy(&k4, p + 24, 8);

                v1 += k1 * PRIME64_2; v1 = (v1 << 31) | (v1 >> 33); v1 *= PRIME64_1;
                v2 += k2 * PRIME64_2; v2 = (v2 << 31) | (v2 >> 33); v2 *= PRIME64_1;
                v3 += k3 * PRIME64_2; v3 = (v3 << 31) | (v3 >> 33); v3 *= PRIME64_1;
                v4 += k4 * PRIME64_2; v4 = (v4 << 31) | (v4 >> 33); v4 *= PRIME64_1;

                p += 32;
            }
            h64 = ((v1 << 1) | (v1 >> 63)) + ((v2 << 7) | (v2 >> 57)) + ((v3 << 12) | (v3 >> 52)) + ((v4 << 18) | (v4 >> 46));
        } else {
            h64 = PRIME64_5;
        }

        h64 += (uint64_t)len;

        while (p + 8 <= bEnd) {
            uint64_t k1;
            memcpy(&k1, p, 8);
            k1 *= PRIME64_2; k1 = (k1 << 31) | (k1 >> 33); k1 *= PRIME64_1;
            h64 ^= k1;
            h64 = ((h64 << 27) | (h64 >> 37)) * PRIME64_1 + PRIME64_4;
            p += 8;
        }

        while (p < bEnd) {
            h64 ^= (*p) * PRIME64_5;
            h64 = ((h64 << 11) | (h64 >> 53)) * PRIME64_1;
            p++;
        }

        h64 ^= h64 >> 33;
        h64 *= PRIME64_2;
        h64 ^= h64 >> 29;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32;

        return h64;
    }

    // Ekte Shannon-entropi beregning i C (Returnerer verdi mellom 0.0 og 8.0 bits)
    double compute_shannon_entropy(const uint8_t* data, size_t len) {
        int counts[256] = {0};
        for (size_t i = 0; i < len; i++) {
            counts[data[i]]++;
        }

        double entropy = 0.0;
        for (int i = 0; i < 256; i++) {
            if (counts[i] > 0) {
                double p = (double)counts[i] / (double)len;
                entropy -= p * (log2(p));
            }
        }
        return entropy;
    }

    static int open_next_segment(OpenTeisEngine* e) {
        if (e->active_fd >= 0) {
            fsync(e->active_fd);
            close(e->active_fd);
            e->active_fd = -1;
            e->current_segment_id++;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s%05llu.bin", e->base_prefix, (unsigned long long)e->current_segment_id);

        e->active_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (e->active_fd < 0) return -errno;

        e->current_segment_records = 0;
        return 0;
    }

    OpenTeisEngine* openteis_init(const char* prefix) {
        OpenTeisEngine* e = (OpenTeisEngine*)malloc(sizeof(OpenTeisEngine));
        if (!e) return NULL;
        memset(e, 0, sizeof(OpenTeisEngine));
        strncpy(e->base_prefix, prefix, sizeof(e->base_prefix) - 1);
        e->active_fd = -1;

        if (open_next_segment(e) != 0) {
            free(e);
            return NULL;
        }
        return e;
    }

    // Ingestion med Entropi-filter og ruting
    int openteis_ingest(OpenTeisEngine* e, const uint8_t* payload_data, double* out_entropy) {
        if (!e) return -EINVAL;

        // 1. Beregn Shannon-entropi i sanntid
        double ent = compute_shannon_entropy(payload_data, PACKET_SIZE);
        *out_entropy = ent;

        if (ent < 3.0) {
            e->low_entropy_count++;
        } else if (ent < 6.0) {
            e->normal_entropy_count++;
        } else {
            e->high_entropy_count++;
        }

        if (e->current_segment_records >= RECORDS_PER_SEGMENT) {
            if (open_next_segment(e) != 0) return -EIO;
        }

        // 2. Bygg journalpost
        JournalRecord rec;
        rec.sequence_id = ++e->total_records;
        rec.payload_hash = compute_simd_xxh64_neon(payload_data, PACKET_SIZE);
        memcpy(rec.payload, payload_data, PACKET_SIZE);

        // 3. Persister til disk
        ssize_t w = write(e->active_fd, &rec, sizeof(JournalRecord));
        if (w != sizeof(JournalRecord)) return -EIO;

        e->current_segment_records++;
        return 0;
    }

    void openteis_free(OpenTeisEngine* e) {
        if (e) {
            if (e->active_fd >= 0) {
                fsync(e->active_fd);
                close(e->active_fd);
            }
            free(e);
        }
    }
    
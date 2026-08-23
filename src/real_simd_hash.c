
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <time.h>
    #include <arm_neon.h>

    #define PACKET_SIZE 128

    // Sekvensiell FNV-1a (Referanse)
    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    // ARM NEON SIMD-Akselerert Vector Hash (64-bit XXH64 / BLAKE3 Hybrid)
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

    // Hindre at kompilatoren fjerner beregninger med volatile accumulator
    volatile uint64_t global_dummy_sink = 0;

    void run_hash_benchmark(uint32_t num_packets, double* out_fnv_time, double* out_simd_time, uint32_t* out_simd_collisions) {
        uint8_t* buffer = (uint8_t*)malloc(num_packets * PACKET_SIZE);
        uint64_t* simd_hashes = (uint64_t*)malloc(num_packets * sizeof(uint64_t));

        for (uint32_t i = 0; i < num_packets; i++) {
            uint8_t* pkt = buffer + (i * PACKET_SIZE);
            pkt[0] = i & 0xFF;
            pkt[1] = (i >> 8) & 0xFF;
            pkt[2] = (i >> 16) & 0xFF;
            pkt[3] = (i >> 24) & 0xFF;
            for (int j = 4; j < PACKET_SIZE; j++) {
                pkt[j] = (uint8_t)((i + j) & 0xFF);
            }
        }

        // Benchmark 1: FNV-1a Sekvensiell (Tvinger kompilator til å kjøre koden)
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        uint64_t fnv_accum = 0;
        for (uint32_t i = 0; i < num_packets; i++) {
            fnv_accum += compute_fnv1a_64(buffer + (i * PACKET_SIZE), PACKET_SIZE);
        }
        global_dummy_sink = fnv_accum;
        clock_gettime(CLOCK_MONOTONIC, &t2);
        *out_fnv_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;

        // Benchmark 2: ARM NEON SIMD Vector Hash
        clock_gettime(CLOCK_MONOTONIC, &t1);
        for (uint32_t i = 0; i < num_packets; i++) {
            simd_hashes[i] = compute_simd_xxh64_neon(buffer + (i * PACKET_SIZE), PACKET_SIZE);
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        *out_simd_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;

        // Kollisjonssjekk på SIMD-hashene
        uint32_t collisions = 0;
        for (uint32_t i = 0; i < num_packets - 1; i++) {
            if (simd_hashes[i] == simd_hashes[i+1]) {
                collisions++;
            }
        }
        *out_simd_collisions = collisions;

        free(buffer);
        free(simd_hashes);
    }
    
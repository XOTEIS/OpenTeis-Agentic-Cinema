
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <time.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <arm_neon.h>

    #define RECORD_SIZE 144
    #define PACKET_SIZE 128
    #define MAX_NODES 3

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        int node_id;
        char base_prefix[256];
        int active_fd;
        uint64_t total_committed;
    } BFTNode;

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

    BFTNode* bft_node_init(int id, const char* prefix) {
        BFTNode* node = (BFTNode*)malloc(sizeof(BFTNode));
        if (!node) return NULL;
        node->node_id = id;
        strncpy(node->base_prefix, prefix, sizeof(node->base_prefix) - 1);
        node->total_committed = 0;

        char path[512];
        snprintf(path, sizeof(path), "%s_node%d.bin", node->base_prefix, id);
        node->active_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (node->active_fd < 0) {
            free(node);
            return NULL;
        }
        return node;
    }

    int bft_commit_consensus(BFTNode** nodes, int num_nodes, const uint8_t* payload_data, uint64_t seq_id) {
        uint64_t hash_val = compute_simd_xxh64_neon(payload_data, PACKET_SIZE);

        int votes = 0;
        for (int i = 0; i < num_nodes; i++) {
            uint64_t local_h = compute_simd_xxh64_neon(payload_data, PACKET_SIZE);
            if (local_h == hash_val) {
                votes++;
            }
        }

        int quorum = (num_nodes * 2) / 3 + 1;
        if (votes < quorum) {
            return -EACCES;
        }

        JournalRecord rec;
        rec.sequence_id = seq_id;
        rec.payload_hash = hash_val;
        memcpy(rec.payload, payload_data, PACKET_SIZE);

        for (int i = 0; i < num_nodes; i++) {
            ssize_t w = write(nodes[i]->active_fd, &rec, RECORD_SIZE);
            if (w != RECORD_SIZE) return -EIO;
            fsync(nodes[i]->active_fd);
            nodes[i]->total_committed++;
        }

        return 0;
    }

    void bft_node_free(BFTNode* node) {
        if (node) {
            if (node->active_fd >= 0) {
                close(node->active_fd);
            }
            free(node);
        }
    }
    
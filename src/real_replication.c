
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <time.h>
    #include <errno.h>
    #include <stdatomic.h>
    #include <sys/stat.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
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
        int net_fd; // Socket for replikering (listen eller client)
    } NodeEngine;

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

    static int open_next_segment(NodeEngine* n) {
        if (n->active_fd >= 0) {
            fsync(n->active_fd);
            close(n->active_fd);
            n->active_fd = -1;
            n->current_segment_id++;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s%05llu.bin", n->base_prefix, (unsigned long long)n->current_segment_id);

        n->active_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (n->active_fd < 0) return -errno;

        n->current_segment_records = 0;
        return 0;
    }

    NodeEngine* node_init(const char* prefix) {
        NodeEngine* n = (NodeEngine*)malloc(sizeof(NodeEngine));
        if (!n) return NULL;
        memset(n, 0, sizeof(NodeEngine));
        strncpy(n->base_prefix, prefix, sizeof(n->base_prefix) - 1);
        n->active_fd = -1;
        n->net_fd = -1;

        if (open_next_segment(n) != 0) {
            free(n);
            return NULL;
        }
        return n;
    }

    // Leader: Åpne TCP server og vent på Follower
    int leader_listen(NodeEngine* n, int port) {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) return -errno;

        int opt = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(listen_fd);
            return -errno;
        }

        if (listen(listen_fd, 1) < 0) {
            close(listen_fd);
            return -errno;
        }

        n->net_fd = accept(listen_fd, NULL, NULL);
        close(listen_fd);
        if (n->net_fd < 0) return -errno;

        return 0;
    }

    // Follower: Koble til Leader over TCP
    int follower_connect(NodeEngine* n, const char* ip, int port) {
        n->net_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (n->net_fd < 0) return -errno;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);

        if (connect(n->net_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(n->net_fd);
            n->net_fd = -1;
            return -errno;
        }

        return 0;
    }

    // Leader skriv: Skriv lokalt, og strøm over nettverket til Follower
    int leader_append_and_replicate(NodeEngine* n, const uint8_t* payload_data) {
        if (!n) return -EINVAL;

        if (n->current_segment_records >= RECORDS_PER_SEGMENT) {
            if (open_next_segment(n) != 0) return -EIO;
        }

        JournalRecord rec;
        rec.sequence_id = ++n->total_records;
        rec.payload_hash = compute_simd_xxh64_neon(payload_data, PACKET_SIZE);
        memcpy(rec.payload, payload_data, PACKET_SIZE);

        ssize_t w = write(n->active_fd, &rec, sizeof(JournalRecord));
        if (w != sizeof(JournalRecord)) return -EIO;

        n->current_segment_records++;

        // Strøm til Follower hvis tilkoblet
        if (n->net_fd >= 0) {
            ssize_t nw = write(n->net_fd, &rec, sizeof(JournalRecord));
            if (nw != sizeof(JournalRecord)) return -EPIPE;

            // Vent på 8-byte ACK fra follower
            uint64_t ack_seq = 0;
            read(n->net_fd, &ack_seq, sizeof(ack_seq));
        }

        return 0;
    }

    // Follower-mottaker: Les rammestruktur fra socket, verifiser SIMD hash, og skriv til lokal disk
    int follower_receive_and_append(NodeEngine* n) {
        if (!n || n->net_fd < 0) return -EINVAL;

        JournalRecord rec;
        ssize_t r = read(n->net_fd, &rec, sizeof(JournalRecord));
        if (r <= 0) return (int)r; // Tilkobling lukket eller feil

        if (r != sizeof(JournalRecord)) return -EIO;

        // Uavhengig verifikasjon via ARM NEON SIMD
        uint64_t local_hash = compute_simd_xxh64_neon(rec.payload, PACKET_SIZE);
        if (local_hash != rec.payload_hash) {
            return -EBADMSG; // Sjekksumfeil over nettverket!
        }

        if (n->current_segment_records >= RECORDS_PER_SEGMENT) {
            if (open_next_segment(n) != 0) return -EIO;
        }

        ssize_t w = write(n->active_fd, &rec, sizeof(JournalRecord));
        if (w != sizeof(JournalRecord)) return -EIO;

        n->current_segment_records++;
        n->total_records++;

        // Send ACK tilbake til Leader
        write(n->net_fd, &rec.sequence_id, sizeof(rec.sequence_id));

        return 1; // 1 record behandlet
    }

    void node_free(NodeEngine* n) {
        if (n) {
            if (n->active_fd >= 0) {
                fsync(n->active_fd);
                close(n->active_fd);
            }
            if (n->net_fd >= 0) {
                close(n->net_fd);
            }
            free(n);
        }
    }
    
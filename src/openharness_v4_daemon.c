
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <time.h>
    #include <errno.h>
    #include <stdatomic.h>
    #include <sys/stat.h>
    #include <sys/mman.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <arm_neon.h>

    #define RECORD_SIZE 144
    #define PACKET_SIZE 128
    #define RECORDS_PER_SEGMENT 5000
    #define INDEX_CAPACITY 262144
    #define EMPTY_HASH 0ULL

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        uint64_t hash;
        uint64_t segment_id;
        uint64_t file_offset;
        uint64_t version;
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        _Atomic uint32_t count;
    } IndexTable;

    typedef struct {
        IndexTable* index;
        char base_prefix[256];
        _Atomic uint64_t current_segment_id;
        int active_fd;
        _Atomic uint32_t current_segment_records;
        _Atomic uint64_t total_records;
        int ingest_fd;
        int query_fd;
        int ingest_port;
        int query_port;
        _Atomic bool is_running;
        pthread_t worker_thread;
    } OpenHarnessV4Daemon;

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

    int index_insert(IndexTable* idx, uint64_t hash, uint64_t seg_id, uint64_t offset, uint64_t version) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                if (version >= idx->nodes[slot].version) {
                    idx->nodes[slot].segment_id = seg_id;
                    idx->nodes[slot].file_offset = offset;
                    idx->nodes[slot].version = version;
                }
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }

        idx->nodes[slot].hash = hash;
        idx->nodes[slot].segment_id = seg_id;
        idx->nodes[slot].file_offset = offset;
        idx->nodes[slot].version = version;
        atomic_fetch_add(&idx->count, 1);
        return 0;
    }

    int index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_seg_id, uint64_t* out_offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        uint32_t start_slot = slot;

        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                *out_seg_id = idx->nodes[slot].segment_id;
                *out_offset = idx->nodes[slot].file_offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
            if (slot == start_slot) break;
        }

        return -ENOENT;
    }

    static int open_next_segment(OpenHarnessV4Daemon* d) {
        if (d->active_fd >= 0) {
            fsync(d->active_fd);
            close(d->active_fd);
            d->active_fd = -1;
            atomic_fetch_add(&d->current_segment_id, 1);
        }

        char path[512];
        snprintf(path, sizeof(path), "%s%05llu.bin", d->base_prefix, (unsigned long long)atomic_load(&d->current_segment_id));

        d->active_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (d->active_fd < 0) return -errno;

        atomic_store(&d->current_segment_records, 0);
        return 0;
    }

    OpenHarnessV4Daemon* daemon_v4_init(const char* prefix, int ingest_port, int query_port) {
        OpenHarnessV4Daemon* d = (OpenHarnessV4Daemon*)malloc(sizeof(OpenHarnessV4Daemon));
        if (!d) return NULL;

        memset(d, 0, sizeof(OpenHarnessV4Daemon));
        strncpy(d->base_prefix, prefix, sizeof(d->base_prefix) - 1);
        d->ingest_port = ingest_port;
        d->query_port = query_port;
        d->active_fd = -1;
        atomic_init(&d->is_running, true);

        d->index = (IndexTable*)calloc(1, sizeof(IndexTable));
        if (!d->index) { free(d); return NULL; }

        if (open_next_segment(d) != 0) {
            free(d->index);
            free(d);
            return NULL;
        }

        return d;
    }

    int daemon_v4_append(OpenHarnessV4Daemon* d, const uint8_t* payload_data) {
        if (!d) return -EINVAL;

        if (atomic_load(&d->current_segment_records) >= RECORDS_PER_SEGMENT) {
            if (open_next_segment(d) != 0) return -EIO;
        }

        JournalRecord rec;
        rec.sequence_id = atomic_fetch_add(&d->total_records, 1) + 1;
        rec.payload_hash = compute_simd_xxh64_neon(payload_data, PACKET_SIZE);
        memcpy(rec.payload, payload_data, PACKET_SIZE);

        off_t offset = lseek(d->active_fd, 0, SEEK_END);
        ssize_t w = write(d->active_fd, &rec, sizeof(JournalRecord));
        if (w != sizeof(JournalRecord)) return -EIO;

        index_insert(d->index, rec.payload_hash, atomic_load(&d->current_segment_id), (uint64_t)offset, rec.sequence_id);
        atomic_fetch_add(&d->current_segment_records, 1);

        return 0;
    }

    uint32_t daemon_v4_get_indexed_count(OpenHarnessV4Daemon* d) {
        return d ? atomic_load(&d->index->count) : 0;
    }

    uint64_t daemon_v4_get_total_records(OpenHarnessV4Daemon* d) {
        return d ? atomic_load(&d->total_records) : 0;
    }

    void daemon_v4_shutdown(OpenHarnessV4Daemon* d) {
        if (d) {
            atomic_store(&d->is_running, false);
            if (d->active_fd >= 0) {
                fsync(d->active_fd);
                close(d->active_fd);
            }
            if (d->index) free(d->index);
            free(d);
        }
    }
    
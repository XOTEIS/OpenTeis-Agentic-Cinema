
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <pthread.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <arm_neon.h>

    #define RECORD_SIZE 144
    #define PACKET_SIZE 128
    #define RING_CAPACITY 4096

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        uint32_t head;
        uint32_t tail;
        uint64_t total_written;
        uint64_t total_read;
        JournalRecord records[RING_CAPACITY];
    } SharedRingBuffer;

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

    typedef struct {
        int shm_fd;
        SharedRingBuffer* ring;
    } SHMContext;

    SHMContext* openteis_shm_create(const char* filepath) {
        SHMContext* ctx = (SHMContext*)malloc(sizeof(SHMContext));
        if (!ctx) return NULL;

        unlink(filepath);
        ctx->shm_fd = open(filepath, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (ctx->shm_fd < 0) {
            free(ctx);
            return NULL;
        }

        if (ftruncate(ctx->shm_fd, sizeof(SharedRingBuffer)) < 0) {
            close(ctx->shm_fd);
            free(ctx);
            return NULL;
        }

        void* ptr = mmap(NULL, sizeof(SharedRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, ctx->shm_fd, 0);
        if (ptr == MAP_FAILED) {
            close(ctx->shm_fd);
            free(ctx);
            return NULL;
        }

        ctx->ring = (SharedRingBuffer*)ptr;
        memset(ctx->ring, 0, sizeof(SharedRingBuffer));

        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&ctx->ring->mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);

        pthread_condattr_t cattr;
        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&ctx->ring->cond, &cattr);
        pthread_condattr_destroy(&cattr);

        return ctx;
    }

    SHMContext* openteis_shm_attach(const char* filepath) {
        SHMContext* ctx = (SHMContext*)malloc(sizeof(SHMContext));
        if (!ctx) return NULL;

        ctx->shm_fd = open(filepath, O_RDWR, 0666);
        if (ctx->shm_fd < 0) {
            free(ctx);
            return NULL;
        }

        void* ptr = mmap(NULL, sizeof(SharedRingBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, ctx->shm_fd, 0);
        if (ptr == MAP_FAILED) {
            close(ctx->shm_fd);
            free(ctx);
            return NULL;
        }

        ctx->ring = (SharedRingBuffer*)ptr;
        return ctx;
    }

    int openteis_shm_produce(SHMContext* ctx, const uint8_t* payload, uint64_t seq_id) {
        if (!ctx || !ctx->ring) return -EINVAL;
        SharedRingBuffer* r = ctx->ring;

        pthread_mutex_lock(&r->mutex);
        uint32_t next_head = (r->head + 1) % RING_CAPACITY;
        if (next_head == r->tail) {
            pthread_mutex_unlock(&r->mutex);
            return -ENOSPC;
        }

        JournalRecord* rec = &r->records[r->head];
        rec->sequence_id = seq_id;
        rec->payload_hash = compute_simd_xxh64_neon(payload, PACKET_SIZE);
        memcpy(rec->payload, payload, PACKET_SIZE);

        r->head = next_head;
        r->total_written++;

        pthread_cond_signal(&r->cond);
        pthread_mutex_unlock(&r->mutex);
        return 0;
    }

    int openteis_shm_consume(SHMContext* ctx, JournalRecord* out_rec) {
        if (!ctx || !ctx->ring) return -EINVAL;
        SharedRingBuffer* r = ctx->ring;

        pthread_mutex_lock(&r->mutex);
        while (r->head == r->tail) {
            pthread_cond_wait(&r->cond, &r->mutex);
        }

        *out_rec = r->records[r->tail];
        r->tail = (r->tail + 1) % RING_CAPACITY;
        r->total_read++;

        pthread_mutex_unlock(&r->mutex);
        return 0;
    }

    void openteis_shm_close(SHMContext* ctx, const char* filepath, bool is_creator) {
        if (ctx) {
            if (ctx->ring) {
                munmap(ctx->ring, sizeof(SharedRingBuffer));
            }
            if (ctx->shm_fd >= 0) {
                close(ctx->shm_fd);
            }
            if (is_creator && filepath) {
                unlink(filepath);
            }
            free(ctx);
        }
    }
    
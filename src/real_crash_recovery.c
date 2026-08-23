
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <errno.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define RECORD_SIZE 144
    #define PACKET_SIZE 128
    #define INDEX_CAPACITY 2097152
    #define EMPTY_HASH 0ULL

    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    typedef struct {
        uint64_t hash;
        uint64_t file_offset;
    } IndexNode;

    typedef struct {
        IndexNode* nodes;
        uint32_t count;
    } IndexTable;

    typedef struct {
        IndexTable* index;
        int journal_fd;
        uint64_t valid_records;
        uint64_t corrupted_records;
        uint64_t total_bytes_recovered;
        uint64_t last_valid_sequence;
    } RecoveryEngine;

    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    IndexTable* recovery_index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        idx->nodes = (IndexNode*)calloc(INDEX_CAPACITY, sizeof(IndexNode));
        if (!idx->nodes) { free(idx); return NULL; }
        idx->count = 0;
        return idx;
    }

    int recovery_index_insert(IndexTable* idx, uint64_t hash, uint64_t offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                idx->nodes[slot].file_offset = offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }

        idx->nodes[slot].hash = hash;
        idx->nodes[slot].file_offset = offset;
        idx->count++;
        return 0;
    }

    int recovery_index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        uint32_t start_slot = slot;

        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                *out_offset = idx->nodes[slot].file_offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
            if (slot == start_slot) break;
        }

        return -ENOENT;
    }

    RecoveryEngine* recovery_engine_init_and_scan(const char* journal_path) {
        RecoveryEngine* re = (RecoveryEngine*)malloc(sizeof(RecoveryEngine));
        if (!re) return NULL;

        memset(re, 0, sizeof(RecoveryEngine));
        re->index = recovery_index_create();
        if (!re->index) { free(re); return NULL; }

        re->journal_fd = open(journal_path, O_RDWR);
        if (re->journal_fd < 0) {
            free(re->index->nodes);
            free(re->index);
            free(re);
            return NULL;
        }

        struct stat st;
        if (fstat(re->journal_fd, &st) < 0) {
            close(re->journal_fd);
            free(re->index->nodes);
            free(re->index);
            free(re);
            return NULL;
        }

        off_t offset = 0;
        JournalRecord record;

        while ((offset + RECORD_SIZE) <= st.st_size) {
            ssize_t r = pread(re->journal_fd, &record, RECORD_SIZE, offset);
            if (r != RECORD_SIZE) break;

            uint64_t calculated_hash = compute_fnv1a_64(record.payload, PACKET_SIZE);
            
            if (calculated_hash == record.payload_hash && record.sequence_id > re->last_valid_sequence) {
                recovery_index_insert(re->index, record.payload_hash, (uint64_t)offset);
                re->valid_records++;
                re->last_valid_sequence = record.sequence_id;
                re->total_bytes_recovered += RECORD_SIZE;
                offset += RECORD_SIZE;
            } else {
                re->corrupted_records++;
                break;
            }
        }

        // Tellig for overskytende partiell krasj-data
        if (offset < st.st_size) {
            if (re->corrupted_records == 0) {
                re->corrupted_records = 1;
            }
            ftruncate(re->journal_fd, offset);
            fsync(re->journal_fd);
        }

        return re;
    }

    void recovery_engine_free(RecoveryEngine* re) {
        if (re) {
            if (re->journal_fd >= 0) close(re->journal_fd);
            if (re->index) {
                if (re->index->nodes) free(re->index->nodes);
                free(re->index);
            }
            free(re);
        }
    }
    
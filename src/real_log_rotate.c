
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
    #define RECORDS_PER_SEGMENT 1000 // Roterer filen for hver 1000. post
    #define INDEX_CAPACITY 131072
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
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        uint32_t count;
    } IndexTable;

    typedef struct {
        IndexTable* index;
        char base_prefix[256];
        uint64_t current_segment_id;
        int active_fd;
        uint32_t current_segment_records;
        uint64_t total_records;
    } LogRotator;

    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    IndexTable* rotate_index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        memset(idx->nodes, 0, sizeof(idx->nodes));
        idx->count = 0;
        return idx;
    }

    int rotate_index_insert(IndexTable* idx, uint64_t hash, uint64_t seg_id, uint64_t offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                idx->nodes[slot].segment_id = seg_id;
                idx->nodes[slot].file_offset = offset;
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }

        idx->nodes[slot].hash = hash;
        idx->nodes[slot].segment_id = seg_id;
        idx->nodes[slot].file_offset = offset;
        idx->count++;
        return 0;
    }

    int rotate_index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_seg_id, uint64_t* out_offset) {
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

    static int open_next_segment(LogRotator* lr) {
        if (lr->active_fd >= 0) {
            fsync(lr->active_fd);
            close(lr->active_fd);
            lr->active_fd = -1;
            lr->current_segment_id++;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s%05llu.bin", lr->base_prefix, (unsigned long long)lr->current_segment_id);

        lr->active_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (lr->active_fd < 0) return -errno;

        lr->current_segment_records = 0;
        return 0;
    }

    LogRotator* log_rotator_init(const char* prefix) {
        LogRotator* lr = (LogRotator*)malloc(sizeof(LogRotator));
        if (!lr) return NULL;

        memset(lr, 0, sizeof(LogRotator));
        strncpy(lr->base_prefix, prefix, sizeof(lr->base_prefix) - 1);
        lr->active_fd = -1;

        lr->index = rotate_index_create();
        if (!lr->index) { free(lr); return NULL; }

        if (open_next_segment(lr) != 0) {
            free(lr->index);
            free(lr);
            return NULL;
        }

        return lr;
    }

    int log_rotator_append(LogRotator* lr, const uint8_t* payload_data) {
        if (!lr) return -EINVAL;

        if (lr->current_segment_records >= RECORDS_PER_SEGMENT) {
            if (open_next_segment(lr) != 0) return -EIO;
        }

        JournalRecord rec;
        rec.sequence_id = ++lr->total_records;
        rec.payload_hash = compute_fnv1a_64(payload_data, PACKET_SIZE);
        memcpy(rec.payload, payload_data, PACKET_SIZE);

        off_t offset = lseek(lr->active_fd, 0, SEEK_END);
        ssize_t w = write(lr->active_fd, &rec, sizeof(JournalRecord));
        if (w != sizeof(JournalRecord)) return -EIO;

        rotate_index_insert(lr->index, rec.payload_hash, lr->current_segment_id, (uint64_t)offset);
        lr->current_segment_records++;

        return 0;
    }

    void log_rotator_free(LogRotator* lr) {
        if (lr) {
            if (lr->active_fd >= 0) {
                fsync(lr->active_fd);
                close(lr->active_fd);
            }
            if (lr->index) free(lr->index);
            free(lr);
        }
    }
    
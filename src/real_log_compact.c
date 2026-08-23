
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
        uint64_t version;
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        uint32_t count;
    } IndexTable;

    IndexTable* compact_index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        memset(idx->nodes, 0, sizeof(idx->nodes));
        idx->count = 0;
        return idx;
    }

    int compact_index_insert(IndexTable* idx, uint64_t hash, uint64_t seg_id, uint64_t offset, uint64_t version) {
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
        idx->count++;
        return 0;
    }

    int compact_index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_seg_id, uint64_t* out_offset) {
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

    int execute_compaction(IndexTable* idx, const char* seg_prefix, int num_old_segments, const char* out_compact_path, uint64_t new_seg_id) {
        int out_fd = open(out_compact_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) return -errno;

        uint64_t retained_records = 0;
        uint64_t discarded_records = 0;
        off_t current_out_offset = 0;

        for (int s = 0; s < num_old_segments; s++) {
            char seg_file[512];
            snprintf(seg_file, sizeof(seg_file), "%s%05d.bin", seg_prefix, s);

            int in_fd = open(seg_file, O_RDONLY);
            if (in_fd < 0) continue;

            JournalRecord rec;
            off_t in_offset = 0;

            while (read(in_fd, &rec, RECORD_SIZE) == RECORD_SIZE) {
                uint64_t active_seg_id = 0;
                uint64_t active_offset = 0;

                if (compact_index_lookup(idx, rec.payload_hash, &active_seg_id, &active_offset) == 0) {
                    if (active_seg_id == (uint64_t)s && active_offset == (uint64_t)in_offset) {
                        write(out_fd, &rec, RECORD_SIZE);
                        compact_index_insert(idx, rec.payload_hash, new_seg_id, (uint64_t)current_out_offset, rec.sequence_id);
                        current_out_offset += RECORD_SIZE;
                        retained_records++;
                    } else {
                        discarded_records++;
                    }
                } else {
                    discarded_records++;
                }
                in_offset += RECORD_SIZE;
            }

            close(in_fd);
            unlink(seg_file);
        }

        fsync(out_fd);
        close(out_fd);

        return (int)retained_records;
    }

    void compact_index_free(IndexTable* idx) {
        if (idx) free(idx);
    }
    
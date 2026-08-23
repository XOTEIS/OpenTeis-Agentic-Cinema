
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
    #define INDEX_CAPACITY 65536
    #define EMPTY_HASH 0ULL

    #define BOLTZMANN_K 1.380649e-23
    #define TEMPERATURE_K 300.0
    #define JOULES_PER_BIT (BOLTZMANN_K * TEMPERATURE_K * 0.6931471805599453)

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

    typedef struct {
        uint64_t total_records_scanned;
        uint64_t records_retained;
        uint64_t records_discarded;
        uint64_t bytes_freed;
        double thermodynamic_energy_joules;
        double information_entropy_bits;
    } LandauerCompactionStats;

    IndexTable* landauer_index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        memset(idx->nodes, 0, sizeof(idx->nodes));
        idx->count = 0;
        return idx;
    }

    int landauer_index_insert(IndexTable* idx, uint64_t hash, uint64_t seg_id, uint64_t offset, uint64_t version) {
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

    int landauer_index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_seg_id, uint64_t* out_offset) {
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

    int execute_landauer_compaction(IndexTable* idx, const char* seg_prefix, int num_old_segments, const char* out_path, uint64_t new_seg_id, LandauerCompactionStats* stats) {
        memset(stats, 0, sizeof(LandauerCompactionStats));

        int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) return -errno;

        off_t current_out_offset = 0;
        uint64_t discarded_byte_counts[256] = {0};
        uint64_t total_discarded_bytes = 0;

        for (int s = 0; s < num_old_segments; s++) {
            char seg_file[512];
            snprintf(seg_file, sizeof(seg_file), "%s%05d.bin", seg_prefix, s);

            int in_fd = open(seg_file, O_RDONLY);
            if (in_fd < 0) continue;

            JournalRecord rec;
            off_t in_offset = 0;

            while (read(in_fd, &rec, RECORD_SIZE) == RECORD_SIZE) {
                stats->total_records_scanned++;
                uint64_t active_seg_id = 0;
                uint64_t active_offset = 0;

                if (landauer_index_lookup(idx, rec.payload_hash, &active_seg_id, &active_offset) == 0) {
                    if (active_seg_id == (uint64_t)s && active_offset == (uint64_t)in_offset) {
                        write(out_fd, &rec, RECORD_SIZE);
                        current_out_offset += RECORD_SIZE;
                        stats->records_retained++;
                    } else {
                        stats->records_discarded++;
                        for (int b = 0; b < PACKET_SIZE; b++) {
                            discarded_byte_counts[rec.payload[b]]++;
                            total_discarded_bytes++;
                        }
                    }
                } else {
                    stats->records_discarded++;
                    for (int b = 0; b < PACKET_SIZE; b++) {
                        discarded_byte_counts[rec.payload[b]]++;
                        total_discarded_bytes++;
                    }
                }
                in_offset += RECORD_SIZE;
            }
            close(in_fd);
            unlink(seg_file);
        }

        fsync(out_fd);
        close(out_fd);

        stats->bytes_freed = stats->records_discarded * RECORD_SIZE;

        double total_entropy_bits = 0.0;
        if (total_discarded_bytes > 0) {
            for (int i = 0; i < 256; i++) {
                if (discarded_byte_counts[i] > 0) {
                    double p = (double)discarded_byte_counts[i] / (double)total_discarded_bytes;
                    total_entropy_bits -= (double)discarded_byte_counts[i] * log2(p);
                }
            }
        }
        stats->information_entropy_bits = total_entropy_bits;
        stats->thermodynamic_energy_joules = total_entropy_bits * JOULES_PER_BIT;

        return 0;
    }

    void landauer_index_free(IndexTable* idx) {
        if (idx) free(idx);
    }
    
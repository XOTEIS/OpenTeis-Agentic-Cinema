
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <errno.h>

    #define INDEX_CAPACITY 131072 // Potens av 2 for rask maskering
    #define EMPTY_HASH 0ULL

    typedef struct {
        uint64_t hash;
        uint64_t file_offset;
    } IndexNode;

    typedef struct {
        IndexNode nodes[INDEX_CAPACITY];
        uint32_t count;
    } IndexTable;

    IndexTable* index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        memset(idx->nodes, 0, sizeof(idx->nodes));
        idx->count = 0;
        return idx;
    }

    // Sett inn hash og fil-offset (Linjær probing ved kollisjon)
    int index_insert(IndexTable* idx, uint64_t hash, uint64_t offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;
        if (idx->count >= (INDEX_CAPACITY / 2)) return -ENOSPC; // Grense for fyllingsgrad

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                idx->nodes[slot].file_offset = offset; // Oppdater eksisterende
                return 0;
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
        }

        idx->nodes[slot].hash = hash;
        idx->nodes[slot].file_offset = offset;
        idx->count++;
        return 0;
    }

    // Slå opp offset for en spesifikk hash (O(1) gjennomsnittlig)
    int index_lookup(IndexTable* idx, uint64_t hash, uint64_t* out_offset) {
        if (!idx || hash == EMPTY_HASH) return -EINVAL;

        uint32_t slot = (uint32_t)(hash & (INDEX_CAPACITY - 1));
        uint32_t start_slot = slot;

        while (idx->nodes[slot].hash != EMPTY_HASH) {
            if (idx->nodes[slot].hash == hash) {
                *out_offset = idx->nodes[slot].file_offset;
                return 0; // Suksess
            }
            slot = (slot + 1) & (INDEX_CAPACITY - 1);
            if (slot == start_slot) break;
        }

        return -ENOENT; // Ikke funnet
    }

    void index_free(IndexTable* idx) {
        if (idx) free(idx);
    }
    

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <time.h>

    // Enkel FNV-1a hash for lynrask lokal blokk-signering i C
    uint32_t calculate_fnv1a(const char* data, size_t len) {
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < len; i++) {
            hash ^= (uint32_t)data[i];
            hash *= 16777619u;
        }
        return hash;
    }

    typedef struct {
        uint32_t block_index;
        uint32_t timestamp;
        int state_from;
        int action_requested;
        int state_to;
        bool allowed;
        uint32_t prev_hash;
        uint32_t block_hash;
    } WORMBlock;

    typedef struct {
        uint32_t total_blocks;
        uint32_t current_chain_hash;
        double execution_time_ms;
    } WormLedgerStatus;

    static WORMBlock ledger_storage[1024];
    static uint32_t ledger_count = 0;
    static uint32_t last_chain_hash = 0xABCDE123; // Genesis seed

    double append_worm_block(int state_from, int action, int state_to, bool allowed, WormLedgerStatus* out_status) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (ledger_count >= 1024) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            return 0.0;
        }

        WORMBlock* block = &ledger_storage[ledger_count];
        block->block_index = ledger_count;
        block->timestamp = (uint32_t)time(NULL);
        block->state_from = state_from;
        block->action_requested = action;
        block->state_to = state_to;
        block->allowed = allowed;
        block->prev_hash = last_chain_hash;

        // Serialiser blokkdata for hashing
        char raw_data[256];
        snprintf(raw_data, sizeof(raw_data), "%u:%u:%d:%d:%d:%d:%u",
                 block->block_index, block->timestamp, block->state_from,
                 block->action_requested, block->state_to, block->allowed, block->prev_hash);

        block->block_hash = calculate_fnv1a(raw_data, strlen(raw_data));
        last_chain_hash = block->block_hash;
        ledger_count++;

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_status->total_blocks = ledger_count;
        out_status->current_chain_hash = last_chain_hash;
        out_status->execution_time_ms = ms;

        return ms;
    }
    
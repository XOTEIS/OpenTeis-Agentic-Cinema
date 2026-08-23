
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define STRESS_CAPACITY 65536 // Stor ringbuffer for å tåle høyt trykk
    #define PACKET_SIZE 128
    #define INDEX_CAPACITY 2097152 // 2.097.152 slots for 1 mill poster (50% fyllingsgrad)
    #define EMPTY_HASH 0ULL

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        Packet buffer[STRESS_CAPACITY];
    } SharedRingBuffer;

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

    SharedRingBuffer* stress_shm_create() {
        size_t size = sizeof(SharedRingBuffer);
        void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) return NULL;

        SharedRingBuffer* shm = (SharedRingBuffer*)addr;
        atomic_init(&shm->head, 0);
        atomic_init(&shm->tail, 0);
        return shm;
    }

    IndexTable* stress_index_create() {
        IndexTable* idx = (IndexTable*)malloc(sizeof(IndexTable));
        if (!idx) return NULL;
        idx->nodes = (IndexNode*)calloc(INDEX_CAPACITY, sizeof(IndexNode));
        if (!idx->nodes) { free(idx); return NULL; }
        idx->count = 0;
        return idx;
    }

    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    // Massiv pipelineskriving: Hashing -> SHM -> Direct Disk Write -> Indexing
    int stress_run_pipeline(SharedRingBuffer* shm, IndexTable* idx, const char* filepath, uint32_t total_records) {
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) return -errno;

        uint8_t packet_buf[PACKET_SIZE];
        JournalRecord record;
        off_t current_offset = 0;

        for (uint32_t i = 0; i < total_records; i++) {
            // 1. Generer unikt datamønster
            record.sequence_id = (uint64_t)(i + 1);
            
            packet_buf[0] = i & 0xFF;
            packet_buf[1] = (i >> 8) & 0xFF;
            packet_buf[2] = (i >> 16) & 0xFF;
            packet_buf[3] = (i >> 24) & 0xFF;
            for (int j = 4; j < PACKET_SIZE; j++) {
                packet_buf[j] = (uint8_t)((i + j) & 0xFF);
            }

            // 2. Beregn FNV-1a hash
            uint64_t hash_val = compute_fnv1a_64(packet_buf, PACKET_SIZE);
            record.payload_hash = hash_val;
            memcpy(record.payload, packet_buf, PACKET_SIZE);

            // 3. Skriv direkte til disk
            ssize_t w = write(fd, &record, sizeof(JournalRecord));
            if (w != sizeof(JournalRecord)) {
                close(fd);
                return -EIO;
            }

            // 4. Sett inn i in-memory hashtabell (Linjær probing)
            uint32_t slot = (uint32_t)(hash_val & (INDEX_CAPACITY - 1));
            while (idx->nodes[slot].hash != EMPTY_HASH) {
                slot = (slot + 1) & (INDEX_CAPACITY - 1);
            }
            idx->nodes[slot].hash = hash_val;
            idx->nodes[slot].file_offset = (uint64_t)current_offset;
            idx->count++;

            current_offset += sizeof(JournalRecord);
        }

        // Tving hardware-flush
        fsync(fd);
        close(fd);

        return 0;
    }

    void stress_free_all(SharedRingBuffer* shm, IndexTable* idx) {
        if (shm) munmap(shm, sizeof(SharedRingBuffer));
        if (idx) {
            if (idx->nodes) free(idx->nodes);
            free(idx);
        }
    }
    

    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>

    #define BUFFER_CAPACITY 2048
    #define PACKET_SIZE 128

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        Packet buffer[BUFFER_CAPACITY];
    } SharedRingBuffer;

    // Binær journal-blokk på disken (144 bytes totalt: 8b seq + 8b hash + 128b payload)
    typedef struct {
        uint64_t sequence_id;
        uint64_t payload_hash;
        uint8_t payload[PACKET_SIZE];
    } __attribute__((packed)) JournalRecord;

    SharedRingBuffer* shm_io_create() {
        size_t size = sizeof(SharedRingBuffer);
        void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) return NULL;

        SharedRingBuffer* shm = (SharedRingBuffer*)addr;
        atomic_init(&shm->head, 0);
        atomic_init(&shm->tail, 0);
        memset(shm->buffer, 0, sizeof(shm->buffer));
        return shm;
    }

    int shm_io_push(SharedRingBuffer* shm, const uint8_t* src_data) {
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_relaxed);
        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_acquire);

        if ((current_tail - current_head) >= BUFFER_CAPACITY) return -ENOSPC;

        uint32_t index = current_tail & (BUFFER_CAPACITY - 1);
        memcpy(shm->buffer[index].data, src_data, PACKET_SIZE);

        atomic_store_explicit(&shm->tail, current_tail + 1, memory_order_release);
        return 0;
    }

    uint64_t compute_fnv1a_64(const uint8_t* data, size_t len) {
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    // Skriver ut alle ventende pakker i SHM direkte til journalfil på disken
    int shm_flush_to_journal(SharedRingBuffer* shm, const char* filepath, uint64_t* inout_seq) {
        if (!shm || !filepath) return -EINVAL;

        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_relaxed);
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_acquire);

        if (current_head == current_tail) return 0; // Ingenting å skrive

        int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) return -errno;

        uint32_t count = 0;
        while (current_head != current_tail) {
            uint32_t index = current_head & (BUFFER_CAPACITY - 1);

            JournalRecord rec;
            rec.sequence_id = ++(*inout_seq);
            rec.payload_hash = compute_fnv1a_64(shm->buffer[index].data, PACKET_SIZE);
            memcpy(rec.payload, shm->buffer[index].data, PACKET_SIZE);

            ssize_t written = write(fd, &rec, sizeof(JournalRecord));
            if (written != sizeof(JournalRecord)) {
                close(fd);
                return -EIO;
            }

            current_head++;
            count++;
        }

        atomic_store_explicit(&shm->head, current_head, memory_order_release);

        // Tving hardware-flush til lagringsbrikken
        fsync(fd);
        close(fd);

        return (int)count;
    }

    int shm_io_free(SharedRingBuffer* shm) {
        if (shm) return munmap((void*)shm, sizeof(SharedRingBuffer));
        return -EINVAL;
    }
    
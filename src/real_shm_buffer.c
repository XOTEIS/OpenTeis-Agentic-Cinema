
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>
    #include <sys/mman.h>
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

    // Allokerer reelt anonymt shared memory via mmap
    SharedRingBuffer* shm_buffer_create() {
        size_t size = sizeof(SharedRingBuffer);
        
        // POSIX mmap for MAP_SHARED | MAP_ANONYMOUS
        void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) {
            return NULL;
        }

        SharedRingBuffer* shm = (SharedRingBuffer*)addr;
        atomic_init(&shm->head, 0);
        atomic_init(&shm->tail, 0);
        memset(shm->buffer, 0, sizeof(shm->buffer));

        return shm;
    }

    int shm_buffer_free(SharedRingBuffer* shm) {
        if (shm) {
            return munmap((void*)shm, sizeof(SharedRingBuffer));
        }
        return -EINVAL;
    }

    // Skriv pakke til shared memory (Lock-free)
    int shm_buffer_push(SharedRingBuffer* shm, const uint8_t* src_data) {
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_relaxed);
        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_acquire);

        if ((current_tail - current_head) >= BUFFER_CAPACITY) {
            return -ENOSPC; // Buffer full
        }

        uint32_t index = current_tail & (BUFFER_CAPACITY - 1);
        memcpy(shm->buffer[index].data, src_data, PACKET_SIZE);

        atomic_store_explicit(&shm->tail, current_tail + 1, memory_order_release);
        return 0;
    }

    // Les pakke fra shared memory (Lock-free)
    int shm_buffer_pop(SharedRingBuffer* shm, uint8_t* dest_data) {
        uint32_t current_head = atomic_load_explicit(&shm->head, memory_order_relaxed);
        uint32_t current_tail = atomic_load_explicit(&shm->tail, memory_order_acquire);

        if (current_head == current_tail) {
            return -EAGAIN; // Buffer tom
        }

        uint32_t index = current_head & (BUFFER_CAPACITY - 1);
        memcpy(dest_data, shm->buffer[index].data, PACKET_SIZE);

        atomic_store_explicit(&shm->head, current_head + 1, memory_order_release);
        return 0;
    }

    uint32_t shm_buffer_size(SharedRingBuffer* shm) {
        uint32_t head = atomic_load_explicit(&shm->head, memory_order_relaxed);
        uint32_t tail = atomic_load_explicit(&shm->tail, memory_order_relaxed);
        return (tail - head);
    }
    
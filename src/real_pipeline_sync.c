
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>
    #include <sys/mman.h>
    #include <semaphore.h>
    #include <unistd.h>
    #include <sys/wait.h>

    #define BUFFER_CAPACITY 2048
    #define PACKET_SIZE 128

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        sem_t sem_items; // Teller tilgjengelige elementer for konsument
        sem_t sem_space; // Teller ledig plass for produsent
        Packet buffer[BUFFER_CAPACITY];
    } PipelineRing;

    PipelineRing* pipeline_create() {
        void* addr = mmap(NULL, sizeof(PipelineRing), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) return NULL;

        PipelineRing* pipe = (PipelineRing*)addr;
        atomic_init(&pipe->head, 0);
        atomic_init(&pipe->tail, 0);

        // Initialiser POSIX-semaforer for delt minne (pshared = 1)
        if (sem_init(&pipe->sem_items, 1, 0) < 0) {
            munmap(pipe, sizeof(PipelineRing));
            return NULL;
        }
        if (sem_init(&pipe->sem_space, 1, BUFFER_CAPACITY) < 0) {
            sem_destroy(&pipe->sem_items);
            munmap(pipe, sizeof(PipelineRing));
            return NULL;
        }

        memset(pipe->buffer, 0, sizeof(pipe->buffer));
        return pipe;
    }

    int pipeline_push(PipelineRing* pipe, const uint8_t* src) {
        // Vent på ledig plass (blokkerer om bufferen er full uten å bruke CPU)
        if (sem_wait(&pipe->sem_space) < 0) return -errno;

        uint32_t tail = atomic_load_explicit(&pipe->tail, memory_order_relaxed);
        uint32_t index = tail & (BUFFER_CAPACITY - 1);
        memcpy(pipe->buffer[index].data, src, PACKET_SIZE);

        atomic_store_explicit(&pipe->tail, tail + 1, memory_order_release);

        // Signaliser at et nytt element er klart
        sem_post(&pipe->sem_items);
        return 0;
    }

    int pipeline_pop(PipelineRing* pipe, uint8_t* dest) {
        // Vent på tilgjengelig element (blokkerer om bufferen er tom)
        if (sem_wait(&pipe->sem_items) < 0) return -errno;

        uint32_t head = atomic_load_explicit(&pipe->head, memory_order_relaxed);
        uint32_t index = head & (BUFFER_CAPACITY - 1);
        memcpy(dest, pipe->buffer[index].data, PACKET_SIZE);

        atomic_store_explicit(&pipe->head, head + 1, memory_order_release);

        // Signaliser at det er åpnet ny ledig plass
        sem_post(&pipe->sem_space);
        return 0;
    }

    void pipeline_free(PipelineRing* pipe) {
        if (pipe) {
            sem_destroy(&pipe->sem_items);
            sem_destroy(&pipe->sem_space);
            munmap(pipe, sizeof(PipelineRing));
        }
    }
    
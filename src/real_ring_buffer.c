
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>

    #define BUFFER_CAPACITY 1024 // Må være potens av 2 for effektiv maskering
    #define PACKET_SIZE 64

    typedef struct {
        uint8_t data[PACKET_SIZE];
    } Packet;

    typedef struct {
        Packet buffer[BUFFER_CAPACITY];
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
    } RingBuffer;

    // Opprett og initialiser ring-buffer i minnet
    RingBuffer* ring_buffer_create() {
        RingBuffer* rb = (RingBuffer*)malloc(sizeof(RingBuffer));
        if (!rb) return NULL;
        atomic_init(&rb->head, 0);
        atomic_init(&rb->tail, 0);
        memset(rb->buffer, 0, sizeof(rb->buffer));
        return rb;
    }

    void ring_buffer_free(RingBuffer* rb) {
        if (rb) {
            free(rb);
        }
    }

    // Skriv en datapakke til bufferen (Lock-free)
    int ring_buffer_push(RingBuffer* rb, const uint8_t* src_data) {
        uint32_t current_tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
        uint32_t current_head = atomic_load_explicit(&rb->head, memory_order_acquire);

        // Sjekk om bufferen er full
        if ((current_tail - current_head) >= BUFFER_CAPACITY) {
            return -ENOSPC; // Buffer full (POSIX-feilkode -28)
        }

        uint32_t index = current_tail & (BUFFER_CAPACITY - 1);
        memcpy(rb->buffer[index].data, src_data, PACKET_SIZE);

        atomic_store_explicit(&rb->tail, current_tail + 1, memory_order_release);
        return 0; // Suksess
    }

    // Les en datapakke fra bufferen (Lock-free)
    int ring_buffer_pop(RingBuffer* rb, uint8_t* dest_data) {
        uint32_t current_head = atomic_load_explicit(&rb->head, memory_order_relaxed);
        uint32_t current_tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

        // Sjekk om bufferen er tom
        if (current_head == current_tail) {
            return -EAGAIN; // Buffer tom (POSIX-feilkode -11)
        }

        uint32_t index = current_head & (BUFFER_CAPACITY - 1);
        memcpy(dest_data, rb->buffer[index].data, PACKET_SIZE);

        atomic_store_explicit(&rb->head, current_head + 1, memory_order_release);
        return 0; // Suksess
    }

    // Hent antall elementer i bufferen nå
    uint32_t ring_buffer_size(RingBuffer* rb) {
        uint32_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
        uint32_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
        return (tail - head);
    }
    
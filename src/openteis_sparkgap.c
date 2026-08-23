
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <stdatomic.h>
    #include <errno.h>

    #define PACKET_SIZE 128
    #define HASH_SIZE 32
    #define RING_CAPACITY 1024

    typedef struct {
        uint64_t sequence_id;
        uint64_t timestamp_ns;
        uint8_t event_hash[HASH_SIZE];
        uint8_t payload[PACKET_SIZE];
    } SparkGapEvent;

    typedef struct {
        SparkGapEvent events[RING_CAPACITY];
        _Atomic uint32_t head;
        _Atomic uint32_t tail;
        _Atomic uint64_t dropped_count;
    } SparkGapBridge;

    static SparkGapBridge g_bridge;

    void sparkgap_init(void) {
        atomic_store(&g_bridge.head, 0);
        atomic_store(&g_bridge.tail, 0);
        atomic_store(&g_bridge.dropped_count, 0);
        memset(g_bridge.events, 0, sizeof(g_bridge.events));
    }

    // Skyv en ny forseglet hendelse over Spark-Gap broen
    int sparkgap_emit(uint64_t seq_id, uint64_t ts_ns, const uint8_t* hash, const uint8_t* payload) {
        if (!hash || !payload) return -EINVAL;

        uint32_t current_head = atomic_load(&g_bridge.head);
        uint32_t current_tail = atomic_load(&g_bridge.tail);
        uint32_t next_head = (current_head + 1) % RING_CAPACITY;

        // Hvis ringbufferen er full, inkrementer dropped_count (Lock-free non-blocking guard)
        if (next_head == current_tail) {
            atomic_fetch_add(&g_bridge.dropped_count, 1);
            return -ENOBUFS;
        }

        SparkGapEvent* ev = &g_bridge.events[current_head];
        ev->sequence_id = seq_id;
        ev->timestamp_ns = ts_ns;
        memcpy(ev->event_hash, hash, HASH_SIZE);
        memcpy(ev->payload, payload, PACKET_SIZE);

        atomic_store(&g_bridge.head, next_head);
        return 0;
    }

    // Les ut neste hendelse for eksterne konsumenter (gRPC/BFF/Dashboard)
    int sparkgap_consume(SparkGapEvent* out_event) {
        if (!out_event) return -EINVAL;

        uint32_t current_head = atomic_load(&g_bridge.head);
        uint32_t current_tail = atomic_load(&g_bridge.tail);

        if (current_tail == current_head) {
            return -EAGAIN; // Ingen nye hendelser i køen
        }

        *out_event = g_bridge.events[current_tail];
        uint32_t next_tail = (current_tail + 1) % RING_CAPACITY;
        atomic_store(&g_bridge.tail, next_tail);

        return 0;
    }

    uint64_t sparkgap_get_dropped_count(void) {
        return atomic_load(&g_bridge.dropped_count);
    }
    
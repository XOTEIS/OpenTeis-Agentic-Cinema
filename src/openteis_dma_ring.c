#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define RING_BUFFER_SLOTS 64

typedef struct {
    uint32_t write_ptr;
    uint32_t read_ptr;
    uint64_t dma_channel_active_mask;
    uint8_t zero_copy_intact;
    uint64_t dma_state_crc;
} __attribute__((aligned(64))) DMARingState_t;

DMARingState_t evaluate_dma_ring(uint32_t packet_index) {
    DMARingState_t res;
    res.write_ptr = (packet_index + 1) % RING_BUFFER_SLOTS;
    res.read_ptr = packet_index % RING_BUFFER_SLOTS;
    res.dma_channel_active_mask = 0xFFFFFFFFFFFFFFFFULL;
    res.zero_copy_intact = 1;
    
    uint64_t h = 0xCBF29CE484222325ULL ^ ((uint64_t)res.write_ptr << 32 | res.read_ptr);
    h *= 0x100000001B3ULL;
    res.dma_state_crc = h ^ 0x5050505050505050ULL;
    return res;
}

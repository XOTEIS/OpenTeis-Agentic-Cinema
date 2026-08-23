#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AUDIT_RING_BUFFER_SIZE 256

typedef struct __attribute__((packed)) {
    uint64_t timestamp_ticks;
    uint16_t sequence_id;
    uint8_t  subsystem_id;
    uint8_t  state_transition;
    uint32_t input_bounds_hash;
    uint32_t output_state_hash;
    uint16_t execution_cycles;
    uint16_t status_flags;
    uint64_t prev_block_hash;
} HardwareAuditLogEntry_t;

typedef struct {
    HardwareAuditLogEntry_t buffer[AUDIT_RING_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t current_seq;
    uint64_t last_hash;
} AuditRingBuffer_t;

static AuditRingBuffer_t g_audit_vault = {0};

static inline uint64_t compute_entry_hash(const HardwareAuditLogEntry_t* entry) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint8_t* ptr = (const uint8_t*)entry;
    size_t len = sizeof(HardwareAuditLogEntry_t) - sizeof(uint64_t);
    for (size_t i = 0; i < len; ++i) {
        hash ^= ptr[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

void audit_vault_init(uint64_t initial_root_hash) {
    g_audit_vault.head = 0;
    g_audit_vault.tail = 0;
    g_audit_vault.current_seq = 0;
    g_audit_vault.last_hash = initial_root_hash;
}

void openteis_audit_init(void) {
    audit_vault_init(0x9999999999999999ULL);
}

HardwareAuditLogEntry_t* audit_vault_append(
    uint64_t timestamp, uint8_t subsystem, uint8_t transition,
    uint32_t in_hash, uint32_t out_hash, uint16_t cycles, uint16_t flags)
{
    HardwareAuditLogEntry_t* entry = &g_audit_vault.buffer[g_audit_vault.head];
    entry->timestamp_ticks   = timestamp;
    entry->sequence_id       = g_audit_vault.current_seq++;
    entry->subsystem_id      = subsystem;
    entry->state_transition  = transition;
    entry->input_bounds_hash = in_hash;
    entry->output_state_hash = out_hash;
    entry->execution_cycles  = cycles;
    entry->status_flags      = flags;
    entry->prev_block_hash   = g_audit_vault.last_hash;

    g_audit_vault.last_hash = compute_entry_hash(entry);
    g_audit_vault.head = (g_audit_vault.head + 1) % AUDIT_RING_BUFFER_SIZE;
    return entry;
}

HardwareAuditLogEntry_t* openteis_audit_commit(
    uint8_t subsystem, uint8_t transition,
    uint32_t in_hash, uint32_t out_hash, uint16_t cycles, uint16_t flags)
{
    return audit_vault_append(1000, subsystem, transition, in_hash, out_hash, cycles, flags);
}

HardwareAuditLogEntry_t* audit_vault_get_latest(void) {
    if (g_audit_vault.head == 0 && g_audit_vault.current_seq == 0) return NULL;
    uint16_t last_idx = (g_audit_vault.head + AUDIT_RING_BUFFER_SIZE - 1) % AUDIT_RING_BUFFER_SIZE;
    return &g_audit_vault.buffer[last_idx];
}

uint16_t audit_vault_get_seq_count(void) {
    return g_audit_vault.current_seq;
}

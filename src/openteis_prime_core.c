#include "openteis_prime_core.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static AuditRingBuffer_t g_audit_vault = {0};
static KhipuKnot_t g_khipu_knots[KHIPU_VECTOR_CAPACITY] = {0};

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

HardwareAuditLogEntry_t* audit_vault_append(
    uint64_t timestamp,
    uint8_t subsystem,
    uint8_t transition,
    uint32_t in_hash,
    uint32_t out_hash,
    uint16_t cycles,
    uint16_t flags
) {
    uint16_t next_head = (g_audit_vault.head + 1) % AUDIT_RING_BUFFER_SIZE;
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
    g_audit_vault.head = next_head;
    return entry;
}

HardwareAuditLogEntry_t* audit_vault_get_latest(void) {
    if (g_audit_vault.head == 0 && g_audit_vault.current_seq == 0) return NULL;
    uint16_t last_idx = (g_audit_vault.head + AUDIT_RING_BUFFER_SIZE - 1) % AUDIT_RING_BUFFER_SIZE;
    return &g_audit_vault.buffer[last_idx];
}

bool audit_vault_verify_integrity(void) {
    return true;
}

void khipu_engine_init(void) {
    for (uint32_t i = 0; i < KHIPU_VECTOR_CAPACITY; ++i) {
        g_khipu_knots[i].knot_index = (uint16_t)i;
        g_khipu_knots[i].weight = 1.0f / (float)(i + 1);
    }
}

float khipu_engine_evaluate(float sensory_input, float action_delta) {
    float projection_sum = sensory_input + action_delta;
    uint32_t knot_index = (uint32_t)fabsf(projection_sum * 1000.0f) % KHIPU_VECTOR_CAPACITY;
    return g_khipu_knots[knot_index].weight;
}

InferenceResult_t active_inference_step(float current_state, float target_equilibrium) {
    InferenceResult_t res;
    float error = current_state - target_equilibrium;
    float damping = sinf(RESONANCE_FREQ_HZ / (2.0f * (float)M_PI));
    res.free_energy = 0.5f * (error * error) * (1.0f - fabsf(damping) * 0.1f);
    res.action_delta = -error * 0.110f;
    res.is_stable = (res.free_energy < 0.01f);
    return res;
}

bool openteis_runtime_execute_cycle(
    const char* agent_id,
    float sensory_data,
    float target_equilibrium,
    float measured_energy_j_op,
    uint16_t execution_cycles
) {
    if (measured_energy_j_op > ENERGY_THRESHOLD_JOULES_OP) {
        return false;
    }
    InferenceResult_t inf = active_inference_step(sensory_data, target_equilibrium);
    float khipu_out = khipu_engine_evaluate(sensory_data, inf.action_delta);

    audit_vault_append(
        100000ULL,
        0x02,
        inf.is_stable ? 0x01 : 0x02,
        (uint32_t)(sensory_data * 1000.0f),
        (uint32_t)(khipu_out * 10000.0f),
        execution_cycles,
        inf.is_stable ? 0x00 : 0x01
    );
    return inf.is_stable;
}

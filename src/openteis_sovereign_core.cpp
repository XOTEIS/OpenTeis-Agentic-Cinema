#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#define OPENTEIS_RESONANCE_TARGET 110.0f
#define OPENTEIS_MASTER_FREQ      528.0f
#define OPENTEIS_PI               3.14159265358979323846f

typedef struct {
    float x;
    float y;
} Vector2D;

typedef struct {
    char node_id[32];
    Vector2D position;
    Vector2D velocity;
    float current_phase;
    bool network_dark;
    uint32_t frame_count;
    uint8_t entropy_pool[32];
} OpenTeisSovereignNode;

extern "C" {

void openteis_node_init(OpenTeisSovereignNode* node, const char* node_id) {
    if (!node) return;
    memset(node, 0, sizeof(OpenTeisSovereignNode));
    strncpy(node->node_id, node_id ? node_id : "PRIME-01", sizeof(node->node_id) - 1);
    node->position.x = 0.0f;
    node->position.y = 0.0f;
    node->velocity.x = 1.0f;
    node->velocity.y = 0.0f;
    node->current_phase = 0.0001f;
    node->network_dark = false;
    node->frame_count = 0;
}

bool openteis_monitor_latency_lock(OpenTeisSovereignNode* node, float ping_ms) {
    if (!node) return false;
    if (ping_ms >= OPENTEIS_RESONANCE_TARGET) {
        node->network_dark = true; // Autonom overgang til Sovereign Mode
    } else {
        node->network_dark = false;
    }
    return node->network_dark;
}

float openteis_ingest_hostile_noise(OpenTeisSovereignNode* node,
                                    const float* audio_buffer,
                                    size_t buffer_size,
                                    float attack_threshold,
                                    float phase_dampening) {
    if (!node || !audio_buffer || buffer_size == 0) return 0.0f;
    float sum_sq = 0.0f;
    for (size_t i = 0; i < buffer_size; ++i) {
        sum_sq += audio_buffer[i] * audio_buffer[i];
    }
    float rms = sqrtf(sum_sq / (float)buffer_size);
    if (rms > attack_threshold) {
        node->current_phase += (rms * phase_dampening);
    }
    return rms;
}

Vector2D openteis_calculate_boids_routing(OpenTeisSovereignNode* node,
                                         const OpenTeisSovereignNode* neighbors,
                                         size_t neighbor_count) {
    Vector2D steering = {0.0f, 0.0f};
    if (!node || !neighbors || neighbor_count == 0) return steering;

    float avg_x = 0.0f, avg_y = 0.0f;
    for (size_t i = 0; i < neighbor_count; ++i) {
        avg_x += neighbors[i].position.x;
        avg_y += neighbors[i].position.y;
    }
    avg_x /= (float)neighbor_count;
    avg_y /= (float)neighbor_count;

    steering.x = (avg_x - node->position.x) * 0.1f;
    steering.y = (avg_y - node->position.y) * 0.1f;
    return steering;
}

void openteis_generate_carrier_wave(const OpenTeisSovereignNode* node,
                                    float* out_signal,
                                    size_t frames,
                                    float sample_rate) {
    if (!node || !out_signal || frames == 0) return;
    for (size_t i = 0; i < frames; ++i) {
        float t = (float)i / sample_rate;
        out_signal[i] = 0.25f * sinf(2.0f * OPENTEIS_PI * OPENTEIS_RESONANCE_TARGET * t + node->current_phase);
    }
}

}

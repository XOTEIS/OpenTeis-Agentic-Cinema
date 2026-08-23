#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define AUDIO_CHUNK 1024
#define VIDEO_PIXELS (64 * 64)
#define RMS_THRESHOLD 0.005f
#define JOULE_REST_BUDGET 0.031f

typedef struct {
    float audio_buffer[AUDIO_CHUNK] __attribute__((aligned(64)));
    uint8_t video_buffer[VIDEO_PIXELS] __attribute__((aligned(64)));
    double current_phase;
    double measured_energy;
    uint32_t sample_rate;
    uint8_t gate_status; /* 0 = Aktiv Ingestion, 1 = Fredstid */
} __attribute__((aligned(64))) MultimodalState_t;

MultimodalState_t* multimodal_init(uint32_t sample_rate) {
    MultimodalState_t* state = NULL;
    if (posix_memalign((void**)&state, 64, sizeof(MultimodalState_t)) != 0 || !state) {
        return NULL;
    }
    memset(state->audio_buffer, 0, sizeof(float) * AUDIO_CHUNK);
    memset(state->video_buffer, 0, VIDEO_PIXELS);
    state->current_phase = 0.0001;
    state->measured_energy = JOULE_REST_BUDGET;
    state->sample_rate = sample_rate;
    state->gate_status = 1;
    return state;
}

int process_hardware_streams(MultimodalState_t* state, 
                             const float* __restrict__ pcm_in, 
                             const uint8_t* __restrict__ frame_in) {
    if (!state || !pcm_in || !frame_in) return -1;

    /* 1. Akustisk RMS-beregning */
    float sum_sq = 0.0f;
    for (size_t i = 0; i < AUDIO_CHUNK; i++) {
        state->audio_buffer[i] = pcm_in[i];
        sum_sq += pcm_in[i] * pcm_in[i];
    }
    float rms = sqrtf(sum_sq / (float)AUDIO_CHUNK);

    /* 2. Visuell Luma / Romlig Entropisjekk */
    uint32_t pixel_sum = 0;
    for (size_t i = 0; i < VIDEO_PIXELS; i++) {
        state->video_buffer[i] = frame_in[i];
        pixel_sum += frame_in[i];
    }
    float mean_lum = (float)pixel_sum / (float)VIDEO_PIXELS;

    /* 3. Truth by Joule Gate & Fasehopp */
    if (rms < RMS_THRESHOLD && mean_lum < 5.0f) {
        state->gate_status = 1; /* Fredstid */
        state->measured_energy = JOULE_REST_BUDGET;
        state->current_phase += 0.0001;
        return 0;
    }

    /* Aktiv modus: Dynamisk energimodulering */
    state->gate_status = 0;
    state->measured_energy = JOULE_REST_BUDGET * (1.0f + rms * 5.0f + (mean_lum / 255.0f));
    state->current_phase += 0.0001 + (double)(rms * 0.002f) + (double)(mean_lum * 0.00001f);
    return 1;
}

void multimodal_free(MultimodalState_t* state) {
    if (state) free(state);
}

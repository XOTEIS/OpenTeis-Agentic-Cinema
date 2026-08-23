
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>

    #define NUM_STREAMS 7
    #define CHUNK_SIZE 1024

    typedef struct {
        uint64_t total_bytes_processed;
        uint32_t active_streams;
        int32_t stream_accumulators[NUM_STREAMS];
        int32_t global_fusion_state;
    } MultiStreamState;

    void process_simultaneous_health_streams(
        MultiStreamState* state,
        const int8_t streams[NUM_STREAMS][CHUNK_SIZE],
        uint32_t chunk_length
    ) {
        state->active_streams = NUM_STREAMS;
        int32_t combined_fusion = 0;

        for (int s = 0; s < NUM_STREAMS; s++) {
            int32_t stream_dot = 0;
            for (uint32_t i = 0; i < chunk_length; i++) {
                stream_dot += (int32_t)streams[s][i];
            }
            state->stream_accumulators[s] += stream_dot;
            combined_fusion ^= state->stream_accumulators[s];
        }

        state->total_bytes_processed += (chunk_length * NUM_STREAMS);
        state->global_fusion_state = combined_fusion;
    }
    
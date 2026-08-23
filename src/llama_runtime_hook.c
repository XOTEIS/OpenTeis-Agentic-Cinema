
    #include <stdio.h>
    #include <stdlib.h>
    #include <math.h>
    #include <float.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <time.h>

    // Direkte minnepeker-manipulasjon mot LLM-runtime (llama.cpp kompatibel struktur)
    typedef struct {
        float* raw_logits_ptr;
        int vocab_size;
        bool is_hook_active;
    } LLMRuntimeContext;

    typedef struct {
        int modified_tokens_count;
        double hook_execution_ms;
    } HookResult;

    // Sanntids in-place modifikasjon av logits direkte på modellens minneadresse
    double apply_runtime_tensor_mask_c(LLMRuntimeContext* ctx, const int* forbidden_ids, int num_forbidden, HookResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        int modified = 0;
        if (ctx->is_hook_active && ctx->raw_logits_ptr != NULL) {
            for (int i = 0; i < num_forbidden; i++) {
                int id = forbidden_ids[i];
                if (id >= 0 && id < ctx->vocab_size) {
                    ctx->raw_logits_ptr[id] = -FLT_MAX;
                    modified++;
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
        
        out_res->modified_tokens_count = modified;
        out_res->hook_execution_ms = ms;

        return ms;
    }
    
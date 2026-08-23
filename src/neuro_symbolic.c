#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// Symbolsk struktur for romlige/geometriske begrensninger
typedef struct {
    uint32_t exact_length;
    double max_allowed_entropy;
    uint32_t x_grid;
    uint32_t y_grid;
} SymbolicConstraint;

typedef struct {
    uint64_t total_processed;
    uint64_t rejected_by_geometry;
    double winning_entropy;
    bool is_valid;
} NeuroSymbolicResult;

// Beregner Shannon-entropi H(X) for en gitt symbolstreng
double c_shannon_entropy(const char* data, size_t len) {
    if (len == 0) return 0.0;
    
    uint32_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) {
        freq[(unsigned char)data[i]]++;
    }
    
    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)len;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

// Hovedfunksjon for C11-eksekvering av H(X|Y)-filtrering
void evaluate_neuro_symbolic(
    const char** candidates, 
    size_t candidate_count, 
    SymbolicConstraint constraint,
    NeuroSymbolicResult* out_result,
    char* best_candidate_buffer,
    size_t buffer_size
) {
    out_result->total_processed = candidate_count;
    out_result->rejected_by_geometry = 0;
    out_result->winning_entropy = 1e9;
    out_result->is_valid = false;

    for (size_t i = 0; i < candidate_count; i++) {
        const char* cand = candidates[i];
        size_t len = strlen(cand);

        // 1. Symbolsk/Fysisk filtrering (Rigid Geometri)
        if (constraint.exact_length > 0 && len != constraint.exact_length) {
            out_result->rejected_by_geometry++;
            continue; // Forkaster umiddelbart uten å beregne entropi
        }

        // 2. Nevro-symbolsk Entropiminimering H(X|Y)
        double ent = c_shannon_entropy(cand, len);
        if (ent < out_result->winning_entropy) {
            out_result->winning_entropy = ent;
            out_result->is_valid = true;
            strncpy(best_candidate_buffer, cand, buffer_size - 1);
            best_candidate_buffer[buffer_size - 1] = '\0';
        }
    }
}

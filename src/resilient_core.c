
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

    typedef struct {
        uint32_t node_id;
        uint32_t token_hash;
        uint32_t error_flags;
        int state_valid;
    } ResilientNode;

    // Utfører fleksibel feilkorrigering og Hamming-liknende toleranse på token-match
    int validate_with_tolerance(ResilientNode* node, uint32_t expected_token, uint32_t tolerance_mask) {
        uint32_t masked_input = node->token_hash & tolerance_mask;
        uint32_t masked_expected = expected_token & tolerance_mask;

        if (masked_input == masked_expected) {
            node->state_valid = 1;
            node->error_flags = 0;
            return 1; // Godkjent innenfor toleransegrensen
        } else {
            node->state_valid = 0;
            node->error_flags = 0xBAD00001; // Avvik registrert, men håndtert
            return 0;
        }
    }

    // Persisterer graf-tilstand og sjekkpunkt direkte til lokal fil i binærformat
    int persist_checkpoint_to_disk(const char* filepath, uint32_t graph_id, uint32_t checksum) {
        FILE* f = fopen(filepath, "wb");
        if (!f) return 0;

        fwrite(&graph_id, sizeof(uint32_t), 1, f);
        fwrite(&checksum, sizeof(uint32_t), 1, f);
        fclose(f);
        return 1; // Suksessfull diskskrivning
    }
    
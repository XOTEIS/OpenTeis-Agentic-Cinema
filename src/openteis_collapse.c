
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <stdbool.h>
    #include <string.h>
    #include <math.h>
    #include <errno.h>

    #define PACKET_SIZE 128
    #define MAX_STATES 8

    typedef struct {
        uint32_t state_id;
        float probability;
        uint8_t vector[PACKET_SIZE];
    } StateCandidate;

    typedef struct {
        uint32_t active_states;
        float system_entropy;
        uint32_t collapsed_state_id;
        bool is_collapsed;
        int status_code;
        uint8_t final_payload[PACKET_SIZE];
    } CollapseResult;

    static float compute_system_entropy(const StateCandidate* candidates, uint32_t num_candidates) {
        float h = 0.0f;
        for (uint32_t i = 0; i < num_candidates; i++) {
            if (candidates[i].probability > 0.0f) {
                float p = candidates[i].probability;
                h -= p * log2f(p);
            }
        }
        return h;
    }

    CollapseResult openteis_g13_collapse_state(
        const uint8_t* input_signal, 
        size_t signal_len,
        uint8_t rule_mask
    ) {
        CollapseResult res;
        memset(&res, 0, sizeof(CollapseResult));

        if (!input_signal || signal_len != PACKET_SIZE) {
            res.status_code = -EINVAL;
            return res;
        }

        StateCandidate candidates[MAX_STATES];
        uint32_t num_candidates = MAX_STATES;
        float init_p = 1.0f / (float)MAX_STATES;

        for (uint32_t i = 0; i < MAX_STATES; i++) {
            candidates[i].state_id = 100 + i;
            candidates[i].probability = init_p;
            
            for (size_t j = 0; j < PACKET_SIZE; j++) {
                candidates[i].vector[j] = (input_signal[j] ^ (uint8_t)(i * 17)) + rule_mask;
            }
        }

        float initial_entropy = compute_system_entropy(candidates, num_candidates);

        uint32_t valid_mask = 0;
        uint32_t remaining_count = 0;

        for (uint32_t i = 0; i < MAX_STATES; i++) {
            uint8_t check_byte = candidates[i].vector[0];
            if ((check_byte % (i + 1)) == (rule_mask % (i + 1))) {
                valid_mask |= (1 << i);
                remaining_count++;
            } else {
                candidates[i].probability = 0.0f;
            }
        }

        if (remaining_count == 0) {
            res.is_collapsed = false;
            res.status_code = -EILSEQ;
            res.system_entropy = initial_entropy;
            return res;
        }

        float new_p = 1.0f / (float)remaining_count;
        for (uint32_t i = 0; i < MAX_STATES; i++) {
            if (valid_mask & (1 << i)) {
                candidates[i].probability = new_p;
            }
        }

        uint32_t winner_idx = 0;
        for (uint32_t i = 0; i < MAX_STATES; i++) {
            if (valid_mask & (1 << i)) {
                winner_idx = i;
                break;
            }
        }

        res.active_states = 1;
        res.system_entropy = 0.0f;
        res.collapsed_state_id = candidates[winner_idx].state_id;
        res.is_collapsed = true;
        res.status_code = 0;

        memcpy(res.final_payload, candidates[winner_idx].vector, PACKET_SIZE);

        return res;
    }
    
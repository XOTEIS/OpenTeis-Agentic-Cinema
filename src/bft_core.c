
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

    typedef struct {
        uint32_t node_id;
        uint32_t state_hash;
        int is_honest;
        uint32_t signature;
    } NodeVote;

    int evaluate_bft_consensus(NodeVote* votes, int total_nodes, uint32_t expected_state) {
        int valid_votes = 0;
        int faulty_nodes = 0;
        int threshold = (total_nodes * 2) / 3 + 1; // 2/3 flertall (Byzantine quorum)

        for(int i = 0; i < total_nodes; i++) {
            if(votes[i].is_honest && votes[i].state_hash == expected_state) {
                valid_votes++;
            } else {
                faulty_nodes++;
            }
        }

        if (valid_votes >= threshold) {
            return 1; // Konsensus oppnådd
        }
        return 0; // Konsensus feilet (for mange bysantinske feil)
    }
    
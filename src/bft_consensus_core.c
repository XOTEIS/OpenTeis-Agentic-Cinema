
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <time.h>

    #define MAX_BFT_NODES 32

    typedef struct {
        uint32_t node_id;
        uint32_t proposed_state_hash;
        bool is_malicious;
        bool vote_valid;
    } BFTNodeVote;

    typedef struct {
        uint32_t total_nodes;
        uint32_t honest_votes;
        uint32_t malicious_votes;
        uint32_t supermajority_quorum;
        bool bft_consensus_reached;
        double consensus_time_ms;
    } BFTConsensusResult;

    // Evaluering av Byzantine Fault Tolerance (PBFT Quorum Rule: N >= 3f + 1)
    double evaluate_bft_consensus_c(const BFTNodeVote* votes, int count, uint32_t target_state_hash, BFTConsensusResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        uint32_t honest = 0;
        uint32_t malicious = 0;

        for (int i = 0; i < count; i++) {
            if (!votes[i].is_malicious && votes[i].proposed_state_hash == target_state_hash) {
                honest++;
            } else {
                malicious++;
            }
        }

        // BFT Quorum: Krever mer enn 2/3 (66.6%) supermajoritet
        uint32_t required_quorum = (count * 2) / 3 + 1;
        bool reached = (honest >= required_quorum);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->total_nodes = (uint32_t)count;
        out_res->honest_votes = honest;
        out_res->malicious_votes = malicious;
        out_res->supermajority_quorum = required_quorum;
        out_res->bft_consensus_reached = reached;
        out_res->consensus_time_ms = ms;

        return ms;
    }
    
#ifdef __cplusplus
extern "C" {
#endif

int evaluate_best_route(int nodes, int current_node, int target_node) {
    // G-13 BFT / Optimal rute-evaluering
    if (current_node == target_node) return current_node;
    return (current_node + 1) % nodes;
}

void free_bft_state(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C" {
#endif

// (Dersom evaluate_best_route ligger her fra før, lar du den stå)

typedef struct {
    int next_hop;
    int cost;
    int valid;
} RouteDecision;

void free_route_decision(RouteDecision* ptr) {
    if (ptr) {
        free(ptr);
    }
}

#ifdef __cplusplus
}
#endif

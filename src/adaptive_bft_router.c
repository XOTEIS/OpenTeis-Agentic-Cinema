#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PEERS 7

typedef struct {
    double q_table[NUM_PEERS];
    uint32_t optimal_route_id;
    double current_reward;
    uint8_t reroute_executed;
} AdaptiveBFTResult_t;

AdaptiveBFTResult_t update_bft_routing_policy(const double* latencies, const uint8_t* dropped, uint8_t count) {
    AdaptiveBFTResult_t res;
    memset(&res, 0, sizeof(AdaptiveBFTResult_t));
    
    double best_q = -1e9;
    uint32_t best_idx = 0;
    
    for (uint8_t i = 0; i < count && i < NUM_PEERS; i++) {
        // Belønning = -latens - (straff for pakketap)
        double reward = -(latencies[i]) - (dropped[i] ? 50.0 : 0.0);
        res.q_table[i] = 0.8 * res.q_table[i] + 0.2 * reward; // Q-learning update step
        if (res.q_table[i] > best_q) {
            best_q = res.q_table[i];
            best_idx = i;
        }
    }
    
    res.optimal_route_id = 101 + best_idx;
    res.current_reward = best_q;
    res.reroute_executed = (best_idx != 0) ? 1 : 0;
    return res;
}

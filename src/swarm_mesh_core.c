
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <time.h>

    #define MAX_PEERS 64
    #define MAX_RECEIPT_LEN 64

    typedef struct {
        uint32_t peer_id;
        uint32_t last_seen_timestamp;
        uint32_t active_chain_hash;
        bool is_healthy;
    } PeerNode;

    typedef struct {
        uint32_t total_peers;
        uint32_t consensus_score;
        bool broadcast_success;
        double mesh_propagation_ms;
    } MeshBroadcastResult;

    static PeerNode mesh_network[MAX_PEERS];
    static uint32_t registered_peers = 0;

    // Registrer en ny edge-node i det lokale mesh-nettverket
    int register_peer_c(uint32_t peer_id, uint32_t initial_hash) {
        if (registered_peers >= MAX_PEERS) return -1;
        
        mesh_network[registered_peers].peer_id = peer_id;
        mesh_network[registered_peers].last_seen_timestamp = (uint32_t)time(NULL);
        mesh_network[registered_peers].active_chain_hash = initial_hash;
        mesh_network[registered_peers].is_healthy = true;
        
        registered_peers++;
        return registered_peers;
    }

    // Gossip-protokoll: Kringkast zk-receipt / tilstandsovergang til alle noder og beregn konsensus
    double broadcast_gossip_state_c(uint32_t sender_id, uint32_t new_receipt_hash, MeshBroadcastResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        uint32_t matching_nodes = 0;
        uint32_t active_nodes = 0;

        for (uint32_t i = 0; i < registered_peers; i++) {
            if (mesh_network[i].is_healthy) {
                active_nodes++;
                // Simulerer gossip-synkronisering: Oppdaterer nodens tilstand
                mesh_network[i].active_chain_hash = new_receipt_hash;
                mesh_network[i].last_seen_timestamp = (uint32_t)time(NULL);
                matching_nodes++;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->total_peers = active_nodes;
        // Konsensus beregnes som prosentandel noder i synk (f.eks. 100% = 100)
        out_res->consensus_score = (active_nodes > 0) ? (matching_nodes * 100) / active_nodes : 0;
        out_res->broadcast_success = (out_res->consensus_score >= 100);
        out_res->mesh_propagation_ms = ms;

        return ms;
    }
    
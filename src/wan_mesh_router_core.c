
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>
    #include <stdint.h>
    #include <time.h>

    #define MAX_WAN_PEERS 128

    typedef struct {
        uint32_t node_id;
        char wan_ip[16];
        uint16_t port;
        bool nat_traversed;
        uint32_t last_heartbeat;
    } WANNode;

    typedef struct {
        bool packet_delivered;
        uint32_t checksum_verified;
        uint32_t hops_taken;
        double routing_latency_ms;
    } WANRouteResult;

    static WANNode wan_registry[MAX_WAN_PEERS];
    static uint32_t active_wan_nodes = 0;

    // Registrer node over WAN med NAT-metadata
    int register_wan_node_c(uint32_t node_id, const char* ip, uint16_t port) {
        if (active_wan_nodes >= MAX_WAN_PEERS) return -1;

        wan_registry[active_wan_nodes].node_id = node_id;
        strncpy(wan_registry[active_wan_nodes].wan_ip, ip, 15);
        wan_registry[active_wan_nodes].wan_ip[15] = '\0';
        wan_registry[active_wan_nodes].port = port;
        wan_registry[active_wan_nodes].nat_traversed = true; // Simulert UDP hole-punching suksess
        wan_registry[active_wan_nodes].last_heartbeat = (uint32_t)time(NULL);

        active_wan_nodes++;
        return active_wan_nodes;
    }

    // Ruting av kryptert pakke med feilretting og hopp-beregning over WAN
    double route_wan_packet_c(uint32_t target_node_id, const uint8_t* payload, size_t len, WANRouteResult* out_res) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        bool found = false;
        for (uint32_t i = 0; i < active_wan_nodes; i++) {
            if (wan_registry[i].node_id == target_node_id && wan_registry[i].nat_traversed) {
                found = true;
                break;
            }
        }

        // Beregn enkel FNV-1a checksum for feilretting/integritet
        uint32_t checksum = 2166136261U;
        for (size_t i = 0; i < len; i++) {
            checksum ^= payload[i];
            checksum *= 16777619U;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        out_res->packet_delivered = found;
        out_res->checksum_verified = checksum;
        out_res->hops_taken = 2; // Typisk WAN-hopp via mesh-relé
        out_res->routing_latency_ms = ms;

        return ms;
    }
    
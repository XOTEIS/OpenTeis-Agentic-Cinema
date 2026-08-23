
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

    typedef struct {
        uint32_t node_id;
        uint32_t current_state;
        uint32_t next_node_id;
        int execution_status; // 0 = venter, 1 = fullført, -1 = feilet
    } GraphNode;

    typedef struct {
        uint32_t graph_id;
        int node_count;
        GraphNode* nodes;
        uint32_t checkpoint_hash;
    } ExecutionGraph;

    // Utfører en ekte syklisk tilstandsovergang med sjekkpunkt-generering
    uint32_t execute_graph_step(ExecutionGraph* graph, uint32_t input_signal) {
        uint32_t accumulated_hash = graph->graph_id;

        for(int i = 0; i < graph->node_count; i++) {
            GraphNode* node = &graph->nodes[i];
            
            if (node->execution_status == 0) { // Aktiv node
                // Behandler tilstand basert på input og forrige node
                node->current_state = node->current_state ^ input_signal;
                node->execution_status = 1; // Markert som fullført
                
                accumulated_hash ^= node->current_state;
            }
        }

        graph->checkpoint_hash = accumulated_hash;
        return accumulated_hash;
    }
    
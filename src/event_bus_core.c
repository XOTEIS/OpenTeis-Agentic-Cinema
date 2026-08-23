
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>

    typedef struct {
        uint32_t event_id;
        uint32_t sender_node;
        uint32_t topic_hash;
        uint32_t payload;
        int processed;
    } EventMessage;

    // Publiserer og prosesserer meldinger i en asynkron køstruktur
    int dispatch_event_bus(EventMessage* queue, int capacity, uint32_t target_topic) {
        int handled_count = 0;

        for(int i = 0; i < capacity; i++) {
            if(queue[i].topic_hash == target_topic && queue[i].processed == 0) {
                // Utfører ekte tilstandsendring på meldingen i minnet
                queue[i].processed = 1;
                queue[i].payload ^= 0xCAFEBABEU;
                handled_count++;
            }
        }
        return handled_count;
    }
    
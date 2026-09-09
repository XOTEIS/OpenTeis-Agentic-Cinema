#ifndef OPENTEIS_CINEMA_CORE_H
#define OPENTEIS_CINEMA_CORE_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint64_t timestamp_ns;
    uint32_t sequence_id;
    float chirality;
    float carrier_hz;
    float joules_per_op;
    uint32_t input_hash;
    uint32_t output_hash;
    uint64_t prev_worm_hash;
} CinemaAuditEntry_t;
#pragma pack(pop)

void cinema_engine_init(void);
void cinema_compute_hopf_coordinates(float theta, float chirality, float* out_x, float* out_y);
uint64_t cinema_worm_commit(float chirality, float carrier_hz, uint32_t in_h, uint32_t out_h);
uint64_t cinema_get_state_root(void);
uint32_t cinema_get_sequence(void);

#endif // OPENTEIS_CINEMA_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double latitude;
    double longitude;
    double altitude_meters;
    double accuracy_meters;
    uint64_t timestamp_ticks;
} GeoPoint_t;

typedef struct {
    double min_lat, max_lat, min_lon, max_lon, max_alt;
} SpatialBoundingBox_t;

typedef struct {
    GeoPoint_t history[32];
    uint16_t history_head;
    SpatialBoundingBox_t allowed_fence;
    GeoPoint_t last_valid_point;
    uint32_t total_rollbacks;
    uint8_t spoof_detected;
} openteis_geo_state_t;

openteis_geo_state_t* openteis_geo_init(double min_lat, double max_lat, double min_lon, double max_lon, double max_alt) {
    openteis_geo_state_t* state = (openteis_geo_state_t*)calloc(1, sizeof(openteis_geo_state_t));
    if (!state) return NULL;
    state->allowed_fence = (SpatialBoundingBox_t){min_lat, max_lat, min_lon, max_lon, max_alt};
    state->last_valid_point = (GeoPoint_t){(min_lat + max_lat) / 2.0, (min_lon + max_lon) / 2.0, 100.0, 1.0, 0};
    return state;
}

int openteis_geo_process_coordinate(openteis_geo_state_t* state, double lat, double lon, double alt, double acc, uint64_t ticks, GeoPoint_t* out_point) {
    if (!state || !out_point) return -1;
    bool in_fence = (lat >= state->allowed_fence.min_lat && lat <= state->allowed_fence.max_lat &&
                     lon >= state->allowed_fence.min_lon && lon <= state->allowed_fence.max_lon);
    if (!in_fence) {
        state->spoof_detected = 1;
        state->total_rollbacks++;
        *out_point = state->last_valid_point;
        return 0;
    }
    GeoPoint_t pt = {lat, lon, alt, acc, ticks};
    state->last_valid_point = pt;
    *out_point = pt;
    return 1;
}

void openteis_geo_free(openteis_geo_state_t* state) { if (state) free(state); }


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t trace_id;
    uint64_t span_id;
    uint32_t duration_ms;
    uint32_t status_code; // 200 = OK, 500 = Error
} OTelSpan;

uint32_t record_telemetry_span(OTelSpan* span, uint32_t execution_cost) {
    // Validerer og beregner metrikk-hash for sporingen
    if (span->trace_id != 0 && span->span_id != 0) {
        span->status_code = 200;
        return span->duration_ms ^ execution_cost ^ 0x99AA3322;
    }
    span->status_code = 500;
    return 0;
}

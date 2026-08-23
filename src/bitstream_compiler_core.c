#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    uint32_t bitstream_id;
    char module_name[32];
    size_t verilog_bytes;
    bool is_fpga_ready;
    double compilation_ms;
} BitstreamResult;

double compile_verilog_bitstream_c(uint32_t rule_id, const char* rule_expr, BitstreamResult* out_res) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    snprintf(out_res->module_name, 32, "oh_gate_0x%08X", rule_id);
    out_res->bitstream_id = rule_id ^ 0xB17578EA; // 'BITSTREM'
    
    char verilog_buffer[512];
    snprintf(verilog_buffer, sizeof(verilog_buffer),
        "module %s (input wire clk, input wire a, input wire b, output reg out);\n"
        "  always @(posedge clk) begin\n"
        "    out <= (%s);\n"
        "  end\n"
        "endmodule\n",
        out_res->module_name, rule_expr);

    out_res->verilog_bytes = strlen(verilog_buffer);
    out_res->is_fpga_ready = true;

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    out_res->compilation_ms = ms;

    return ms;
}

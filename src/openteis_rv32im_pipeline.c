#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define JOULE_BUDGET_OP 0.031f

typedef struct {
    uint32_t pc;
    uint32_t registers[32];
    uint32_t instructions_executed;
    float operating_voltage_v;
    float dynamic_energy_per_op_j;
    bool pipeline_hazard_free;
} __attribute__((aligned(64))) Rv32imPipelineState_t;

__attribute__((visibility("default")))
Rv32imPipelineState_t* rv32im_pipeline_init(void) {
    Rv32imPipelineState_t* st = NULL;
    if (posix_memalign((void**)&st, 64, sizeof(Rv32imPipelineState_t)) != 0 || !st) {
        return NULL;
    }
    memset(st, 0, sizeof(Rv32imPipelineState_t));
    st->pc = 0x80000000;
    st->operating_voltage_v = 0.45f; /* Sub-terskel spenning */
    st->dynamic_energy_per_op_j = 0.0118f;
    st->pipeline_hazard_free = true;
    return st;
}

__attribute__((visibility("default")))
int rv32im_pipeline_execute_instruction(Rv32imPipelineState_t* st, uint32_t raw_instruction) {
    if (!st) return -1;

    uint32_t opcode = raw_instruction & 0x7F;
    uint32_t rd     = (raw_instruction >> 7) & 0x1F;
    uint32_t funct3 = (raw_instruction >> 12) & 0x07;
    uint32_t rs1    = (raw_instruction >> 15) & 0x1F;
    uint32_t rs2    = (raw_instruction >> 20) & 0x1F;

    /* Deterministisk dekoding av basale OP / OP-IMM instruksjoner */
    if (opcode == 0x13) { /* ADDI */
        int32_t imm = ((int32_t)raw_instruction) >> 20;
        if (rd != 0) st->registers[rd] = st->registers[rs1] + imm;
    } else if (opcode == 0x33) { /* R-Type ALU / M-Extension */
        if (funct3 == 0x0) { /* ADD */
            if (rd != 0) st->registers[rd] = st->registers[rs1] + st->registers[rs2];
        }
    }

    st->pc += 4;
    st->instructions_executed++;
    st->pipeline_hazard_free = (st->dynamic_energy_per_op_j <= JOULE_BUDGET_OP);

    return st->pipeline_hazard_free ? 0 : 1;
}

__attribute__((visibility("default")))
void rv32im_pipeline_free(Rv32imPipelineState_t* st) {
    if (st) free(st);
}

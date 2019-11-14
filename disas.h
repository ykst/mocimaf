#pragma once
#include "cpu_opcode.h"
#include "mapper.h"
#include "utils.h"

typedef struct disas_entry {
    union {
        struct {
            uint8_t op : 1;
            uint8_t arg : 1;
            uint8_t data : 1;
        };
        uint8_t marked;
    };
    struct {
        uint8_t unofficial : 1;
        uint8_t subroutine : 1;
    };
} disas_entry_t;

typedef struct disas *disas_ref;

void disas_destroy(disas_ref self);
disas_ref disas_create(const mapper_t *mapper);
int disas_mark_from_vector(disas_ref self, uint16_t vector);
int disas_mark_from_vectors(disas_ref self);
int disas_mark_data_runtime(disas_ref self, uint16_t addr, uint32_t size);
void disas_mark_op_runtime(disas_ref self, uint16_t addr, bool subroutine);
int disas_dump(disas_ref self, FILE *fp);
// int disas_show(disas_ref self, int x, int height, uint16_t virt,
//               uint32_t pc_phys, uint32_t head_phys);
uint32_t disas_scroll_address(disas_ref self, uint32_t base, int32_t scroll);
const disas_entry_t *disas_get_entries(disas_ref self);
void disas_print_entry(disas_ref self, uint32_t phys, char *buf, int buf_size,
                       bool show_comment);

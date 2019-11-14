// generated file
#pragma once
#include "utils.h"

typedef enum op {
/*% profiles.each do |p| -%*/
    // Addressing:/*%= p.mode %*/
    /*%= p.enum %*/ = 0x/*%= p.op %*/,
/*% end -%*/
} op_t;

typedef enum op_mode {
/*% profiles.group_by{|p| p.mode}.each_key do |k| -%*/
    OP_MODE_/*%= k %*/,
/*% end -%*/
} op_mode_t;

static inline bool op_is_official(op_t op)
{
    switch (op) {
/*% profiles.select{|p| p.official }.each do |p| -%*/
    case /*%= p.enum %*/: /* Fallthrough */
/*% end -%*/
        return true;
    default:
        return false;
    }
}

static inline int op_length(op_t op)
{
    switch (op) {
/*% profiles.group_by{|p| p.length}.each do |k, v| -%*/
/*% v.each do |p| -%*/
    case /*%= p.enum %*/: /* Fallthrough */
/*% end -%*/
        return /*%= k %*/;
/*% end -%*/
    }
}

typedef struct op_profile {
    uint8_t op;
    char name[4];
    op_mode_t mode;
    uint8_t length;
    bool official;
} op_profile_t;

extern op_profile_t op_profiles[256];

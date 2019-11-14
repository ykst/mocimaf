// generated file
#pragma once
#include "cpu.h"
#include "cpu_decode.h"
#include "cpu_decode_official.h"
#include "cpu_decode_unofficial.h"
#include "utils.h"

static inline void cpu_decode_internal(cpu_t *self)
{
    uint8_t op = cpu_fetch(self);

    switch (op) {
/*%= cases.join("\n") %*/
    }

    self->last_op = op;
}


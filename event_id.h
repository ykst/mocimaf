// *** WARNING ***
// This file is generated by genevents.rb
// Keep untouched or ruin the abstraction
#pragma once
#include "utils.h"

typedef enum event_domain_id {
    EVENT_DOMAIN_ID_CORE,
    EVENT_DOMAIN_ID_BUS,
    EVENT_DOMAIN_ID_PPU,
    EVENT_DOMAIN_ID_CPU,
    EVENT_DOMAIN_ID_APU,
    EVENT_DOMAIN_ID_INPUT,
    EVENT_DOMAIN_ID_LOGGER,
} event_domain_id_t;

typedef enum event_id {
    EVENT_ID_CORE_START,
    EVENT_ID_CORE_PAUSE,
    EVENT_ID_CORE_RESUME,
    EVENT_ID_CORE_POLL_IN_PAUSE,
    EVENT_ID_CORE_QUIT,
    EVENT_ID_CORE_DESTROY,
    EVENT_ID_BUS_INT_ASSERTED,
    EVENT_ID_BUS_INT_ACKED,
    EVENT_ID_BUS_OAMDMA,
    EVENT_ID_BUS_DMCDMA,
    EVENT_ID_PPU_SCANLINE,
    EVENT_ID_PPU_FRAME,
    EVENT_ID_PPU_SPRITE0_HIT,
    EVENT_ID_PPU_VBLANK,
    EVENT_ID_PPU_REG,
    EVENT_ID_CPU_DECODE,
    EVENT_ID_CPU_READ,
    EVENT_ID_CPU_WRITE,
    EVENT_ID_CPU_STEP,
    EVENT_ID_CPU_INTERRUPT,
    EVENT_ID_APU_SAMPLE,
    EVENT_ID_APU_LEVEL30,
    EVENT_ID_INPUT_SET_CONTROLS,
    EVENT_ID_INPUT_CONTROL_JOYPAD,
    EVENT_ID_LOGGER_EVENT_TRACE,
} event_id_t;

static inline const char *event_domain_id_show(event_domain_id_t id)
{
    switch (id) {
    case EVENT_DOMAIN_ID_CORE:
        return "CORE";
    case EVENT_DOMAIN_ID_BUS:
        return "BUS";
    case EVENT_DOMAIN_ID_PPU:
        return "PPU";
    case EVENT_DOMAIN_ID_CPU:
        return "CPU";
    case EVENT_DOMAIN_ID_APU:
        return "APU";
    case EVENT_DOMAIN_ID_INPUT:
        return "INPUT";
    case EVENT_DOMAIN_ID_LOGGER:
        return "LOGGER";
    }

    return "-";
}

static inline const char *event_id_show(event_id_t id)
{
    switch (id) {
    case EVENT_ID_CORE_START:
        return "START";
    case EVENT_ID_CORE_PAUSE:
        return "PAUSE";
    case EVENT_ID_CORE_RESUME:
        return "RESUME";
    case EVENT_ID_CORE_POLL_IN_PAUSE:
        return "POLL_IN_PAUSE";
    case EVENT_ID_CORE_QUIT:
        return "QUIT";
    case EVENT_ID_CORE_DESTROY:
        return "DESTROY";
    case EVENT_ID_BUS_INT_ASSERTED:
        return "INT_ASSERTED";
    case EVENT_ID_BUS_INT_ACKED:
        return "INT_ACKED";
    case EVENT_ID_BUS_OAMDMA:
        return "OAMDMA";
    case EVENT_ID_BUS_DMCDMA:
        return "DMCDMA";
    case EVENT_ID_PPU_SCANLINE:
        return "SCANLINE";
    case EVENT_ID_PPU_FRAME:
        return "FRAME";
    case EVENT_ID_PPU_SPRITE0_HIT:
        return "SPRITE0_HIT";
    case EVENT_ID_PPU_VBLANK:
        return "VBLANK";
    case EVENT_ID_PPU_REG:
        return "REG";
    case EVENT_ID_CPU_DECODE:
        return "DECODE";
    case EVENT_ID_CPU_READ:
        return "READ";
    case EVENT_ID_CPU_WRITE:
        return "WRITE";
    case EVENT_ID_CPU_STEP:
        return "STEP";
    case EVENT_ID_CPU_INTERRUPT:
        return "INTERRUPT";
    case EVENT_ID_APU_SAMPLE:
        return "SAMPLE";
    case EVENT_ID_APU_LEVEL30:
        return "LEVEL30";
    case EVENT_ID_INPUT_SET_CONTROLS:
        return "SET_CONTROLS";
    case EVENT_ID_INPUT_CONTROL_JOYPAD:
        return "CONTROL_JOYPAD";
    case EVENT_ID_LOGGER_EVENT_TRACE:
        return "EVENT_TRACE";
    }

    return "-";
}

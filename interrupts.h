#pragma once
//#include "event_logger.h"
#include "utils.h"

typedef enum interruption_type {
    INTERRUTPION_TYPE_NONE = 0,
    INTERRUTPION_TYPE_NMI = 1,
    INTERRUTPION_TYPE_RST,
    INTERRUTPION_TYPE_IRQ,
    INTERRUTPION_TYPE_IRQ_EXT,
    INTERRUTPION_TYPE_IRQ_FRAME,
    INTERRUTPION_TYPE_IRQ_DMC,
} interruption_type_t;

typedef struct interrupts {
    bool reserve_nmi;
    // bool cancell_nmi;
    // bool cancell_vbl;

    /// bool enable_rendering;

    union {
        struct {
            uint8_t nmi;
            uint8_t rst;
            union {
                struct {
                    uint8_t dmc : 1;
                    uint8_t frame : 1;
                    uint8_t ext : 1;
                    uint8_t : 5;
                };
                uint8_t irq;
            };
            uint8_t padding;
        };
        uint32_t interrupted;
    };

    // event_logger_ref shared_elogger;
} interrupts_t;

static inline const char *interruption_type_show(interruption_type_t type)
{
    switch (type) {
    case INTERRUTPION_TYPE_IRQ_DMC:
        return "DMC";
    case INTERRUTPION_TYPE_IRQ_EXT:
        return "EXT";
    case INTERRUTPION_TYPE_IRQ_FRAME:
        return "FRAME";
    case INTERRUTPION_TYPE_IRQ:
        return "IRQ";
    case INTERRUTPION_TYPE_NMI:
        return "NMI";
    case INTERRUTPION_TYPE_RST:
        return "RST";
    default:
        return "?";
    }
}

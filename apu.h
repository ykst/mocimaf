#pragma once
#include "bus.h"
#include "counter.h"
#include "interrupts.h"
#include "utils.h"

typedef struct apu *apu_ref;
#include "apu_event_emitter.h"

typedef struct apu_envelope {
    // Used as the volume in constant volume (C set) mode. Also used as the
    // reload value for the envelope's divider (the period becomes V + 1 quarter
    // frames).
    uint8_t volume : 4;
    // Constant volume flag (0: use volume from envelope; 1: use constant
    // volume)
    uint8_t constant : 1;
    // APU Length Counter halt flag/envelope loop flag
    uint8_t loop : 1;
    uint8_t duty : 2;
} apu_envelope_t;

typedef struct apu_sweep {
    uint8_t shift : 3;
    // 0 : add to period, sweeping toward lower frequencies 1: subtract from
    // period, sweeping toward higher frequencies
    uint8_t negate : 1;
    // The divider's period is P + 1 half-frames
    uint8_t period : 3;
    uint8_t enabled : 1;
} apu_sweep_t;

typedef struct apu {
    struct {
        union {
            struct {
                apu_envelope_t envelope;
                apu_sweep_t sweep;
                uint8_t timer_low;
                struct {
                    uint8_t timer_high : 3;
                    uint8_t length : 5;
                };
            } pulse1;
            struct {
                uint8_t $4000;
                uint8_t $4001;
                uint8_t $4002;
                uint8_t $4003;
            };
        };

        union {
            struct {
                apu_envelope_t envelope;
                apu_sweep_t sweep;
                uint8_t timer_low;
                struct {
                    uint8_t timer_high : 3;
                    uint8_t length : 5;
                };
            } pulse2;
            struct {
                uint8_t $4004;
                uint8_t $4005;
                uint8_t $4006;
                uint8_t $4007;
            };
        };

        union {
            struct {
                struct {
                    uint8_t linear : 7;
                    uint8_t control : 1;
                };
                uint8_t unused;
                uint8_t timer_low;
                struct {
                    uint8_t timer_high : 3;
                    uint8_t length : 5;
                };
            } triangle;
            struct {
                uint8_t $4008;
                uint8_t $4009;
                uint8_t $400a;
                uint8_t $400b;
            };
        };

        union {
            struct {
                apu_envelope_t envelope;
                uint8_t unused;
                struct {
                    uint8_t period : 4;
                    uint8_t : 3;
                    uint8_t mode : 1;
                };
                struct {
                    uint8_t : 3;
                    uint8_t length : 5;
                };
            } noise;
            struct {
                uint8_t $400c;
                uint8_t $400d;
                uint8_t $400e;
                uint8_t $400f;
            };
        };

        union {
            struct {
                struct {
                    uint8_t rate : 4;
                    uint8_t : 2;
                    uint8_t loop : 1;
                    uint8_t int_enable : 1;
                };
                struct {
                    uint8_t level : 7;
                    uint8_t : 1;
                };
                // DPCM samples must begin in the memory range $C000-FFFF at an
                // address set by register $4012 (address = %11AAAAAA.AA000000).
                // The address is incremented; if it exceeds $FFFF, it is
                // wrapped around to $8000.
                uint8_t address;
                // The length of the sample in bytes is set by register $4013
                // (length = %LLLL.LLLL0001).
                uint8_t length;
            } dmc;
            struct {
                uint8_t $4010;
                uint8_t $4011;
                uint8_t $4012;
                uint8_t $4013;
            };
        };

        union {
            // Writing a zero to any of the channel enable bits will silence
            // that channel and immediately set its length counter to 0.
            // Writing to this register clears the DMC interrupt flag.
            // Power-up and reset have the effect of writing $00, silencing all
            // channels.
            struct {
                uint8_t pulse1 : 1;
                uint8_t pulse2 : 1;
                uint8_t triangle : 1;
                uint8_t noise : 1;
                // If the DMC bit is clear, the DMC bytes remaining will be set
                // to 0 and the DMC will silence when it empties.
                uint8_t dmc : 1;
                uint8_t : 3;

            } status;
            uint8_t $4015;
        };

        union {
            struct {
                uint8_t : 6;
                // Interrupt inhibit flag. If set, the frame interrupt flag is
                // cleared, otherwise it is unaffected.
                uint8_t int_disable : 1;
                // 	Sequencer mode: 0 selects 4-step sequence, 1 selects
                // 5-step sequence
                uint8_t mode : 1;
            } frame_counter;
            uint8_t $4017;
        };

    } reg;

    struct {
        bool start;
        int32_t length_counter;
        int32_t volume;
        int32_t decay;
        int32_t divider;

        int32_t sweep_divider;
        bool sweep_reload;
        bool sweep_mute;

        int32_t tval;
        int32_t tval_counter;
        int32_t sequence;
    } pulse1;

    struct {
        bool start;
        int32_t length_counter;
        int32_t volume;
        int32_t decay;
        int32_t divider;

        int32_t sweep_divider;
        bool sweep_reload;
        bool sweep_mute;

        int32_t tval;
        int32_t tval_counter;
        int32_t sequence;
    } pulse2;

    struct {
        int32_t linear;
        int32_t linear_counter;
        int32_t length_counter;
        int32_t tval;
        int32_t tval_counter;
        bool reload;
        int32_t sequence;
    } triangle;

    struct {
        int32_t length_counter;
        int32_t volume;
        int32_t decay;
        int32_t divider;

        int32_t tval;
        int32_t tval_counter;
        bool start;
        uint16_t shift;
    } noise;

    struct {
        int32_t tval;
        int32_t tval_counter;
        uint16_t address;
        uint16_t current_address;
        uint16_t length;
        uint16_t length_counter;
        int16_t level;
        uint8_t buf;
        int bits;
    } dmc;

    int frame_phase;
    int32_t cycles;
    int32_t next_qframe_cycles;
    int32_t cycle_margin;

    struct {
        bool pulse1;
        bool pulse2;
        bool noise;
        bool triangle;
        bool dmc;
    } channel_mute;

    float accum;
    float accum_cnt;
    float pulse_table[31];
    float tnd_table[203];

    counter_t *shared_counter;

    apu_event_emitter_t *shared_emitter;

    bus_ref shared_bus;
} apu_t;

apu_t *apu_create(apu_event_emitter_t *emitter);
void apu_destroy(apu_t *self);
int apu_clock(apu_t *self);
int apu_set_bus(apu_t *self, bus_ref bus);
int apu_write_reg(apu_t *self, uint16_t addr, uint8_t value);
uint8_t apu_read_status(apu_t *self);
uint8_t apu_peek_status(apu_t *self);

// int apu_buffered_samples(apu_t *self);
int apu_cold_boot(apu_t *self);
void apu_soft_reset(apu_t *self);

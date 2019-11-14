// *** WARNING ***
// This file is generated by gen6502.rb
// Keep untouched or ruin the abstraction
// generated file
#pragma once
#include "cpu.h"
#include "cpu_decode.h"
#include "utils.h"

// [BRK] OP:00 Mode:IMPLIED Length:1 Memory:-
// Semantics:PUSH16(PC + 1), P((P & 0xcf) | 0x30), PUSH(P), I(1), VEC(0xfffe)
static inline void cpu_decode_00_brk(cpu_t *self)
{
    // PUSH16(PC + 1)
    cpu_push16(self, self->reg.pc + 1);
    // P((P & 0xcf) | 0x30)
    self->reg.p = (self->reg.p & 0xcf) | 0x30;
    // PUSH(P)
    cpu_push(self, self->reg.p);
    // I(1)
    self->reg.int_disable = (bool)(1);
    // VEC(0xfffe)
    self->reg.pc = cpu_read_vector(self, 0xfffe);
}

// [ORA] OP:01 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(A | T), N, Z, A
static inline void cpu_decode_01_ora(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(A | T)
    tmp = self->reg.a | tmp;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [ASL] OP:06 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), C(T & 0x80), W, T(T << 1), N, Z, W
static inline void cpu_decode_06_asl(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // C(T & 0x80)
    self->reg.carry = (bool)(tmp & 0x80);
    // W(T)
    cpu_write(self, addr, tmp);
    // T(T << 1)
    tmp = tmp << 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [PHP] OP:08 Mode:IMPLIED Length:1 Memory:-
// Semantics:PUSH((P & 0xcf) | 0x30)
static inline void cpu_decode_08_php(cpu_t *self)
{
    // PUSH((P & 0xcf) | 0x30)
    cpu_push(self, (self->reg.p & 0xcf) | 0x30);
}

// [ASL] OP:0A Mode:ACCUMULATOR Length:1 Memory:-
// Semantics:T(A << 1), C(A & 0x80), N, Z, A
static inline void cpu_decode_0A_asl(cpu_t *self)
{
    uint16_t tmp;
    // T(A << 1)
    tmp = self->reg.a << 1;
    // C(A & 0x80)
    self->reg.carry = (bool)(self->reg.a & 0x80);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [BPL] OP:10 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(!N)
static inline void cpu_decode_10_bpl(cpu_t *self, uint16_t addr)
{
    // B(!N)
    cpu_decode_branch(self, addr, !self->reg.negative);
}

// [CLC] OP:18 Mode:IMPLIED Length:1 Memory:-
// Semantics:C(0)
static inline void cpu_decode_18_clc(cpu_t *self)
{
    // C(0)
    self->reg.carry = (bool)(0);
}

// [JSR] OP:20 Mode:ABSOLUTE Length:3 Memory:W
// Semantics:PUSH16(PC - 1), G(1), PC(M)
static inline void cpu_decode_20_jsr(cpu_t *self, uint16_t addr)
{
    // PUSH16(PC - 1)
    cpu_push16(self, self->reg.pc - 1);
    // G(1)
    bus_clock_cpu(self->shared_bus, 1);
    // PC(M)
    self->reg.pc = addr;
}

// [AND] OP:21 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(A & T), N, Z, A
static inline void cpu_decode_21_and(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(A & T)
    tmp = self->reg.a & tmp;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [BIT] OP:24 Mode:ZERO_PAGE Length:2 Memory:R
// Semantics:N, Z(T & A), V(T & 0x40)
static inline void cpu_decode_24_bit(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T & A)
    self->reg.zero = !((tmp & self->reg.a) & 0xff);
    // V(T & 0x40)
    self->reg.overflow = (bool)(tmp & 0x40);
}

// [ROL] OP:26 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), W, T((T << 1) | C), C(T & 0x100), N, Z, W
static inline void cpu_decode_26_rol(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // W(T)
    cpu_write(self, addr, tmp);
    // T((T << 1) | C)
    tmp = (tmp << 1) | self->reg.carry;
    // C(T & 0x100)
    self->reg.carry = (bool)(tmp & 0x100);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [PLP] OP:28 Mode:IMPLIED Length:1 Memory:-
// Semantics:R(PC), P((P & 0x30) | (PULL & ~0x30)), IRQ_DELAY(1)
static inline void cpu_decode_28_plp(cpu_t *self)
{
    // R(PC)
    (void)cpu_read(self, self->reg.pc);
    // P((P & 0x30) | (PULL & ~0x30))
    self->reg.p = (self->reg.p & 0x30) | (cpu_pull(self) & ~0x30);
    // IRQ_DELAY(1)
    self->irq_poll_delay = 1;
}

// [ROL] OP:2A Mode:ACCUMULATOR Length:1 Memory:-
// Semantics:T((A << 1) | C), C(T & 0x100), N, Z, A
static inline void cpu_decode_2A_rol(cpu_t *self)
{
    uint16_t tmp;
    // T((A << 1) | C)
    tmp = (self->reg.a << 1) | self->reg.carry;
    // C(T & 0x100)
    self->reg.carry = (bool)(tmp & 0x100);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [BMI] OP:30 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(N)
static inline void cpu_decode_30_bmi(cpu_t *self, uint16_t addr)
{
    // B(N)
    cpu_decode_branch(self, addr, self->reg.negative);
}

// [SEC] OP:38 Mode:IMPLIED Length:1 Memory:-
// Semantics:C(1)
static inline void cpu_decode_38_sec(cpu_t *self)
{
    // C(1)
    self->reg.carry = (bool)(1);
}

// [RTI] OP:40 Mode:IMPLIED Length:1 Memory:-
// Semantics:R(PC), P((P & 0x30) | (PULL & ~0x30)), PC(PULL16)
static inline void cpu_decode_40_rti(cpu_t *self)
{
    // R(PC)
    (void)cpu_read(self, self->reg.pc);
    // P((P & 0x30) | (PULL & ~0x30))
    self->reg.p = (self->reg.p & 0x30) | (cpu_pull(self) & ~0x30);
    // PC(PULL16)
    self->reg.pc = cpu_pull16(self);
}

// [EOR] OP:41 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(A ^ T), N, Z, A
static inline void cpu_decode_41_eor(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(A ^ T)
    tmp = self->reg.a ^ tmp;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [LSR] OP:46 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), W, C(T & 0x01), T(T >> 1), N, Z, W
static inline void cpu_decode_46_lsr(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // W(T)
    cpu_write(self, addr, tmp);
    // C(T & 0x01)
    self->reg.carry = (bool)(tmp & 0x01);
    // T(T >> 1)
    tmp = tmp >> 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [PHA] OP:48 Mode:IMPLIED Length:1 Memory:-
// Semantics:PUSH(A)
static inline void cpu_decode_48_pha(cpu_t *self)
{
    // PUSH(A)
    cpu_push(self, self->reg.a);
}

// [LSR] OP:4A Mode:ACCUMULATOR Length:1 Memory:-
// Semantics:T(A), C(T & 0x01), T(T >> 1), N, Z, A
static inline void cpu_decode_4A_lsr(cpu_t *self)
{
    uint16_t tmp;
    // T(A)
    tmp = self->reg.a;
    // C(T & 0x01)
    self->reg.carry = (bool)(tmp & 0x01);
    // T(T >> 1)
    tmp = tmp >> 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [JMP] OP:4C Mode:ABSOLUTE Length:3 Memory:W
// Semantics:PC(M)
static inline void cpu_decode_4C_jmp(cpu_t *self, uint16_t addr)
{
    // PC(M)
    self->reg.pc = addr;
}

// [BVC] OP:50 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(!V)
static inline void cpu_decode_50_bvc(cpu_t *self, uint16_t addr)
{
    // B(!V)
    cpu_decode_branch(self, addr, !self->reg.overflow);
}

// [CLI] OP:58 Mode:IMPLIED Length:1 Memory:-
// Semantics:I(0), IRQ_DELAY(1)
static inline void cpu_decode_58_cli(cpu_t *self)
{
    // I(0)
    self->reg.int_disable = (bool)(0);
    // IRQ_DELAY(1)
    self->irq_poll_delay = 1;
}

// [RTS] OP:60 Mode:IMPLIED Length:1 Memory:-
// Semantics:R(PC), G(1), PC(PULL16 + 1)
static inline void cpu_decode_60_rts(cpu_t *self)
{
    // R(PC)
    (void)cpu_read(self, self->reg.pc);
    // G(1)
    bus_clock_cpu(self->shared_bus, 1);
    // PC(PULL16 + 1)
    self->reg.pc = cpu_pull16(self) + 1;
}

// [ADC] OP:61 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(T + A + C), N(T & 0x80), Z, C(T > 0xff), V((A ^ O ^ 0x80) & (A ^ T) & 0x80), A
static inline void cpu_decode_61_adc(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(T + A + C)
    tmp = tmp + self->reg.a + self->reg.carry;
    // N(T & 0x80)
    self->reg.negative = ((tmp & 0x80) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // C(T > 0xff)
    self->reg.carry = (bool)(tmp > 0xff);
    // V((A ^ O ^ 0x80) & (A ^ T) & 0x80)
    self->reg.overflow = (bool)((self->reg.a ^ value ^ 0x80) & (self->reg.a ^ tmp) & 0x80);
    // A(T)
    self->reg.a = tmp;
}

// [ROR] OP:66 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), W, T(T | (C ? 0x100 : 0)), C(T & 0x01), T(T >> 1), N, Z, W
static inline void cpu_decode_66_ror(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // W(T)
    cpu_write(self, addr, tmp);
    // T(T | (C ? 0x100 : 0))
    tmp = tmp | (self->reg.carry ? 0x100 : 0);
    // C(T & 0x01)
    self->reg.carry = (bool)(tmp & 0x01);
    // T(T >> 1)
    tmp = tmp >> 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [PLA] OP:68 Mode:IMPLIED Length:1 Memory:-
// Semantics:R(PC), A(PULL), N(A), Z(A)
static inline void cpu_decode_68_pla(cpu_t *self)
{
    // R(PC)
    (void)cpu_read(self, self->reg.pc);
    // A(PULL)
    self->reg.a = cpu_pull(self);
    // N(A)
    self->reg.negative = ((self->reg.a) >> 7) & 1;
    // Z(A)
    self->reg.zero = !((self->reg.a) & 0xff);
}

// [ROR] OP:6A Mode:ACCUMULATOR Length:1 Memory:-
// Semantics:T(A | (C ? 0x100 : 0)), C(T & 0x01), T(T >> 1), N, Z, A
static inline void cpu_decode_6A_ror(cpu_t *self)
{
    uint16_t tmp;
    // T(A | (C ? 0x100 : 0))
    tmp = self->reg.a | (self->reg.carry ? 0x100 : 0);
    // C(T & 0x01)
    self->reg.carry = (bool)(tmp & 0x01);
    // T(T >> 1)
    tmp = tmp >> 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [BVS] OP:70 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(V)
static inline void cpu_decode_70_bvs(cpu_t *self, uint16_t addr)
{
    // B(V)
    cpu_decode_branch(self, addr, self->reg.overflow);
}

// [SEI] OP:78 Mode:IMPLIED Length:1 Memory:-
// Semantics:I(1)
static inline void cpu_decode_78_sei(cpu_t *self)
{
    // I(1)
    self->reg.int_disable = (bool)(1);
}

// [STA] OP:81 Mode:PRE_INDIRECT_X Length:2 Memory:W
// Semantics:W(A)
static inline void cpu_decode_81_sta(cpu_t *self, uint16_t addr)
{
    // W(A)
    cpu_write(self, addr, self->reg.a);
}

// [STY] OP:84 Mode:ZERO_PAGE Length:2 Memory:W
// Semantics:W(Y)
static inline void cpu_decode_84_sty(cpu_t *self, uint16_t addr)
{
    // W(Y)
    cpu_write(self, addr, self->reg.y);
}

// [STX] OP:86 Mode:ZERO_PAGE Length:2 Memory:W
// Semantics:W(X)
static inline void cpu_decode_86_stx(cpu_t *self, uint16_t addr)
{
    // W(X)
    cpu_write(self, addr, self->reg.x);
}

// [DEY] OP:88 Mode:IMPLIED Length:1 Memory:-
// Semantics:Y(Y - 1), N(Y), Z(Y)
static inline void cpu_decode_88_dey(cpu_t *self)
{
    // Y(Y - 1)
    self->reg.y = self->reg.y - 1;
    // N(Y)
    self->reg.negative = ((self->reg.y) >> 7) & 1;
    // Z(Y)
    self->reg.zero = !((self->reg.y) & 0xff);
}

// [TXA] OP:8A Mode:IMPLIED Length:1 Memory:-
// Semantics:A(X), N(A), Z(A)
static inline void cpu_decode_8A_txa(cpu_t *self)
{
    // A(X)
    self->reg.a = self->reg.x;
    // N(A)
    self->reg.negative = ((self->reg.a) >> 7) & 1;
    // Z(A)
    self->reg.zero = !((self->reg.a) & 0xff);
}

// [BCC] OP:90 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(!C)
static inline void cpu_decode_90_bcc(cpu_t *self, uint16_t addr)
{
    // B(!C)
    cpu_decode_branch(self, addr, !self->reg.carry);
}

// [TYA] OP:98 Mode:IMPLIED Length:1 Memory:-
// Semantics:A(Y), N(A), Z(A)
static inline void cpu_decode_98_tya(cpu_t *self)
{
    // A(Y)
    self->reg.a = self->reg.y;
    // N(A)
    self->reg.negative = ((self->reg.a) >> 7) & 1;
    // Z(A)
    self->reg.zero = !((self->reg.a) & 0xff);
}

// [TXS] OP:9A Mode:IMPLIED Length:1 Memory:-
// Semantics:S(X)
static inline void cpu_decode_9A_txs(cpu_t *self)
{
    // S(X)
    self->reg.s = self->reg.x;
}

// [LDY] OP:A0 Mode:IMMEDIATE Length:2 Memory:-
// Semantics:N, Z, Y
static inline void cpu_decode_A0_ldy(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // Y(T)
    self->reg.y = tmp;
}

// [LDA] OP:A1 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:N, Z, A
static inline void cpu_decode_A1_lda(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // A(T)
    self->reg.a = tmp;
}

// [LDX] OP:A2 Mode:IMMEDIATE Length:2 Memory:-
// Semantics:N, Z, X
static inline void cpu_decode_A2_ldx(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // X(T)
    self->reg.x = tmp;
}

// [TAY] OP:A8 Mode:IMPLIED Length:1 Memory:-
// Semantics:Y(A), N(Y), Z(Y)
static inline void cpu_decode_A8_tay(cpu_t *self)
{
    // Y(A)
    self->reg.y = self->reg.a;
    // N(Y)
    self->reg.negative = ((self->reg.y) >> 7) & 1;
    // Z(Y)
    self->reg.zero = !((self->reg.y) & 0xff);
}

// [TAX] OP:AA Mode:IMPLIED Length:1 Memory:-
// Semantics:X(A), N(X), Z(X)
static inline void cpu_decode_AA_tax(cpu_t *self)
{
    // X(A)
    self->reg.x = self->reg.a;
    // N(X)
    self->reg.negative = ((self->reg.x) >> 7) & 1;
    // Z(X)
    self->reg.zero = !((self->reg.x) & 0xff);
}

// [BCS] OP:B0 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(C)
static inline void cpu_decode_B0_bcs(cpu_t *self, uint16_t addr)
{
    // B(C)
    cpu_decode_branch(self, addr, self->reg.carry);
}

// [CLV] OP:B8 Mode:IMPLIED Length:1 Memory:-
// Semantics:V(0)
static inline void cpu_decode_B8_clv(cpu_t *self)
{
    // V(0)
    self->reg.overflow = (bool)(0);
}

// [TSX] OP:BA Mode:IMPLIED Length:1 Memory:-
// Semantics:X(S), N(X), Z(X)
static inline void cpu_decode_BA_tsx(cpu_t *self)
{
    // X(S)
    self->reg.x = self->reg.s;
    // N(X)
    self->reg.negative = ((self->reg.x) >> 7) & 1;
    // Z(X)
    self->reg.zero = !((self->reg.x) & 0xff);
}

// [CPY] OP:C0 Mode:IMMEDIATE Length:2 Memory:-
// Semantics:T(Y - T), C(T < 0x100), N, Z
static inline void cpu_decode_C0_cpy(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(Y - T)
    tmp = self->reg.y - tmp;
    // C(T < 0x100)
    self->reg.carry = (bool)(tmp < 0x100);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
}

// [CMP] OP:C1 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(A - T), C(T < 0x100), N, Z
static inline void cpu_decode_C1_cmp(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(A - T)
    tmp = self->reg.a - tmp;
    // C(T < 0x100)
    self->reg.carry = (bool)(tmp < 0x100);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
}

// [DEC] OP:C6 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), W, T(T - 1), N, Z, W
static inline void cpu_decode_C6_dec(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // W(T)
    cpu_write(self, addr, tmp);
    // T(T - 1)
    tmp = tmp - 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [INY] OP:C8 Mode:IMPLIED Length:1 Memory:-
// Semantics:Y(Y + 1), N(Y), Z(Y)
static inline void cpu_decode_C8_iny(cpu_t *self)
{
    // Y(Y + 1)
    self->reg.y = self->reg.y + 1;
    // N(Y)
    self->reg.negative = ((self->reg.y) >> 7) & 1;
    // Z(Y)
    self->reg.zero = !((self->reg.y) & 0xff);
}

// [DEX] OP:CA Mode:IMPLIED Length:1 Memory:-
// Semantics:X(X - 1), N(X), Z(X)
static inline void cpu_decode_CA_dex(cpu_t *self)
{
    // X(X - 1)
    self->reg.x = self->reg.x - 1;
    // N(X)
    self->reg.negative = ((self->reg.x) >> 7) & 1;
    // Z(X)
    self->reg.zero = !((self->reg.x) & 0xff);
}

// [BNE] OP:D0 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(!Z)
static inline void cpu_decode_D0_bne(cpu_t *self, uint16_t addr)
{
    // B(!Z)
    cpu_decode_branch(self, addr, !self->reg.zero);
}

// [CLD] OP:D8 Mode:IMPLIED Length:1 Memory:-
// Semantics:D(0)
static inline void cpu_decode_D8_cld(cpu_t *self)
{
    // D(0)
    self->reg.decimal = (bool)(0);
}

// [CPX] OP:E0 Mode:IMMEDIATE Length:2 Memory:-
// Semantics:T(X - T), C(T < 0x100), N, Z
static inline void cpu_decode_E0_cpx(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(X - T)
    tmp = self->reg.x - tmp;
    // C(T < 0x100)
    self->reg.carry = (bool)(tmp < 0x100);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
}

// [SBC] OP:E1 Mode:PRE_INDIRECT_X Length:2 Memory:R
// Semantics:T(A - T - (C ? 0 : 1)), N, Z, V((A ^ T) & (A ^ O) & 0x80), C(T < 0x100), A
static inline void cpu_decode_E1_sbc(cpu_t *self, uint8_t value)
{
    uint16_t tmp = value;
    // T(A - T - (C ? 0 : 1))
    tmp = self->reg.a - tmp - (self->reg.carry ? 0 : 1);
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // V((A ^ T) & (A ^ O) & 0x80)
    self->reg.overflow = (bool)((self->reg.a ^ tmp) & (self->reg.a ^ value) & 0x80);
    // C(T < 0x100)
    self->reg.carry = (bool)(tmp < 0x100);
    // A(T)
    self->reg.a = tmp;
}

// [INC] OP:E6 Mode:ZERO_PAGE Length:2 Memory:RMW
// Semantics:T(R), W, T(T + 1), N, Z, W
static inline void cpu_decode_E6_inc(cpu_t *self, uint16_t addr)
{
    uint16_t tmp;
    // T(R)
    tmp = cpu_read(self, addr);
    // W(T)
    cpu_write(self, addr, tmp);
    // T(T + 1)
    tmp = tmp + 1;
    // N(T)
    self->reg.negative = ((tmp) >> 7) & 1;
    // Z(T)
    self->reg.zero = !((tmp) & 0xff);
    // W(T)
    cpu_write(self, addr, tmp);
}

// [INX] OP:E8 Mode:IMPLIED Length:1 Memory:-
// Semantics:X(X + 1), N(X), Z(X)
static inline void cpu_decode_E8_inx(cpu_t *self)
{
    // X(X + 1)
    self->reg.x = self->reg.x + 1;
    // N(X)
    self->reg.negative = ((self->reg.x) >> 7) & 1;
    // Z(X)
    self->reg.zero = !((self->reg.x) & 0xff);
}

// [BEQ] OP:F0 Mode:RELATIVE Length:2 Memory:W
// Semantics:B(Z)
static inline void cpu_decode_F0_beq(cpu_t *self, uint16_t addr)
{
    // B(Z)
    cpu_decode_branch(self, addr, self->reg.zero);
}

// [SED] OP:F8 Mode:IMPLIED Length:1 Memory:-
// Semantics:D(1)
static inline void cpu_decode_F8_sed(cpu_t *self)
{
    // D(1)
    self->reg.decimal = (bool)(1);
}
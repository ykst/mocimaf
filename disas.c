#include "disas.h"
#include <stdarg.h>
#include <string.h>

typedef struct disas {
    disas_entry_t *entries;
    const mapper_t *shared_mapper;
} disas_t;

static void disas_recursive_virt(disas_ref self, uint16_t virt, bool subroutine,
                                 bool allow_unofficial);
static int disas_absolute_jump_phys(disas_ref self, uint32_t phys,
                                    bool ignore_boundary);
static int disas_absolute_jump_virt(disas_ref self, uint16_t vector,
                                    bool ignore_boundary);

static void disas_recursive_phys(disas_ref self, uint16_t virt_base,
                                 uint32_t phys, bool subroutine,
                                 bool allow_unofficial);
static int disas_mark_data_virt(disas_ref self, uint16_t virt, uint32_t size);
static int disas_mark_data_phys(disas_ref self, uint32_t phys, uint32_t size);
static int disas_absolute_jump_phys(disas_ref self, uint32_t phys,
                                    bool ignore_boundary);

static inline bool _is_potential_jump(disas_t *self, uint8_t *prg);

void disas_destroy(disas_ref self)
{
    if (self) {
        FREE(self->entries);
        FREE(self);
    }
}

disas_ref disas_create(const mapper_t *mapper)
{
    disas_ref self = NULL;

    TALLOC(self);
    TALLOCS(self->entries, mapper->prg_size);
    GUARD(self->shared_mapper = mapper);

    return self;
error:
    disas_destroy(self);
    return NULL;
}

const disas_entry_t *disas_get_entries(disas_ref self)
{
    return self->entries;
}

static int disas_absolute_jump_virt(disas_ref self, uint16_t vector,
                                    bool ignore_boundary)
{
    uint32_t phys =
        self->shared_mapper->on_prg_phys(self->shared_mapper, vector);

    return disas_absolute_jump_phys(self, phys, ignore_boundary);
}

int disas_mark_from_vector(disas_ref self, uint16_t vector)
{
    return disas_absolute_jump_virt(self, vector, true);
}

int disas_mark_from_vectors(disas_ref self)
{
    GUARD(disas_mark_data_runtime(self, 0xFFFA, 6));
    GUARD(disas_mark_from_vector(self, 0xFFFA));
    GUARD(disas_mark_from_vector(self, 0xFFFC));
    GUARD(disas_mark_from_vector(self, 0xFFFE));

    return SUCCESS;
error:
    return NG;
}

static int disas_mark_data_virt(disas_ref self, uint16_t virt, uint32_t size)
{
    uint32_t phys = self->shared_mapper->on_prg_phys(self->shared_mapper, virt);
    return disas_mark_data_phys(self, phys, size);
}

int disas_mark_data_runtime(disas_ref self, uint16_t addr, uint32_t size)
{
    return disas_mark_data_virt(self, addr, size);
}

static int disas_mark_data_phys(disas_ref self, uint32_t phys, uint32_t size)
{
    for (int i = 0; i < size; ++i) {
        GUARD(phys < self->shared_mapper->prg_size);

        if (self->entries[phys].marked) {
            if (!self->entries[phys].data) {
                WARN("overlapping data %02x:%02x@%05x\n",
                     self->entries[phys].marked,
                     self->shared_mapper->prg_base[phys], phys);
                self->entries[phys].data = 1;
            }
        } else {
            self->entries[phys].data = 1;
        }

        ++phys;
    }

    return SUCCESS;
error:
    return NG;
}

static int _snprintf(char **buf_ref, int *inout_size, const char *fmt, ...)
{
    if (*inout_size <= 0) {
        return NG;
    }
    va_list args;
    va_start(args, fmt);

    int delta = vsnprintf(*buf_ref, *inout_size, fmt, args);
    *buf_ref += delta;
    *inout_size -= delta;

    va_end(args);

    return (delta > 0) ? SUCCESS : NG;
}

static int _print_op(disas_t *self, uint32_t phys, char *buf, int org_buf_size)
{
    int buf_size = org_buf_size;
    uint8_t *prg = self->shared_mapper->prg_base;
    op_t op = prg[phys];
    op_profile_t *profile = &op_profiles[op];

    _snprintf(&buf, &buf_size, "%05x: ", phys);

    for (int i = 0; i < 3; ++i) {
        if (i < profile->length) {
            if (i > 0) {
                _snprintf(&buf, &buf_size, " ");
            }
            _snprintf(&buf, &buf_size, "%02x", prg[phys + i]);
        } else {
            _snprintf(&buf, &buf_size, "   ");
        }
    }

    _snprintf(&buf, &buf_size, ": ");

    switch (profile->mode) {
    case OP_MODE_IMPLIED: // Fallthrough
    case OP_MODE_ACCUMULATOR:
        _snprintf(&buf, &buf_size, "%s", profile->name);
        break;
    case OP_MODE_ZERO_PAGE:
        _snprintf(&buf, &buf_size, "%s $%02x", profile->name, prg[phys + 1]);
        break;
    case OP_MODE_ZERO_PAGE_X:
        _snprintf(&buf, &buf_size, "%s $%02x,x", profile->name, prg[phys + 1]);
        break;
    case OP_MODE_ZERO_PAGE_Y:
        _snprintf(&buf, &buf_size, "%s $%02x,y", profile->name, prg[phys + 1]);
        break;
    case OP_MODE_ABSOLUTE:
        _snprintf(&buf, &buf_size, "%s $%04x", profile->name,
                  (uint16_t)prg[phys + 1] | (prg[phys + 2] << 8));
        break;
    case OP_MODE_ABSOLUTE_X:
        _snprintf(&buf, &buf_size, "%s $%04x,x", profile->name,
                  (uint16_t)prg[phys + 1] | (prg[phys + 2] << 8));
        break;
    case OP_MODE_ABSOLUTE_Y:
        _snprintf(&buf, &buf_size, "%s $%04x,y", profile->name,
                  (uint16_t)prg[phys + 1] | (prg[phys + 2] << 8));
        break;
    case OP_MODE_POST_INDIRECT_Y:
        _snprintf(&buf, &buf_size, "%s ($%02x),y", profile->name,
                  prg[phys + 1]);
        break;
    case OP_MODE_PRE_INDIRECT_X:
        _snprintf(&buf, &buf_size, "%s ($%02x,x)", profile->name,
                  prg[phys + 1]);
        break;
    case OP_MODE_INDIRECT:
        _snprintf(&buf, &buf_size, "%s ($%04x)", profile->name,
                  (uint16_t)prg[phys + 1] | (prg[phys + 2] << 8));
        break;
    case OP_MODE_IMMEDIATE:
        _snprintf(&buf, &buf_size, "%s #$%02x", profile->name, prg[phys + 1]);
        break;
    case OP_MODE_RELATIVE:
        _snprintf(&buf, &buf_size, "%s #%d", profile->name,
                  (int8_t)prg[phys + 1]);
        break;
    }

    return org_buf_size - buf_size;
}

static bool _print_entry(disas_t *self, uint32_t phys, char *buf, int buf_size,
                         bool show_comments)
{
    disas_entry_t *entries = self->entries;
    uint8_t *prg = self->shared_mapper->prg_base;

    if (self->entries[phys].op) {
        int offset = _print_op(self, phys, buf, buf_size);
        buf += offset;
        buf_size -= offset;
    } else if (!entries[phys].arg) {
        _snprintf(&buf, &buf_size, "%05x: %02x", phys, prg[phys]);
    } else {
        return false;
    }

    if (show_comments) {
        if (self->entries[phys].subroutine) {
            _snprintf(&buf, &buf_size, " ;subroutine");
        }

        if (self->entries[phys].unofficial) {
            _snprintf(&buf, &buf_size, " ;unofficial");
        }

        if (self->entries[phys].data) {
            _snprintf(&buf, &buf_size, " ;data");
        }

        if (!self->entries[phys].marked) {
            _snprintf(&buf, &buf_size, " ;unknown");
        } else if (self->entries[phys].op &&
                   _is_potential_jump(self, &prg[phys])) {
            _snprintf(&buf, &buf_size, " ;;");
        }

        if ((phys % self->shared_mapper->prg_split) == 0) {
            _snprintf(&buf, &buf_size, " ;bank start %d/%d",
                      phys / self->shared_mapper->prg_split,
                      self->shared_mapper->num_prg_pages);
        }

        if ((phys % self->shared_mapper->prg_split) ==
            self->shared_mapper->prg_split - 1) {
            _snprintf(&buf, &buf_size, " ;bank end %d/%d",
                      phys / self->shared_mapper->prg_split + 1,
                      self->shared_mapper->num_prg_pages);
        }
    }

    return true;
}

void disas_print_entry(disas_ref self, uint32_t phys, char *buf, int buf_size,
                       bool show_comments)
{
    _print_entry(self, phys, buf, buf_size, show_comments);
}

static void disas_recursive_phys(disas_t *self, uint16_t virt_base,
                                 uint32_t phys, bool subroutine,
                                 bool allow_unofficial)
{
    disas_entry_t *entries = self->entries;
    uint8_t *prg = self->shared_mapper->prg_base;

    entries[phys].subroutine = subroutine;

    bool terminate = false;

    do {
        op_t op = prg[phys];

        if (entries[phys].marked) {
            if (entries[phys].op) {
                break;
            }

            if (entries[phys].data || entries[phys].arg) {
                int partial_advance = op_profiles[op].length;
                if (entries[phys + partial_advance].op) {
                    WARN("HACK:instruction overlap %05x\n", phys);
                    entries[phys].op = 1;
                    break;
                } else {
                    WARN("HACK:instruction is used as data %02x@%05x\n",
                         entries[phys].marked, phys);
                }
            }
        }

        if (!op_is_official(op)) {
            if (allow_unofficial) {
                WARN("accepted unofficial op %02x at %05x\n", op, phys);
                entries[phys].unofficial = 1;
                entries[phys].op = 1;
            } else {
                WARN("stop marking at unofficial op %02x at %05x\n", op, phys);
                entries[phys].data = 1;
                break;
            }
        }

        allow_unofficial = false;

        self->entries[phys].op = 1;

        int advance = op_length(op);

        if (phys + advance >= self->shared_mapper->prg_size) {
            WARN("overrun %05x\n", phys);
            break;
        }

        for (int i = 0; i < advance - 1; ++i) {
            uint32_t pos = phys + 1 + i;
            if (self->entries[pos].op) {
                WARN("HACK:instruction overlap %05x\n", pos);
            }
            self->entries[pos].arg = 1;
        }

        switch (op) {
        case OP_20_JSR:
            if (self->shared_mapper->on_boundary_check(self->shared_mapper,
                                                       phys, phys + 2)) {
                disas_absolute_jump_phys(self, phys + 1, false);
            } else {
                WARN("JSR arg crossed bank boundary: %05x\n", phys);
                terminate = true;
            }
            break;
        case OP_4C_JMP: {
            disas_absolute_jump_phys(self, phys + 1, false);
            terminate = true;
            break;
        }
        case OP_40_RTI:
        case OP_60_RTS:
        case OP_6C_JMP:
            terminate = true;
            break;
        default:
            if (op_profiles[op].mode == OP_MODE_RELATIVE) {
                uint32_t branch_phys =
                    phys + (int8_t)(prg[phys + 1]) + op_profiles[op].length;
                if (self->shared_mapper->on_boundary_check(self->shared_mapper,
                                                           phys, branch_phys)) {
                    disas_recursive_phys(self, virt_base, branch_phys, false,
                                         false);
                } else {
                    terminate = true;
                }
            }
            break;
        }

        if (self->shared_mapper->on_boundary_check(self->shared_mapper, phys,
                                                   phys + advance)) {
            phys += advance;
        } else {
            terminate = true;
        }
    } while (!terminate);
}

static void disas_recursive_virt(disas_t *self, uint16_t virt, bool subroutine,
                                 bool allow_unofficial)
{
    uint32_t phys = self->shared_mapper->on_prg_phys(self->shared_mapper, virt);

    disas_recursive_phys(self, virt, phys, subroutine, allow_unofficial);
}

void disas_mark_op_runtime(disas_ref self, uint16_t addr, bool subroutine)
{
    return disas_recursive_virt(self, addr, subroutine, true);
}

static inline bool _is_potential_jump(disas_t *self, uint8_t *prg)
{
    switch (*prg) {
    case OP_60_RTS:
    case OP_40_RTI:
        return true;

    case OP_00_BRK:
        if (op_is_official(prg[1])) {
            return true;
        }
        break;

    case OP_4C_JMP:
    case OP_6C_JMP:
    case OP_20_JSR:
        if (prg[2] < 0x8 || prg[2] >= 0x80) {
            // potential valid instruction
            return true;
        }
        break;
    default:
        if (op_profiles[*prg].mode == OP_MODE_RELATIVE) {
            uint8_t jump_target = prg[(int8_t)prg[1]];
            if (op_is_official(prg[2]) && op_is_official(jump_target)) {
                return true;
            }
        }
        break;
    }
    return false;
}

static void _analyze(disas_t *self)
{
    disas_entry_t *entries = self->entries;
    uint8_t *prg = self->shared_mapper->prg_base;

    uint32_t unknown_start = 0;
    bool unknown_zone = false;
    for (uint32_t i = 0; i < self->shared_mapper->prg_size; ++i) {
        if (unknown_zone) {
            if (entries[i].marked) {
                bool all_op = true;
                uint32_t j = unknown_start;
                uint32_t last_op = j;

                while (j < i) {
                    if (!op_is_official(prg[j])) {
                        all_op = false;
                        break;
                    }
                    last_op = j;
                    j += op_length(prg[j]);
                }

                if (all_op && j == i &&
                    (entries[j].op ||
                     _is_potential_jump(self, &prg[last_op]))) {
                    j = unknown_start;
                    while (j < i) {
                        entries[j].op = 1;
                        int len = op_length(prg[j]);
                        for (int k = 1; k < len; ++k) {
                            entries[j + k].arg = 1;
                        }
                        j += len;
                    }
                    TRACE("heuristic op %05x-%05x\n", unknown_start, i);
                } else {
                    bool all_data = true;
                    for (uint32_t j = unknown_start; j < i; ++j) {
                        if (_is_potential_jump(self, &prg[j])) {
                            all_data = false;
                            break;
                        }
                    }

                    if (all_data) {
                        for (uint32_t j = unknown_start; j < i; ++j) {
                            entries[j].data = 1;
                        }
                        TRACE("heuristic data %05x-%05x\n", unknown_start, i);
                    }
                }
                unknown_zone = false;
            }
        } else if (!entries[i].marked) {
            unknown_zone = true;
            unknown_start = i;
        }
    }
}

uint32_t disas_scroll_address(disas_t *self, uint32_t base, int32_t scroll)
{
    disas_entry_t *entries = self->entries;
    uint32_t prg_size = self->shared_mapper->prg_size;
    int advance = 0;

    while (scroll != 0) {
        advance += sign(scroll);
        uint32_t pos = (base + advance) % prg_size;
        if (entries[pos].op || !entries[pos].arg) {
            scroll -= sign(scroll);
        }
    }

    return (base + advance) % prg_size;
}

uint32_t _count_marks(disas_t *self)
{
    uint32_t num_marks = 0;

    for (int i = 0; i < self->shared_mapper->prg_size; ++i) {
        if (self->entries[i].marked) {
            ++num_marks;
        }
    }

    return num_marks;
}

static int disas_absolute_jump_phys(disas_ref self, uint32_t phys,
                                    bool ignore_boundary)
{
    GUARD(phys < self->shared_mapper->prg_size - 1);

    uint8_t *prg = self->shared_mapper->prg_base;
    uint32_t to_phys = prg[phys] | (prg[phys + 1] << 8);

    if (ignore_boundary || self->shared_mapper->on_boundary_check(
                               self->shared_mapper, phys, to_phys)) {
        disas_recursive_virt(self, to_phys, true, false);
    }

    return SUCCESS;
error:
    return NG;
}

int disas_dump(disas_t *self, FILE *fp)
{
    GUARD(fp);

    _analyze(self);

    char buf[256] = {};

    for (uint32_t i = 0; i < self->shared_mapper->prg_size; ++i) {
        if (_print_entry(self, i, buf, 256, true)) {
            fprintf(fp, "%s\n", buf);
        }
    }

    fprintf(fp, "; marked %.3f%%\n",
            100.0 * (float)_count_marks(self) /
                (float)self->shared_mapper->prg_size);

    return SUCCESS;
error:
    return NG;
}

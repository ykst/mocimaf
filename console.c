#include <curses.h>
#include <libgen.h>
#include <locale.h>

#include "apu.h"
//#include "console_command_impl.h"
#include "console_input.h"
#include "console_internal.h"
#include "core.h"
#include "cpu.h"
#include "debugger.h"
#include "disas.h"
#include "ines.h"
#include "mapper.h"
#include "ppu.h"
#include "utils.h"

// TODO: multiple windows?
static void console_global_init(void)
{
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_WHITE, COLOR_RED);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    cbreak();
    noecho();

    curs_set(0);
    use_default_colors();
    ESCDELAY = 25;
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_DOUBLE_CLICKED, NULL);
}

void console_global_finalize(void)
{
    clear();
    endwin();
}

void console_destroy(console_ref self)
{
    if (self) {
        console_global_finalize();
        FREE(self);
    }
}

console_ref console_create(core_t *core, debugger_ref debugger, disas_ref disas,
                           event_logger_driver_ref event_logger)
{
    console_ref self = NULL;

    TALLOC(self);
    GUARD(self->shared_core = core);
    GUARD(self->shared_debugger = debugger);
    GUARD(self->shared_disas = disas);
    GUARD(self->shared_event_logger = event_logger);

    self->mode = CONSOLE_MODE_NORMAL;
    self->enable_event_log = true;

    console_global_init();

    return self;
error:
    console_destroy(self);
    return NULL;
}

void console_command_commit(console_ref self)
{
    if (strnlen(self->command_buf.str, 256) == 0) {
        return;
    }

    char *context = NULL;
    char *argv[16] = {};
    int argc = 0;
    char *command = strtok_r(self->command_buf.str, " ", &context);
    char *word = NULL;

    while ((word = strtok_r(NULL, " ", &context)) != NULL) {
        argv[argc++] = word;
        if (argc == 16) {
            break;
        }
    }

    console_exec_command(self, command, argc, argv);
    console_command_backto_normal_mode(self);
}

static int _show_disas(console_t *self, int x, int width, int height,
                       uint16_t virt, uint32_t pc_phys, uint32_t head_phys)
{
    char buf[256];
    const disas_entry_t *entries = disas_get_entries(self->shared_disas);
    uint32_t prg_size = self->shared_core->mapper->prg_size;
    int advance = 0;
    int row = 0;
    uint16_t virt_pos = ((virt - (pc_phys - head_phys)) & 0x7FFF) + 0x8000;
    uint32_t phys_pos = head_phys % prg_size;

    while (advance < height) {
        if (entries[phys_pos].op || !entries[phys_pos].arg) {
            disas_print_entry(self->shared_disas, phys_pos, buf, 256, true);

            if (debugger_is_breakpoint(self->shared_debugger, phys_pos)) {
                mvprintw(row, x + 1, "*");
            } else {
                mvprintw(row, x + 1, " ");
            }

            mvprintw(row, x + 2, "%04x|%s", virt_pos, buf);

            if (pc_phys == phys_pos) {
                move(row, x);
                int color = 1;
                switch (self->pause_reason) {
                case CONSOLE_PAUSE_REASON_BREAKPOINT: // Fallthrough
                case CONSOLE_PAUSE_REASON_INTERRUPTION_BREAK:
                    color = 4;
                    break;
                case CONSOLE_PAUSE_REASON_SCANLINE_BREAK:
                    color = 3;
                default:
                    break;
                }
                chgat(width, 0, color, NULL);
            }

            ++row;
            ++advance;
        }

        phys_pos = (phys_pos + 1) % prg_size;
        virt_pos = ((virt_pos + 1) & 0x7FFF) + 0x8000;
    }

    return SUCCESS;
}

static void _show_event_logs(console_ref self, int x, int y, int width,
                             int height)
{
    int row = height - 1;
    char buf[width];

    move(y, x);
    vline(ACS_VLINE, height);
    x += 1;

    if (self->enable_event_log) {
        event_logger_driver_read_start(self->shared_event_logger);

        while (row >= y && event_logger_driver_read(self->shared_event_logger,
                                                    buf, width)) {
            move(row, x);
            printw("%s", buf);

            --row;
        }
    }
}

int console_update(console_ref self)
{
    int screen_height, screen_width;
    int disas_top_margin = 20;
    int mem_x = 0;
    int core_x = 55;
    int disas_x = 111;
    int event_x = 164;
    core_t *core = self->shared_core;
    cpu_t *cpu = core->cpu;
    ppu_t *ppu = core->ppu;
    apu_t *apu = core->apu;
    mapper_t *mapper = core->mapper;
    debugger_ref debugger = self->shared_debugger;
    disas_ref disas = self->shared_disas;
    // inspector_t *inspector = self->shared_inspector;
    const ines_t *ines = core->shared_ines;

    getmaxyx(stdscr, screen_height, screen_width);
    self->view_height = screen_height - 2;
    self->middle_y = self->view_height / 2;

    uint32_t phys = mapper->on_prg_phys(mapper, cpu->reg.pc);
    self->pc_phys = phys;

    if (self->follow) {
        self->base = disas_scroll_address(disas, phys, -disas_top_margin);
        self->follow = false;
        self->cursor = disas_top_margin;
    }

    int disas_height = screen_height - 2;
    int ch = wgetch(stdscr);

    if (ch != ERR) {
        console_display(self, "");

        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                // TBD:
                if (event.bstate & BUTTON1_PRESSED) {
                    if (event.x > disas_x) {
                        uint32_t pos =
                            disas_scroll_address(disas, self->base, event.y);
                        debugger_breakpoint_toggle(debugger, pos);
                    }
                }
            }
        } else {
            console_handle_input(self, ch);
        }
    }

    // XXX: inspectorのステートを全て取らないと画面静止はうまくいかない。

    erase();

    mvprintw(screen_height - 1, 0, "%s", self->display);

    move(0, 0);

    int row = 0;

    // memory inspector
    mvprintw(row, mem_x + 1,
             "  RAM|+0 +1 +2 +3 +4 +5 +6 +7|+8 +9 +a +b +c +d +e +f ");
    move(row, mem_x + 2);
    chgat(core_x - 3, A_UNDERLINE, 0, NULL);
    ++row;

    for (int i = 0 + self->ram_offset_y * 16;
         i < (screen_height - 3) * 16 + self->ram_offset_y * 16; ++i) {
        uint16_t pos = (i >= 0) ? (i & 0x7FF) : (0x800 - (-i & 0x7FF));

        if ((pos % 16) == 0) {
            mvprintw(row, mem_x + 1, " %04x|", pos);
        }

        printw("%02x", cpu->map.ram[pos]);

        if ((pos % 16) == 7) {
            addch('|');
        } else {
            printw(" ");
        }

        if ((pos % 16) == 15) {
            if (pos % 256 == 255) {
                move(row, mem_x + 2);
                chgat(core_x - 3, A_UNDERLINE, 0, NULL);
            }
            ++row;
            // mvprintw("\n");
        }
    }

    move(0, core_x);
    vline(ACS_VLINE, disas_height);

    interrupts_t *ints = &core->bus->ints;
    // core inspector
    row = 0;
    int x = core_x + 2;
    mvprintw(row++, x - 1, "[CPU]");
    mvprintw(row++, x, "A:%02x X:%02x Y:%02x S:%02x PC:%04x", cpu->reg.a,
             cpu->reg.x, cpu->reg.y, cpu->reg.s, cpu->reg.pc);
    mvprintw(row++, x, "NMI:%01x RST:%01x FINT:%01x DINT:%01x EINT:%01x",
             ints->nmi, ints->rst, ints->frame, ints->dmc, ints->ext);
    mvprintw(row++, x, "P:%02x (C:%d Z:%d I:%d D:%d B:%d V:%d N:%d)",
             cpu->reg.p, cpu->reg.carry, cpu->reg.zero, cpu->reg.int_disable,
             cpu->reg.decimal, cpu->reg.brk, cpu->reg.overflow,
             cpu->reg.negative);
    mvprintw(row++, x, "PAD1:%02x PAD2:%02x CYCLE:%llu",
             core->bus->joypads[0].reg.buttons,
             core->bus->joypads[1].reg.buttons, cpu->shared_counter->cycles);

    move(row, core_x + 1);
    hline(ACS_HLINE, disas_x - core_x);
    ++row;

    mvprintw(row++, x - 1, "[PPU]");
    mvprintw(row++, x, "LINE:%3d DOT:%3d FRAME:%lld ", ppu->scanline.y,
             ppu->scanline.x, ppu->frames);
    mvprintw(row++, x,
             "CTRL:%02x MASK:%02x STAT:%02x OAMA:%02x TOG:%d "
             "DLATCH:%02x",
             ppu->reg.$2000, ppu->reg.$2001, ppu->reg.$2002, ppu->reg.$2004,
             ppu->scrolladdr_toggle, ppu->ppudata_latch);

    mvprintw(row++, x, "SCRT:%04x (NY:%d CY:%2d FY:%01x NX:%d CX:%2d FX:%01x)",
             ppu->scrolladdr_temporary.value,
             ppu->scrolladdr_temporary.scroll.nametable_y,
             ppu->scrolladdr_temporary.scroll.coarce_y,
             ppu->scrolladdr_temporary.scroll.fine_y,
             ppu->scrolladdr_temporary.scroll.nametable_x,
             ppu->scrolladdr_temporary.scroll.coarce_x, ppu->scroll_fine_x);

    mvprintw(row++, x, "SCRV:%04x (NY:%d CY:%2d FY:%01x NX:%d CX:%2d FX:%01x)",
             ppu->scrolladdr_current.value,
             ppu->scrolladdr_current.scroll.nametable_y,
             ppu->scrolladdr_current.scroll.coarce_y,
             ppu->scrolladdr_current.scroll.fine_y,
             ppu->scrolladdr_current.scroll.nametable_x,
             ppu->scrolladdr_current.scroll.coarce_x, ppu->scroll_fine_x);
    mvprintw(row, x, "OAM2:");
    for (int i = 0; i < 8; ++i) {
        printw("%2d ", ppu->sprite_metas[i].oam_idx);
    }

    // move(row, core_x + 1);
    // hline(ACS_HLINE, disas_x - core_x);
    ++row;
    ++row;

    mvprintw(row, x, "  OAM| y  i  a  x| y  i  a  x| y  i  a  x| y  i  a  x ");
    move(row, x + 1);
    chgat(disas_x - x - 2, A_UNDERLINE, 0, NULL);
    ++row;

    for (int i = 0; i < 256; ++i) {
        if ((i % 16) == 0) {
            mvprintw(row, x, " %04x|", i);
        }

        printw("%02x", ppu->map.oam[i]);

        if ((i % 16) == 15) {
            if (i % 256 == 255) {
                move(row, x + 1);
                chgat(disas_x - x - 2, A_UNDERLINE, 0, NULL);
            }
            ++row;
            // mvprintw("\n");
        } else if ((i % 4) == 3) {
            printw("|");
        } else {
            printw(" ");
        }
    }

    move(row, core_x + 1);
    hline(ACS_HLINE, disas_x - core_x);
    ++row;

    mvprintw(row++, x - 1, "[APU]");
    mvprintw(row++, x, "PHASE:%d STAT:%02x (FINT:%01x DINT:%01x)",
             apu->frame_phase, apu_peek_status(apu), ints->frame, ints->dmc);

    // pulse1
    mvprintw(row++, x - 1, "%cP1|FREQ:%04x LEN:%02x EV:%02x SW:%02x FMAX:%04x",
             apu->channel_mute.pulse1 ? '-' : ' ',
             apu->reg.pulse1.timer_low | (apu->reg.pulse1.timer_high << 8),
             apu->reg.pulse1.length, apu->reg.$4000, apu->reg.$4001,
             apu->pulse1.tval);

    mvprintw(
        row++, x,
        "  |FCNT:%04x LCNT:%02x VOL:%01x DIV:%01x SEQ:%01x SR:%01x SM:%01x",
        (uint16_t)apu->pulse1.tval_counter, apu->pulse1.length_counter,
        apu->pulse1.volume, apu->pulse1.divider, apu->pulse1.sequence,
        apu->pulse1.sweep_reload, apu->pulse1.sweep_mute);

    if (apu->pulse1.length_counter > 0) {
        move(row - 2, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
        move(row - 1, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
    }

    // pulse2
    mvprintw(row++, x - 1, "%cP2|FREQ:%04x LEN:%02x EV:%02x SW:%02x FMAX:%04x",
             apu->channel_mute.pulse2 ? '-' : ' ',
             apu->reg.pulse2.timer_low | (apu->reg.pulse2.timer_high << 8),
             apu->reg.pulse2.length, apu->reg.$4004, apu->reg.$4005,
             apu->pulse2.tval);

    mvprintw(
        row++, x,
        "  |FCNT:%04x LCNT:%02x VOL:%01x DIV:%01x SEQ:%01x SR:%01x SM:%01x",
        (uint16_t)apu->pulse2.tval_counter, apu->pulse2.length_counter,
        apu->pulse2.volume, apu->pulse2.divider, apu->pulse2.sequence,
        apu->pulse2.sweep_reload, apu->pulse2.sweep_mute);

    if (apu->pulse2.length_counter > 0) {
        move(row - 2, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
        move(row - 1, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
    }

    // triangle
    mvprintw(row++, x - 1,
             "%cTR|FREQ:%04x LEN:%02x LIN:%02x CTRL:%01x FMAX:%04x",
             apu->channel_mute.triangle ? '-' : ' ',
             apu->reg.triangle.timer_low | (apu->reg.triangle.timer_high << 8),
             apu->reg.triangle.length, apu->reg.triangle.linear,
             apu->reg.triangle.control, apu->triangle.tval);

    mvprintw(row++, x, "  |FCNT:%04x LCNT:%02x SEQ:%01x REL:%01x",
             (uint16_t)apu->triangle.tval_counter, apu->triangle.length_counter,
             apu->triangle.sequence, apu->triangle.reload);

    if (apu->triangle.length_counter > 0) {
        move(row - 2, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
        move(row - 1, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
    }
    // noise
    mvprintw(row++, x - 1,
             "%cNS|PERIOD:%04x LEN:%02x EV:%02x MODE:%01x FMAX:%04x",
             apu->channel_mute.noise ? '-' : ' ', apu->reg.noise.period,
             apu->reg.noise.length, apu->reg.$400c, apu->reg.noise.mode,
             apu->noise.tval);

    mvprintw(row++, x, "  |FCNT:%04x LCNT:%02x VOL:%01x SHIFT:%04x START:%01x",
             (uint16_t)apu->noise.tval_counter, apu->noise.length_counter,
             apu->noise.volume, apu->noise.shift, apu->noise.start);

    if (apu->noise.length_counter > 0) {
        move(row - 2, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
        move(row - 1, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
    }
    // dmc
    mvprintw(row++, x - 1,
             "%cDM|RATE:%04x LOOP:%01x INT:%02x LEN:%02x BASE:%02x FMAX:%04x",
             apu->channel_mute.dmc ? '-' : ' ', apu->reg.dmc.rate,
             apu->reg.dmc.loop, apu->reg.dmc.int_enable, apu->reg.dmc.length,
             apu->reg.dmc.address, apu->dmc.tval);

    mvprintw(row++, x,
             "  |FCNT:%04x LCNT:%02x LEVEL:%02x ADDR:%04x BUF:%02x BIT:%01x",
             (uint16_t)apu->dmc.tval_counter, apu->dmc.length_counter,
             apu->dmc.level, apu->dmc.current_address, apu->dmc.buf,
             apu->dmc.bits);

    if (apu->dmc.length_counter > 0) {
        move(row - 2, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
        move(row - 1, core_x + 1);
        chgat(disas_x - core_x, A_BOLD, 0, NULL);
    }

    // disassembly
    mvprintw(self->cursor, disas_x + 1, ">");
    _show_disas(self, disas_x + 1, event_x - disas_x - 1, disas_height,
                cpu->reg.pc, phys, self->base);

    if (disas_scroll_address(disas, self->base, self->cursor) != phys) {
        move(self->cursor, disas_x + 1);
        chgat(event_x - disas_x - 1, A_UNDERLINE, 0, NULL);
    }

    move(0, disas_x);
    vline(ACS_VLINE, disas_height);

    // event
    _show_event_logs(self, event_x, 0, screen_width - event_x - 1,
                     disas_height);

    move(screen_height - 2, 0);
    printw("ROM:%s MAPPER:%d CHRS:%d CRAM:%d PRGS:%d MIRROR:%s FOURS:%d "
           "MD5:%s LOG:%s",
           basename(ines->name), ines->mapper, ines->header->chr_page_counts,
           ines->chr_ram, ines->header->prg_page_counts,
           ines->header->mirroring ? "V" : "H",
           ines->header->four_screen_mirroring, ines->md5,
           logger_get_path(global_logger));

    if (self->shared_core->paused) {
        switch (self->pause_reason) {
        case CONSOLE_PAUSE_REASON_BREAKPOINT:
            printw(" [BREAKPOINT]");
            break;
        case CONSOLE_PAUSE_REASON_SCANLINE_BREAK:
            printw(" [SCANLINE_BREAK]");
            break;
        case CONSOLE_PAUSE_REASON_INTERRUPTION_BREAK:
            printw(" [INTERRUPTION_BREAK]");
            break;
        default:
            break;
        }
    }
    /*
    if (self->shared_analyzer_state->paused) {
        if (self->shared_analyzer_state->pause_breakpoint) {
            printw(" [BREAKPOINT]");
        } else if (self->shared_analyzer_state->pause_scanline_break) {
            printw(" [SCANLINE]");
        } else if (self->shared_analyzer_state->pause_interruption_break) {
            printw(" [INTERRUPTION]");
        } else {
            printw(" [PAUSED]");
        }
    }
    */
    if (ppu->hide_bg) {
        printw(" [NOBG]");
    }
    if (ppu->hide_sprites) {
        printw(" [NO_SPRITE]");
    }

    // if (inspector->sound_mute) {
    //    printw(" [SOUND_MUTE]");
    //}

    move(screen_height - 2, 0);
    chgat(-1, 0, self->shared_core->paused ? 3 : 1, NULL);

    refresh();
    return SUCCESS;
}

void console_focus_pc(console_ref self)
{
    self->follow = true;
}

void console_set_pause_reason(console_ref self, console_pause_reason_t reason)
{
    self->pause_reason = reason;
}

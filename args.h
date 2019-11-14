#pragma once
#include "utils.h"
#include <getopt.h>
#include <string.h>

typedef struct args {
    const char *record_in;
    const char *record_out;
    const char *state_file;
    const char *sram_file;
    const char *rom_file;
    const char *disas_out;
    const char *log_file;
    bool quiet;
    bool just_show_info;
    bool no_trace;
    bool unlimited_fps;
    bool pause_start;
    bool start_with_analyzer;
    uint64_t verbose_trace_mode;
    uint64_t skip_cycles;
    uint64_t skip_frames;
} args_t;

static inline int args_parse(args_t *self, int argc, char **argv)
{
    const char short_opts[] = "-:f:p:r:c:s:S:d:l:t:iqnua";

    const struct option long_opts[] = {
        {"skip_frames", required_argument, NULL, 'f'},
        {"skip_cycles", required_argument, NULL, 'c'},
        {"play_input_record", required_argument, NULL, 'p'},
        {"record_input", required_argument, NULL, 'r'},
        {"load_state", required_argument, NULL, 's'},
        {"sram_file", required_argument, NULL, 'S'},
        {"disas_out", required_argument, NULL, 'd'},
        {"log_file", required_argument, NULL, 'l'},
        {"verbose_trace", required_argument, NULL, 't'},
        {"start_with_analyzer", no_argument, NULL, 'a'},
        {"no_trace", no_argument, NULL, 'n'},
        {"show_info", no_argument, NULL, 'i'},
        {"quiet", no_argument, NULL, 'q'},
        {"unlimited_fps", no_argument, NULL, 'u'},
        {}};

    while (optind < argc) {
        int c;

        if ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
            switch (c) {
            case 'f':
                self->skip_frames = strtoull(optarg, NULL, 10);
                if (self->skip_frames == 0) {
                    self->pause_start = true;
                }
                break;
            case 'c':
                self->skip_cycles = strtoull(optarg, NULL, 10);
                if (self->skip_cycles == 0) {
                    self->pause_start = true;
                }
                break;
            case 'p':
                GUARD(!self->record_in);
                self->record_in = strdup(optarg);
                break;
            case 'r':
                GUARD(!self->record_out);
                self->record_out = strdup(optarg);
                break;
            case 's':
                GUARD(!self->sram_file);
                self->sram_file = strdup(optarg);
                break;
            case 'S':
                GUARD(!self->state_file);
                self->state_file = strdup(optarg);
                break;
            case 'q':
                self->quiet = !self->quiet;
                break;
            case 'u':
                self->unlimited_fps = !self->unlimited_fps;
                break;
            case 'n':
                self->no_trace = !self->no_trace;
                break;
            case 'a':
                self->start_with_analyzer = true;
                break;
            case 't':
                self->verbose_trace_mode = strtoull(optarg, NULL, 10);
                break;
            case 'd':
                GUARD(!self->disas_out);
                self->disas_out = strdup(optarg);
                break;
            case 'l':
                GUARD(!self->log_file);
                self->log_file = strdup(optarg);
                break;
            case 'i':
                self->just_show_info = true;
                break;
            case 1:
                GUARD(!self->rom_file);
                self->rom_file = strdup(optarg);
                break;

            default:
                DIE("unknown arg %c", c);
            }
        } else {
        }
    }
    return SUCCESS;
error:
    return NG;
}

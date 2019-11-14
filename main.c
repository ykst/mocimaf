#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "drivers.h"
#include "logger.h"
#include "utils.h"

#include "core.h"

#include "args.h"
#include "ines.h"

jmp_buf global_jump;
logger_ref global_logger = NULL;

int nes_simulate(ines_t *ines, args_t *args)
{
    // int mode = args->analyze_mode;
    // console_ref console = NULL;

    // uint32_t mask = 0xFF;
    // int flip = 0;
    // display_ref debug_pattern = NULL;
    // display_ref debug_nametable = NULL;
    // display_ref debug_palette = NULL;

    // display_ref display = NULL;
    // audio_ref audio = NULL;
    // recorder_ref recorder = NULL;
    // playback_ref playback = NULL;

    // interrupts_t ints = {};
    // inspector_t inspector = {.quiet = args->quiet,
    //                          .enable_console = args->analyze_mode & 1,
    //                          .enable_disas = args->disas_out ||
    //                                          (args->analyze_mode & 1) ||
    //                                          !args->quiet};

    // tracer_t *tracer = NULL;
    // cpu_t *cpu = NULL;
    core_t *core = NULL;

    GUARD(core = core_create(ines, args));

    if (!args->quiet) {
        GUARD(sdl_driver_plug(core, args));
    }

    if (args->record_in) {
        GUARD(playback_driver_plug(core, args->record_in));
    }

    if (!args->no_trace) {
        GUARD(recorder_driver_plug(core, args->record_out));
    }

    if (args->disas_out) {
        GUARD(disas_driver_plug(core, args->disas_out));
    }

    GUARD(auto_pause_driver_plug(core, args));

    if (!args->no_trace) {
        GUARD(tracer_driver_plug(core, args));
    }

    if (args->start_with_analyzer) {
        GUARD(analyzer_driver_plug(core));
    }

    GUARD(core_run(core));

    // GUARD(cpu = cpu_create(ines, &ints, args));

    // debugger_ref debugger = cpu->debugger;
#if 0

    if (args->record_in) {
        GUARD(playback = playback_create(args->record_in));
    }

    if (!args->quiet && !args->no_trace) {
        GUARD(recorder = recorder_create(args->record_out));
    }
    if (mode & INSPECTOR_MODE_CONSOLE) {
        console = console_create(core, args, recorder, &inspector);
    }
#endif

    // if (!args->quiet) {
    //     // if (mode & INSPECTOR_MODE_PALETTE) {
    //     //     GUARD(debug_palette = display_create("PALETTE", 128, 288, 4,
    //     9));
    //     // }
    //     // if (mode & INSPECTOR_MODE_PATTERN) {
    //     //     GUARD(debug_pattern =
    //     //               display_create("PATTERN", 1024, 512, 256, 128));
    //     // }
    //     // if (mode & INSPECTOR_MODE_NAMETABLE) {
    //     //     GUARD(debug_nametable =
    //     //               display_create("NAME TABLE", 1024, 960, 512, 480));
    //     // }
    //     GUARD(display = display_create(ines->name, 584, 480, 256, 240));
    //     display_update(display);

    //     GUARD(audio = audio_create(core->apu, &inspector));
    // }
#if 0
    sdl_events_ctx sdl_events = sdl_events_create();
    sdl_keyevents_load(
        sdl_events, &inspector, (sdl_keyevent_handler_t)inspector_on_event,
        (int[][2]){{SDLK_q, INSPECTOR_EVENT_QUIT},
                   {SDLK_r, INSPECTOR_EVENT_RESET},
                   {SDLK_g, INSPECTOR_EVENT_SHOW_GRID},
                   {SDLK_t, INSPECTOR_EVENT_SHOW_ATTRIBUTES},
                   {SDLK_d, INSPECTOR_EVENT_CROP_DISPLAY},
                   {SDLK_b, INSPECTOR_EVENT_HIDE_BG},
                   {SDLK_e, INSPECTOR_EVENT_ABORT},
                   {SDLK_p, INSPECTOR_EVENT_PAUSE},
                   {SDLK_s, INSPECTOR_EVENT_HIDE_SPRITES},
                   {SDLK_u, INSPECTOR_EVENT_UNLIMITED_FPS},
                   {SDLK_a, INSPECTOR_EVENT_SOUND_MUTE},
                   {SDLK_DOWN, INSPECTOR_EVENT_STEP_FRAME},
                   {SDLK_RIGHT, INSPECTOR_EVENT_STEP_SCANLINE},
                   {SDLK_PERIOD, INSPECTOR_EVENT_STEP_INSTRUCTION},
                   {}});

    sdl_mouseevents_load(sdl_events, &inspector,
                         (sdl_keyevent_handler_t)inspector_on_event,
                         (int[][2]){{SDL_BUTTON_LEFT, INSPECTOR_EVENT_PAUSE},
                                    {SDL_BUTTON_RIGHT, INSPECTOR_EVENT_QUIT},
                                    {}});

    uint64_t prev_time = SDL_GetPerformanceCounter();
#endif
    // bool prev_crop_display = false;

    // if (!args->no_trace) {
    //    GUARD(tracer = tracer_create(&inspector));
    //    tracer_start(tracer);
    //}

    // inspector.unlimited_fps = args->unlimited_fps;

    /*
        if (!args->no_trace || (args->analyze_mode & 1) || args->skip_cycles >
       0) { core_cpu_delegate( core, (cpu_delegate_t){.ctx = driver, .on_step =
       driver_on_cpu_step}); core_debugger_delegate( core,
       (debugger_delegate_t){ .ctx = driver, .on_breakpoint =
       driver_on_cpu_breakpoint, .on_scanline_break =
       driver_on_cpu_scanline_break, .on_interruption_break =
       driver_on_cpu_scanline_break,
                      });
        }
                      */

    // if (!args->no_trace) {
    // core_apu_delegate(
    //     core, (apu_delegate_t) {
    //         .ctx = driver,
    //         .on_delta = driver_on_apu_delta,
    //     });
    //}

#if 0
    bool playback_done = false;

    uint64_t last_frame = -1;

    // GUARD(cpu_cold_boot(cpu));
    // if (args->pause_start) {
    //     inspector.pause_frame = 1;
    // }

    TRY while (!inspector.quit)
    {
        // timeperf_lap_start(&perf_loop);
        SDL_Event e;

        if (!args->quiet) {
            // PERF(perf_input)
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    inspector.quit = true;
                    break;
                }
                sdl_events_emit(sdl_events, e);
            }
        }
        if (inspector.quit) {
            // timeperf_lap_end(&perf_loop);
            break;
        }

        if ((!inspector.paused || inspector.step) &&
            last_frame != core->ppu->frames) {
            last_frame = core->ppu->frames;
            if (playback) {
                uint8_t system = 0;

                GUARD(playback_step(playback, &system, &cpu->joypads[0].reg,
                                    &cpu->joypads[1].reg));

                if (system & PLAYLOG_SYSTEM_RESET) {
                    ints.rst = 1;
                }

                if (playback_finished(playback)) {
                    playback_destroy(playback);
                    playback_done = true;
                    args->skip_frames = 0;
                    playback = NULL;
                }
            }

            if (recorder) {
                uint8_t system = 0;
                if (ints.rst) {
                    system |= PLAYLOG_SYSTEM_RESET;
                }
                GUARD(recorder_append(recorder, system, cpu->joypads[0].reg,
                                      cpu->joypads[1].reg));
            }
        }
        inspector.skip =
            args->quiet || (core->ppu->frames + 1) < args->skip_frames;

        if (!inspector.skip && console) {
            console_update(console);
        }

        if (!inspector.paused || inspector.step) {
            // PERF(perf_sim)
            GUARD(cpu_simulate_start(cpu));
        }

        if (inspector.skip || inspector.paused ||
            (debugger && debugger_get_switches(debugger)->playback_mute)) {
            if (audio) {
                audio_pause(audio, true);
            }
            core->apu->tail = core->apu->head = 0;
        } else {
            if (audio) {
                audio_pause(audio, false);
            }
        }

        if (!inspector.skip) {
            uint64_t current_time = SDL_GetPerformanceCounter();
            double delta_ms = (current_time - prev_time) * 1000.0 /
                              (double)SDL_GetPerformanceFrequency();

            // PERF(perf_idle)
            if (!inspector.unlimited_fps && !inspector.paused) {
                if (delta_ms < 16) {
                    // usleep((1000 / 60.0 - delta_ms) * 1000);
                    SDL_Delay(16 - delta_ms);
                }

                while (apu_buffered_samples(core->apu) > 1024 * 2) {
                    // usleep(100);
                    SDL_Delay(1);
                }
            }

            prev_time = SDL_GetPerformanceCounter();

            if (prev_crop_display != inspector.crop_display) {
                if (inspector.crop_display) {
                    // PocketNES Safe Area
                    GUARD(display_crop(display, 8, 8, 16, 12));
                } else {
                    GUARD(display_nocrop(display));
                }
                if (inspector.paused && !inspector.step) {
                    display_update(display);
                }
                prev_crop_display = inspector.crop_display;
            }

            if (!inspector.paused || inspector.step) {
                // timeperf_lap_start(&perf_draw);
                if (debugger && debugger_get_switches(debugger)->show_grid) {
                    debug_draw_grid(core->ppu, palette_lut_argb_2C02,
                                    display_get_argb(display));
                }
                display_update(display);

                if (mode & INSPECTOR_MODE_NAMETABLE) {
                    debug_draw_nametable(core->ppu, palette_lut_argb_2C02,
                                         display_get_argb(debug_nametable));
                    display_update(debug_nametable);
                }

                if (mode & INSPECTOR_MODE_PATTERN) {
                    debug_draw_pattern(core->ppu, palette_lut_argb_2C02,
                                       display_get_argb(debug_pattern));
                    display_update(debug_pattern);
                }

                if (mode & INSPECTOR_MODE_PALETTE) {
                    debug_draw_palette(core->ppu, palette_lut_argb_2C02,
                                       display_get_argb(debug_palette));
                    display_update(debug_palette);
                }
                // timeperf_lap_end(&perf_draw);
            }

            inspector.step = false;
        }

        if (playback_done) {
            inspector.paused = true;
            playback_done = false;
        }

        if (core->ppu->frames == args->skip_frames && args->skip_frames > 0) {
            inspector.paused = true;
        }

        if (inspector.paused) {
            if (tracer) {
                tracer_checkpoint(tracer, cpu);
            }

            if (args->quiet) {
                inspector.quit = true;
            }
        }
        // timeperf_lap_end(&perf_loop);
    }

    if (tracer) {
        tracer_checkpoint(tracer, cpu);
    }

    if (core->disas && args->disas_out) {
        disas_dump(core->disas);
    }

    audio_destroy(audio);
    recorder_destroy(recorder);
    playback_destroy(playback);
    display_destroy(display);
    cpu_destroy(cpu);
    tracer_destroy(tracer);
    console_destroy(console);
#endif

    // timeperf_dump(&perf_sim, &perf_loop);
    // timeperf_dump(&perf_scan, &perf_loop);
    // timeperf_dump(&perf_draw, &perf_loop);
    // timeperf_dump(&perf_input, &perf_loop);
    // timeperf_dump(&perf_idle, &perf_loop);
    // timeperf_dump(&perf_loop, NULL);
    core_destroy(core);
    return SUCCESS;
error:
    core_destroy(core);
#if 0
    recorder_destroy(recorder);
    playback_destroy(playback);
    display_destroy(display);
    audio_destroy(audio);
    cpu_destroy(cpu);
    tracer_destroy(tracer);
    console_destroy(console);

#endif
    return NG;
}

__attribute__((destructor)) void _destructor(void)
{
    console_global_finalize();
}

int main(int argc, char **argv)
{
    args_t args = {};

    GUARD(args_parse(&args, argc, argv));
    GUARD(args.rom_file);

    if (args.log_file || args.start_with_analyzer) {
        GUARD(global_logger =
                  logger_create_with_file(args.log_file, LOGLEVEL_WARN));
    } else {
        GUARD(global_logger = logger_create(LOGLEVEL_ERROR));
    }

    if (args.quiet) {
        logger_set_level(global_logger, LOGLEVEL_ERROR);
    }

    // TODO: argsでloglevel設定

    ines_t *ines;

    GUARD(ines = ines_from_file(args.rom_file));

    if (args.just_show_info) {
        ines_dump_info(ines);
    } else {
        // if (!args.quiet) {
        //    GUARD(SDL_Init(0) == 0);
        //    GUARD(SDL_InitSubSystem(SDL_INIT_EVENTS) == 0);
        //}

        TRY GUARD(nes_simulate(ines, &args));

        // TODO: ドライバーレイヤにSDLパーツを全部分離
        // if (!args.quiet) {
        //    SDL_Quit();
        //}
    }

    logger_destroy(global_logger);
    global_logger = NULL;

    return 0;
error:
    logger_destroy(global_logger);
    global_logger = NULL;
    return 1;
}

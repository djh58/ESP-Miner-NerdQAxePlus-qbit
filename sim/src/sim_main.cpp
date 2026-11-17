// nerdqaxe-sim: renders the miner's screens on a desktop.
//
//   nerdqaxe-sim --scenario scenarios/default.json --out out/          PNG of every screen
//   nerdqaxe-sim --scenario scenarios/default.json --screen mining --out out/
//   nerdqaxe-sim --scenario scenarios/default.json --window            interactive window
//
// In the window, Right/Space/1 is the miner's first button (next screen), 2 is
// the second button (screen off/on), R reloads the scenario (it also reloads
// whenever the file changes), Q or Escape quits.
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "displayDriver.h"
#include "lvgl.h"
#include "png_writer.h"
#include "sim_clock.h"
#include "sim_scenario.h"
#include "themes.h"

#ifdef SIM_HAVE_SDL
#include <SDL.h>
#endif

namespace {

constexpr int kWidth = TDISPLAYS3_LCD_H_RES;
constexpr int kHeight = TDISPLAYS3_LCD_V_RES;

lv_color_t s_framebuffer[kWidth * kHeight];
lv_color_t s_drawbuf[kWidth * kHeight];

void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colors)
{
    for (int y = area->y1; y <= area->y2; y++) {
        int w = area->x2 - area->x1 + 1;
        memcpy(&s_framebuffer[y * kWidth + area->x1], colors, w * sizeof(lv_color_t));
        colors += w;
    }
    lv_disp_flush_ready(drv);
}

// Advances LVGL and the virtual clock until animations and timers settle.
void pump(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 5) {
        lv_tick_inc(5);
        sim_clock_advance_ms(5);
        lv_timer_handler();
    }
}

std::vector<uint8_t> framebuffer_rgb(int scale)
{
    std::vector<uint8_t> rgb((size_t) kWidth * scale * kHeight * scale * 3);
    for (int y = 0; y < kHeight * scale; y++) {
        for (int x = 0; x < kWidth * scale; x++) {
            uint32_t c = lv_color_to32(s_framebuffer[(y / scale) * kWidth + (x / scale)]);
            uint8_t *p = &rgb[((size_t) y * kWidth * scale + x) * 3];
            p[0] = (c >> 16) & 0xff;
            p[1] = (c >> 8) & 0xff;
            p[2] = c & 0xff;
        }
    }
    return rgb;
}

void show(lv_obj_t *screen)
{
    lv_disp_load_scr(screen);
    pump(400);
    lv_refr_now(nullptr);
}

time_t mtime(const std::string &path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 ? st.st_mtime : 0;
}

int usage()
{
    fprintf(stderr,
            "usage: nerdqaxe-sim --scenario <file.json> (--out <dir> [--screen <name>] [--scale N] | --window [--scale N])\n"
            "screens:");
    for (const char *const *n = DisplayDriver::screenNames(); *n; n++) fprintf(stderr, " %s", *n);
    fprintf(stderr, "\n");
    return 2;
}

#ifdef SIM_HAVE_SDL
int run_window(UI &ui, SimScenario &scenario, const std::string &scenario_path, int scale)
{
    const char *cycle[] = {"mining", "settings", "price", "global"};
    const int cycle_len = sizeof(cycle) / sizeof(cycle[0]);
    int current = 0;
    bool screen_on = true;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("NerdQAxe++ display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       kWidth * scale, kHeight * scale, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight);
    SDL_RenderSetLogicalSize(ren, kWidth, kHeight);

    show(DisplayDriver::screenByName(ui, cycle[current]));
    time_t seen = mtime(scenario_path);
    uint32_t last_check = SDL_GetTicks();

    for (bool running = true; running;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type != SDL_KEYDOWN) continue;
            switch (e.key.keysym.sym) {
            case SDLK_q:
            case SDLK_ESCAPE:
                running = false;
                break;
            case SDLK_RIGHT:
            case SDLK_SPACE:
            case SDLK_1:
                if (screen_on) current = (current + 1) % cycle_len;
                screen_on = true;
                show(DisplayDriver::screenByName(ui, cycle[current]));
                break;
            case SDLK_2:
                screen_on = !screen_on;
                break;
            case SDLK_r:
                seen = 0;
                break;
            default:
                break;
            }
        }

        if (SDL_GetTicks() - last_check > 500) {
            last_check = SDL_GetTicks();
            time_t now = mtime(scenario_path);
            if (now != seen) {
                std::string error;
                if (load_scenario(scenario_path, scenario, error)) {
                    DisplayDriver::fill(ui, scenario);
                    fprintf(stderr, "loaded %s\n", scenario_path.c_str());
                } else {
                    fprintf(stderr, "%s\n", error.c_str());
                }
                seen = now;
            }
        }

        pump(16);
        lv_refr_now(nullptr);
        std::vector<uint8_t> rgb = framebuffer_rgb(1);
        if (!screen_on) std::fill(rgb.begin(), rgb.end(), 0);
        SDL_UpdateTexture(tex, nullptr, rgb.data(), kWidth * 3);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
#endif

} // namespace

int main(int argc, char **argv)
{
    std::string scenario_path, out_dir, only_screen;
    bool window = false;
    int scale = 0;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--scenario") scenario_path = next();
        else if (a == "--out") out_dir = next();
        else if (a == "--screen") only_screen = next();
        else if (a == "--scale") scale = atoi(next().c_str());
        else if (a == "--window") window = true;
        else return usage();
    }
    if (scenario_path.empty() || (out_dir.empty() == !window)) return usage();

    SimScenario scenario;
    std::string error;
    if (!load_scenario(scenario_path, scenario, error)) {
        fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, s_drawbuf, nullptr, kWidth * kHeight);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = kWidth;
    disp_drv.ver_res = kHeight;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static ThemeNerdqaxeplus2 theme;
    static Board board(&theme, "NerdQAxe++", "BM1370");
    static DisplayDriver driver;
    static UI ui;
    ui.init(&board, &driver);
    DisplayDriver::fill(ui, scenario);

    if (window) {
#ifdef SIM_HAVE_SDL
        return run_window(ui, scenario, scenario_path, scale > 0 ? scale : 3);
#else
        fprintf(stderr, "built without SDL2: use --out for PNG rendering\n");
        return 1;
#endif
    }

    mkdir(out_dir.c_str(), 0755);
    int failures = 0;
    for (const char *const *n = DisplayDriver::screenNames(); *n; n++) {
        if (!only_screen.empty() && only_screen != *n) continue;
        lv_obj_t *screen = DisplayDriver::screenByName(ui, *n);
        if (!screen) continue;
        show(screen);
        int s = scale > 0 ? scale : 2;
        std::vector<uint8_t> rgb = framebuffer_rgb(s);
        std::string path = out_dir + "/" + scenario.name + "-" + *n + ".png";
        if (!write_png_rgb(path, rgb.data(), kWidth * s, kHeight * s, error)) {
            fprintf(stderr, "%s\n", error.c_str());
            failures++;
            continue;
        }
        printf("%s\n", path.c_str());
    }

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    fprintf(stderr, "LVGL pool: %u of %u bytes used (%u%%), largest free block %u\n",
            (unsigned) (mon.total_size - mon.free_size), (unsigned) mon.total_size, mon.used_pct,
            (unsigned) mon.free_biggest_size);
    return failures ? 1 : 0;
}

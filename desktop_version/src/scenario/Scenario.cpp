#include "Scenario.h"

#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

#ifdef _WIN32
#include <process.h>
#define scenario_getpid _getpid
#else
#include <unistd.h>
#define scenario_getpid getpid
#endif

#include "../Entity.h"
#include "../Exit.h"
#include "../Game.h"
#include "../GlitchrunnerMode.h"
#include "../Graphics.h"
#include "../KeyPoll.h"
#include "../Map.h"
#include "../ReleaseVersion.h"
#include "../Screen.h"
#include "../Script.h"
#include "../Vlogging.h"
#include "../Xoshiro.h"
#ifdef INTERIM_VERSION_EXISTS
#include "../InterimVersion.h"
#endif
#include "ScenarioFile.h"
#include "ScenarioProbes.h"
#include "ScenarioRand.h"
#include "ScenarioTrace.h"

using namespace scenario;

namespace
{

bool g_active = false;
bool g_hidden = false;
bool g_realtime = false;
std::string g_scenario_path;
std::string g_trace_path;
std::string g_temp_basedir;
Scenario g_scenario;

std::vector<std::string> g_argv_storage;
std::vector<char*> g_argv;

void remove_temp_basedir(void)
{
    if (g_temp_basedir.empty())
    {
        return;
    }
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::u8path(g_temp_basedir), ec);
}

std::string make_temp_basedir(void)
{
    std::error_code ec;
    std::filesystem::path base = std::filesystem::temp_directory_path(ec);
    if (ec)
    {
        base = ".";
    }
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        char name[64];
        snprintf(name, sizeof(name), "vvvvvv-scenario-%d-%ld-%d", (int) scenario_getpid(), (long) time(NULL), attempt);
        std::filesystem::path p = base / name;
        if (std::filesystem::create_directory(p, ec) && !ec)
        {
            std::string s = p.u8string();
            if (!s.empty() && s[s.size() - 1] != '/' && s[s.size() - 1] != '\\')
            {
#ifdef _WIN32
                s += '\\';
#else
                s += '/';
#endif
            }
            return s;
        }
    }
    return "";
}

void fatal(const std::string& message)
{
    vlog_error("[scenario] %s", message.c_str());
    fprintf(stderr, "[scenario] %s\n", message.c_str());
    exit(1);
}

std::string default_trace_path(const std::string& scenario_path)
{
    std::string name = scenario_path;
    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
    {
        name = name.substr(slash + 1);
    }
    size_t dot = name.rfind('.');
    if (dot != std::string::npos)
    {
        name = name.substr(0, dot);
    }
    return name + ".trace.jsonl";
}

/* Drops every OS event except SDL_QUIT, so closing the window still works.
 * Our own key events are added with SDL_PeepEvents, which bypasses the filter.
 *
 * Render target/device resets are dropped too: the game reacts to them by
 * setting redraw flags (Screen::recacheTextures), and whether they happen
 * depends on the OS and graphics driver. The picture may be wrong after one
 * until the next full redraw, but the game state stays reproducible. */
int SDLCALL event_filter(void* userdata, SDL_Event* event)
{
    (void) userdata;
    return event->type == SDL_QUIT ? 1 : 0;
}

void push_key_event(const SDL_Keycode keycode, const bool down)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.windowID = gameScreen.m_window != NULL ? SDL_GetWindowID(gameScreen.m_window) : 0;
    event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    event.key.repeat = 0;
    event.key.keysym.sym = keycode;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(keycode);
    event.key.keysym.mod = KMOD_NONE;
    if (SDL_PeepEvents(&event, 1, SDL_ADDEVENT, 0, 0) != 1)
    {
        fatal(std::string("could not queue key event: ") + SDL_GetError());
    }
}

/* Injects KEYUP for released keys, then KEYDOWN for newly held keys, each in
 * ascending keycode order. Both lists are sorted. */
void apply_held_keys(const std::vector<SDL_Keycode>& previous, const std::vector<SDL_Keycode>& current)
{
    for (size_t i = 0; i < previous.size(); ++i)
    {
        if (!std::binary_search(current.begin(), current.end(), previous[i]))
        {
            push_key_event(previous[i], false);
        }
    }
    for (size_t i = 0; i < current.size(); ++i)
    {
        if (!std::binary_search(previous.begin(), previous.end(), current[i]))
        {
            push_key_event(current[i], true);
        }
    }
}

void apply_settings(const Scenario& s)
{
    GlitchrunnerMode_set((enum GlitchrunnerMode) s.glitchrunner);
    game.slowdown = s.slowdown;
    game.inputdelay = s.inputdelay;
    game.noflashingmode = s.noflashingmode;
    game.colourblindmode = s.colourblindmode;
    map.invincibility = s.invincibility;
    graphics.setflipmode = s.flipmode;
    game.over30mode = s.over30mode;
    game.muted = s.muted;

    /* Not part of the game state: never block on the display. */
    gameScreen.vsync = false;
    if (gameScreen.m_renderer != NULL)
    {
        SDL_RenderSetVSync(gameScreen.m_renderer, 0);
    }
}

/* The "spawn" start procedure (FORMAT.md, "Start procedures"). */
void start_spawn(const Scenario& s)
{
    xoshiro_seed(game.framecounter);

    game.gamestate = GAMEMODE;
    game.setstate(0);

    for (size_t i = 0; i < SDL_arraysize(obj.collect); ++i)
    {
        obj.collect[i] = false;
    }
    game.nocutscenes = true;
    game.intimetrial = true;
    graphics.flipmode = graphics.setflipmode;

    game.savex = s.pos_x;
    game.savey = s.pos_y;
    game.saverx = s.room_x;
    game.savery = s.room_y;
    game.savegc = s.gravity_flipped ? 1 : 0;
    game.savedir = s.dir;
    game.savepoint = 0;
    game.gravitycontrol = game.savegc;

    game.state = 0;
    game.deathseq = -1;
    game.lifeseq = 0;

    obj.entities.clear();
    obj.createentity(game.savex, game.savey, 0, 0);

    map.resetplayer();
    map.finalmode = false;
    map.gotoroom(game.saverx, game.savery);
    map.initmapdata();

    graphics.fademode = FADE_NONE;

    game.press_action = false;
    game.press_left = false;
    game.press_right = false;
    game.press_interact = false;
    game.press_map = false;

    game.jumppressed = 0;
    game.jumpheld = false;
    game.interactheld = false;
    game.tapleft = 0;
    game.tapright = 0;

    if (map.towermode)
    {
        map.resetplayer();

        const int i = obj.getplayer();
        if (i >= 0 && i < (int) obj.entities.size())
        {
            map.ypos = obj.entities[i].yp - 120;
        }
        map.oldypos = map.ypos;

        map.setbgobjlerp(graphics.towerbg);
        map.cameramode = 0;
        map.colsuperstate = 0;
    }
}

std::string build_header(const std::vector<std::string>& groups)
{
    std::string h = "{\"format\":\"vvvvvv-scenario-trace\",\"version\":1";

    h += ",\"scenario\":{\"name\":";
    append_json_string(h, g_scenario.name);
    h += ",\"path\":";
    append_json_string(h, g_scenario.path);
    h += ",\"text\":";
    append_json_string(h, g_scenario.text);
    h += "}";

    h += ",\"build\":{\"game\":";
    append_json_string(h, RELEASE_VERSION);
#ifdef INTERIM_VERSION_EXISTS
    h += ",\"commit\":";
    append_json_string(h, INTERIM_COMMIT);
    h += ",\"branch\":";
    append_json_string(h, BRANCH_NAME);
#endif
    h += ",\"compiler\":";
#if defined(_MSC_VER)
    char msc[32];
    snprintf(msc, sizeof(msc), "MSVC %d", (int) _MSC_FULL_VER);
    append_json_string(h, msc);
#elif defined(__clang__)
    append_json_string(h, "clang " __clang_version__);
#elif defined(__GNUC__)
    append_json_string(h, "gcc " __VERSION__);
#else
    append_json_string(h, "unknown");
#endif
    h += ",\"platform\":";
#if defined(_WIN64)
    append_json_string(h, "windows-x64");
#elif defined(_WIN32)
    append_json_string(h, "windows-x86");
#elif defined(__APPLE__)
    append_json_string(h, "macos");
#elif defined(__linux__) && defined(__x86_64__)
    append_json_string(h, "linux-x64");
#elif defined(__linux__)
    append_json_string(h, "linux");
#else
    append_json_string(h, "unknown");
#endif
    h += ",\"sdl\":";
    SDL_version v;
    SDL_GetVersion(&v);
    char sdl[32];
    snprintf(sdl, sizeof(sdl), "%d.%d.%d", v.major, v.minor, v.patch);
    append_json_string(h, sdl);
    h += ",\"rand_interposed\":";
    h += SCENARIO_rand_is_interposed() ? "true" : "false";
    h += "}";

    char buf[64];
    snprintf(buf, sizeof(buf), ",\"frames\":%d,\"every\":%d", g_scenario.frames, g_scenario.every);
    h += buf;

    h += ",\"groups\":[";
    for (size_t i = 0; i < groups.size(); ++i)
    {
        if (i > 0)
        {
            h += ',';
        }
        append_json_string(h, groups[i]);
    }
    h += "]";

    h += ",\"probes\":[";
    const std::vector<ProbeFamily>& families = probe_families();
    bool first = true;
    for (size_t i = 0; i < families.size(); ++i)
    {
        bool included = false;
        for (size_t g = 0; g < groups.size(); ++g)
        {
            if (groups[g] == families[i].group)
            {
                included = true;
                break;
            }
        }
        if (!included)
        {
            continue;
        }
        if (families[i].group == "rng" && families[i].name.compare(0, 8, "rng.crt.") == 0 && !SCENARIO_rand_is_interposed())
        {
            continue;
        }
        if (!first)
        {
            h += ',';
        }
        first = false;
        h += "{\"name\":";
        append_json_string(h, families[i].name);
        h += ",\"type\":\"";
        h += probe_type_name(families[i].type);
        h += "\",\"group\":";
        append_json_string(h, families[i].group);
        h += "}";
    }
    h += "]}";
    return h;
}

} /* namespace */

extern "C" int SCENARIO_rand_is_interposed(void)
{
#ifdef _WIN32
    /* Take the address of rand() exactly the way game code sees it. */
    int (*game_rand)(void) = &rand;
    return (void*) game_rand == SCENARIO_rand_impl_address();
#else
    return 0;
#endif
}

/* The lang/ and fonts/ directories change what the game does (e.g. whether a
 * fresh install starts in the language menu, and whether translator options
 * exist), and the game looks for them in several places, including the source
 * tree when the executable is inside it. Scenarios always use the release
 * layout: both directories next to the executable, passed explicitly. */
static void pin_directory(const char* name, const char* flag, bool given)
{
    if (given)
    {
        return;
    }
    char* base = SDL_GetBasePath();
    std::string dir = base != NULL ? base : "";
    SDL_free(base);
    dir += name;
    std::error_code ec;
    if (!std::filesystem::is_directory(std::filesystem::u8path(dir), ec))
    {
        fatal(std::string("scenarios need the ") + name + "/ directory next to the executable, as in a "
            "release build (copy desktop_version/" + name + " from the 2.4.4 sources), or pass " + flag);
    }
#ifdef _WIN32
    dir += '\\';
#else
    dir += '/';
#endif
    g_argv_storage.push_back(flag);
    g_argv_storage.push_back(dir);
}

void SCENARIO_parse_args(int* argc, char*** argv)
{
    for (int i = 1; i < *argc; ++i)
    {
        if (SDL_strcmp((*argv)[i], "-scenario-check-rand") == 0)
        {
            const int crt_ok = SCENARIO_rand_check_against_crt();
            const int interposed = SCENARIO_rand_is_interposed();
            printf("harness rand() is %sused by the game\n", interposed ? "" : "NOT ");
            fflush(stdout);
            exit(crt_ok && interposed ? 0 : 1);
        }
    }

    bool has_basedir = false;
    bool has_langdir = false;
    bool has_fontsdir = false;

    g_argv_storage.clear();
    for (int i = 0; i < *argc; ++i)
    {
        const std::string arg = (*argv)[i];
        if (i > 0 && (arg == "-scenario" || arg == "-trace"))
        {
            if (i + 1 >= *argc)
            {
                fatal(arg + " requires an argument");
            }
            if (arg == "-scenario")
            {
                g_scenario_path = (*argv)[i + 1];
                g_active = true;
            }
            else
            {
                g_trace_path = (*argv)[i + 1];
            }
            ++i;
            continue;
        }
        if (i > 0 && arg == "-scenario-hidden")
        {
            g_hidden = true;
            continue;
        }
        if (i > 0 && arg == "-scenario-realtime")
        {
            g_realtime = true;
            continue;
        }
        if (arg == "-basedir")
        {
            has_basedir = true;
        }
        if (arg == "-langdir")
        {
            has_langdir = true;
        }
        if (arg == "-fontsdir")
        {
            has_fontsdir = true;
        }
        g_argv_storage.push_back(arg);
    }

    if (!g_active)
    {
        if (!g_trace_path.empty() || g_hidden || g_realtime)
        {
            fatal("-trace, -scenario-hidden and -scenario-realtime require -scenario");
        }
        return;
    }

    std::string error;
    if (!load_scenario(g_scenario_path, g_scenario, error))
    {
        fatal(error);
    }
    for (size_t i = 0; i < g_scenario.groups.size(); ++i)
    {
        const std::vector<std::string>& known = probe_groups();
        if (std::find(known.begin(), known.end(), g_scenario.groups[i]) == known.end())
        {
            fatal("unknown probe group '" + g_scenario.groups[i] + "' in record.groups");
        }
    }
    if (g_trace_path.empty())
    {
        g_trace_path = default_trace_path(g_scenario_path);
    }

    /* Audio output isn't part of the game state, and a missing audio device
     * would crash the game (musicclass assumes a working mixer). Must be set
     * before SDL_Init(); an explicit SDL_AUDIODRIVER from the user wins. */
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);

    pin_directory("lang", "-langdir", has_langdir);
    pin_directory("fonts", "-fontsdir", has_fontsdir);

    if (!has_basedir)
    {
        g_temp_basedir = make_temp_basedir();
        if (g_temp_basedir.empty())
        {
            fatal("could not create a temporary -basedir");
        }
        atexit(remove_temp_basedir);
        g_argv_storage.push_back("-basedir");
        g_argv_storage.push_back(g_temp_basedir);
    }

    g_argv.clear();
    for (size_t i = 0; i < g_argv_storage.size(); ++i)
    {
        g_argv.push_back(&g_argv_storage[i][0]);
    }
    g_argv.push_back(NULL);
    *argc = (int) g_argv_storage.size();
    *argv = &g_argv[0];

    vlog_info("[scenario] %s: %s (%d frames)", g_scenario_path.c_str(), g_scenario.name.c_str(), g_scenario.frames);
}

bool SCENARIO_active(void)
{
    return g_active;
}

void SCENARIO_setup(void)
{
    if (!g_active)
    {
        return;
    }

    /* Real input, focus changes and controller hot-plugging never reach the
     * game; drop whatever is already queued. */
    SDL_SetEventFilter(event_filter, NULL);
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    key.keymap.clear();

    apply_settings(g_scenario);

    if (g_scenario.start != START_BOOT)
    {
        /* Canonical CRT RNG state for every start procedure except boot. */
        srand(1);
    }

    switch (g_scenario.start)
    {
    case START_SPAWN:
        start_spawn(g_scenario);
        break;
    case START_TIMETRIAL:
        script.startgamemode((enum StartMode) (Start_TIMETRIAL_SPACESTATION1 + g_scenario.trial));
        break;
    case START_NEWGAME:
        script.startgamemode(Start_MAINGAME);
        break;
    case START_BOOT:
        break;
    }

    for (size_t i = 0; i < g_scenario.overrides.size(); ++i)
    {
        std::string error;
        if (!set_probe(g_scenario.overrides[i].probe, g_scenario.overrides[i].value, error))
        {
            fatal("[start.set] " + error);
        }
    }

    key.isActive = true;
}

int SCENARIO_run(
    void (*step)(void),
    void (*loop_state)(int* gamestate_func_index, int* num_gamestate_funcs, int* meta_func_index)
) {
    if (g_hidden && gameScreen.m_window != NULL)
    {
        SDL_HideWindow(gameScreen.m_window);
    }

    std::vector<std::string> groups = g_scenario.groups;
    if (groups.empty())
    {
        groups = probe_groups();
    }

    TraceWriter trace;
    if (!trace.open(g_trace_path))
    {
        fatal("cannot write trace file '" + g_trace_path + "'");
    }
    trace.write_line(build_header(groups));

    State state;
    LoopInfo loop;

    loop_state(&loop.gamestate_func_index, &loop.num_gamestate_funcs, &loop.meta_func_index);
    collect_state(state, groups, loop);
    trace.write_frame(-1, state);

    std::vector<SDL_Keycode> held;
    size_t next_input = 0;
    const Uint64 start_ticks = SDL_GetTicks64();
    Uint64 virtual_ms = 0;

    for (int frame = 0; frame < g_scenario.frames; ++frame)
    {
        std::vector<SDL_Keycode> now = held;
        while (next_input < g_scenario.inputs.size() && g_scenario.inputs[next_input].frame <= frame)
        {
            now = g_scenario.inputs[next_input].keys;
            ++next_input;
        }
        if (now != held)
        {
            apply_held_keys(held, now);
            held = now;
        }

        const int timestep = game.get_timestep();
        step();
        virtual_ms += timestep;

        if (g_realtime)
        {
            const Uint64 elapsed = SDL_GetTicks64() - start_ticks;
            if (elapsed < virtual_ms)
            {
                SDL_Delay((Uint32) (virtual_ms - elapsed));
            }
        }

        if (frame % g_scenario.every == 0 || frame == g_scenario.frames - 1)
        {
            loop_state(&loop.gamestate_func_index, &loop.num_gamestate_funcs, &loop.meta_func_index);
            collect_state(state, groups, loop);
            trace.write_frame(frame, state);
        }
    }

    char end[64];
    snprintf(end, sizeof(end), "{\"end\":true,\"frames\":%d}", g_scenario.frames);
    trace.write_line(end);
    trace.close();

    vlog_info("[scenario] wrote %s", g_trace_path.c_str());
    return 0;
}

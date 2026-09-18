#ifndef SCENARIOFILE_H
#define SCENARIOFILE_H

#include <SDL.h>
#include <string>
#include <vector>

#include "ScenarioToml.h"

namespace scenario
{

enum StartKind
{
    START_SPAWN,
    START_TIMETRIAL,
    START_NEWGAME,
    START_BOOT
};

/* One line of the input log: from `frame` on, exactly `keys` are held. */
struct InputEntry
{
    int frame;
    std::vector<SDL_Keycode> keys; /* sorted ascending, no duplicates */
};

/* A raw override from [start.set]. */
struct SetOverride
{
    std::string probe;
    TomlValue value;
};

struct Scenario
{
    std::string path;
    std::string text; /* the scenario file, verbatim */

    int format;
    std::string name;
    std::string description;
    std::string game;

    /* [settings] (defaults per FORMAT.md) */
    int glitchrunner; /* enum GlitchrunnerMode */
    int slowdown;
    bool inputdelay;
    bool noflashingmode;
    bool colourblindmode;
    bool invincibility;
    bool flipmode;
    bool over30mode;
    bool muted;

    /* [start] */
    StartKind start;
    int room_x, room_y;
    int pos_x, pos_y;
    bool gravity_flipped;
    int dir;
    int trial;
    std::vector<SetOverride> overrides;

    /* [record] */
    int every;
    std::vector<std::string> groups;

    /* [input] */
    int frames;
    std::vector<InputEntry> inputs;
};

/* Loads and validates a scenario file. Returns false with a message. */
bool load_scenario(const std::string& path, Scenario& out, std::string& error);

/* Parses one held-button token ("left", "flip", "key:z", ...). */
bool keycode_from_token(const std::string& token, SDL_Keycode& out);

} /* namespace scenario */

#endif /* SCENARIOFILE_H */

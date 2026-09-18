#include "ScenarioFile.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../GlitchrunnerMode.h"

namespace scenario
{

namespace
{

bool read_file(const std::string& path, std::string& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f == NULL)
    {
        return false;
    }
    char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        out.append(buf, n);
    }
    fclose(f);
    return true;
}

std::string dirname_of(const std::string& path)
{
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
    {
        return "";
    }
    return path.substr(0, slash + 1);
}

std::string lower(const std::string& s)
{
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
    {
        if (r[i] >= 'A' && r[i] <= 'Z')
        {
            r[i] = r[i] - 'A' + 'a';
        }
    }
    return r;
}

class Validator
{
public:
    Validator(const TomlDocument& d, std::string& e) : doc(d), error(e) {}

    const TomlDocument& doc;
    std::string& error;

    bool fail(const std::string& msg)
    {
        if (error.empty())
        {
            error = msg;
        }
        return false;
    }

    bool check_keys(const std::string& table, const char* const* allowed)
    {
        const TomlTable* t = doc.table(table);
        if (t == NULL)
        {
            return true;
        }
        for (size_t i = 0; i < t->keys.size(); ++i)
        {
            bool ok = false;
            for (const char* const* a = allowed; *a != NULL; ++a)
            {
                if (t->keys[i] == *a)
                {
                    ok = true;
                    break;
                }
            }
            if (!ok)
            {
                std::string where = table.empty() ? "top level" : "[" + table + "]";
                return fail("unknown key '" + t->keys[i] + "' in " + where);
            }
        }
        return true;
    }

    const TomlValue* get(const std::string& table, const char* key)
    {
        const TomlTable* t = doc.table(table);
        if (t == NULL)
        {
            return NULL;
        }
        return t->get(key);
    }

    bool get_int(const std::string& table, const char* key, int& out, bool required)
    {
        const TomlValue* v = get(table, key);
        if (v == NULL)
        {
            return required ? fail(std::string("missing ") + key + " in [" + table + "]") : true;
        }
        if (v->kind != TomlValue::INTEGER)
        {
            return fail(std::string(key) + " must be an integer");
        }
        out = (int) v->i;
        return true;
    }

    bool get_bool(const std::string& table, const char* key, bool& out)
    {
        const TomlValue* v = get(table, key);
        if (v == NULL)
        {
            return true;
        }
        if (v->kind != TomlValue::BOOLEAN)
        {
            return fail(std::string(key) + " must be true or false");
        }
        out = v->b;
        return true;
    }

    bool get_string(const std::string& table, const char* key, std::string& out, bool required)
    {
        const TomlValue* v = get(table, key);
        if (v == NULL)
        {
            return required ? fail(std::string("missing ") + key) : true;
        }
        if (v->kind != TomlValue::STRING)
        {
            return fail(std::string(key) + " must be a string");
        }
        out = v->s;
        return true;
    }

    bool get_pair(const std::string& table, const char* key, int& a, int& b)
    {
        const TomlValue* v = get(table, key);
        if (v == NULL)
        {
            return fail(std::string("missing ") + key + " in [" + table + "]");
        }
        if (v->kind != TomlValue::ARRAY || v->arr.size() != 2
        || v->arr[0].kind != TomlValue::INTEGER || v->arr[1].kind != TomlValue::INTEGER)
        {
            return fail(std::string(key) + " must be an array of two integers");
        }
        a = (int) v->arr[0].i;
        b = (int) v->arr[1].i;
        return true;
    }
};

bool parse_input_log(const std::string& log, Scenario& out, std::string& error)
{
    int lineno = 0;
    size_t start = 0;
    int last_frame = -1;
    while (start <= log.size())
    {
        size_t end = log.find('\n', start);
        if (end == std::string::npos)
        {
            end = log.size();
        }
        std::string line = log.substr(start, end - start);
        start = end + 1;
        ++lineno;

        size_t hash = line.find('#');
        if (hash != std::string::npos)
        {
            line = line.substr(0, hash);
        }

        std::vector<std::string> tokens;
        std::string cur;
        for (size_t i = 0; i <= line.size(); ++i)
        {
            char c = i < line.size() ? line[i] : ' ';
            if (c == ' ' || c == '\t' || c == '\r' || c == ',')
            {
                if (!cur.empty())
                {
                    tokens.push_back(cur);
                    cur.clear();
                }
            }
            else
            {
                cur += c;
            }
        }
        if (tokens.empty())
        {
            if (end == log.size())
            {
                break;
            }
            continue;
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "input log line %d: ", lineno);
        std::string where = buf;

        char* endp = NULL;
        long frame = strtol(tokens[0].c_str(), &endp, 10);
        if (endp == NULL || *endp != '\0' || frame < 0)
        {
            error = where + "expected a frame number, got '" + tokens[0] + "'";
            return false;
        }
        if (frame <= last_frame)
        {
            error = where + "frame numbers must be strictly increasing";
            return false;
        }
        last_frame = (int) frame;

        InputEntry entry;
        entry.frame = (int) frame;
        if (tokens.size() == 2 && tokens[1] == "-")
        {
            /* Nothing held. */
        }
        else
        {
            if (tokens.size() < 2)
            {
                error = where + "expected held buttons or '-' after the frame number";
                return false;
            }
            for (size_t i = 1; i < tokens.size(); ++i)
            {
                SDL_Keycode k;
                if (!keycode_from_token(tokens[i], k))
                {
                    error = where + "unknown button '" + tokens[i] + "'";
                    return false;
                }
                entry.keys.push_back(k);
            }
        }
        std::sort(entry.keys.begin(), entry.keys.end());
        entry.keys.erase(std::unique(entry.keys.begin(), entry.keys.end()), entry.keys.end());
        out.inputs.push_back(entry);

        if (end == log.size())
        {
            break;
        }
    }
    return true;
}

} /* namespace */

bool keycode_from_token(const std::string& token_in, SDL_Keycode& out)
{
    const std::string token = lower(token_in);

    struct Named
    {
        const char* name;
        SDL_Keycode key;
    };
    static const Named buttons[] = {
        {"left", SDLK_LEFT},
        {"right", SDLK_RIGHT},
        {"up", SDLK_UP},
        {"down", SDLK_DOWN},
        {"flip", SDLK_v},
        {"interact", SDLK_e},
        {"map", SDLK_RETURN},
        {"restart", SDLK_r},
        {"esc", SDLK_ESCAPE},
        {NULL, 0}
    };
    static const Named keys[] = {
        {"space", SDLK_SPACE},
        {"return", SDLK_RETURN},
        {"escape", SDLK_ESCAPE},
        {"backspace", SDLK_BACKSPACE},
        {"tab", SDLK_TAB},
        {"up", SDLK_UP},
        {"down", SDLK_DOWN},
        {"left", SDLK_LEFT},
        {"right", SDLK_RIGHT},
        {"lshift", SDLK_LSHIFT},
        {"rshift", SDLK_RSHIFT},
        {"lctrl", SDLK_LCTRL},
        {"rctrl", SDLK_RCTRL},
        {"lalt", SDLK_LALT},
        {"ralt", SDLK_RALT},
        {"kp_enter", SDLK_KP_ENTER},
        {"f1", SDLK_F1}, {"f2", SDLK_F2}, {"f3", SDLK_F3}, {"f4", SDLK_F4},
        {"f5", SDLK_F5}, {"f6", SDLK_F6}, {"f7", SDLK_F7}, {"f8", SDLK_F8},
        {"f9", SDLK_F9}, {"f10", SDLK_F10}, {"f11", SDLK_F11}, {"f12", SDLK_F12},
        {NULL, 0}
    };

    if (token.compare(0, 4, "key:") == 0)
    {
        const std::string name = token.substr(4);
        if (name.size() == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= '0' && name[0] <= '9')))
        {
            out = (SDL_Keycode) name[0];
            return true;
        }
        for (const Named* k = keys; k->name != NULL; ++k)
        {
            if (name == k->name)
            {
                out = k->key;
                return true;
            }
        }
        return false;
    }

    for (const Named* b = buttons; b->name != NULL; ++b)
    {
        if (token == b->name)
        {
            out = b->key;
            return true;
        }
    }
    return false;
}

bool load_scenario(const std::string& path, Scenario& out, std::string& error)
{
    out = Scenario();
    out.path = path;
    if (!read_file(path, out.text))
    {
        error = "cannot read scenario file '" + path + "'";
        return false;
    }

    TomlDocument doc;
    std::string perr;
    if (!toml_parse(out.text, doc, perr))
    {
        error = path + ": " + perr;
        return false;
    }

    Validator v(doc, error);

    for (std::map<std::string, TomlTable>::const_iterator it = doc.tables.begin(); it != doc.tables.end(); ++it)
    {
        const std::string& n = it->first;
        if (n != "" && n != "settings" && n != "start" && n != "start.set"
        && n != "record" && n != "check" && n != "input")
        {
            error = "unknown table [" + n + "]";
            return false;
        }
    }

    static const char* const top_keys[] = {"format", "name", "description", "game", NULL};
    static const char* const settings_keys[] = {
        "glitchrunner", "slowdown", "inputdelay", "noflashingmode", "colourblindmode",
        "invincibility", "flipmode", "over30mode", "muted", NULL
    };
    static const char* const start_keys[] = {"kind", "room", "pos", "gravity", "dir", "trial", NULL};
    static const char* const record_keys[] = {"every", "groups", NULL};
    static const char* const input_keys[] = {"frames", "log", "file", NULL};

    if (!v.check_keys("", top_keys)
    || !v.check_keys("settings", settings_keys)
    || !v.check_keys("start", start_keys)
    || !v.check_keys("record", record_keys)
    || !v.check_keys("input", input_keys))
    {
        return false;
    }

    /* Top level */
    if (!v.get_int("", "format", out.format, true))
    {
        return false;
    }
    if (out.format != 1)
    {
        error = "unsupported scenario format (expected format = 1)";
        return false;
    }
    if (!v.get_string("", "name", out.name, true)
    || !v.get_string("", "description", out.description, false)
    || !v.get_string("", "game", out.game, false))
    {
        return false;
    }

    /* [settings] */
    out.glitchrunner = GlitchrunnerNone;
    out.slowdown = 30;
    out.inputdelay = false;
    out.noflashingmode = false;
    out.colourblindmode = false;
    out.invincibility = false;
    out.flipmode = false;
    out.over30mode = false;
    out.muted = true;

    std::string glitchrunner = "none";
    if (!v.get_string("settings", "glitchrunner", glitchrunner, false)
    || !v.get_int("settings", "slowdown", out.slowdown, false)
    || !v.get_bool("settings", "inputdelay", out.inputdelay)
    || !v.get_bool("settings", "noflashingmode", out.noflashingmode)
    || !v.get_bool("settings", "colourblindmode", out.colourblindmode)
    || !v.get_bool("settings", "invincibility", out.invincibility)
    || !v.get_bool("settings", "flipmode", out.flipmode)
    || !v.get_bool("settings", "over30mode", out.over30mode)
    || !v.get_bool("settings", "muted", out.muted))
    {
        return false;
    }
    if (glitchrunner == "none")
    {
        out.glitchrunner = GlitchrunnerNone;
    }
    else if (glitchrunner == "2.0")
    {
        out.glitchrunner = Glitchrunner2_0;
    }
    else if (glitchrunner == "2.2")
    {
        out.glitchrunner = Glitchrunner2_2;
    }
    else
    {
        error = "settings.glitchrunner must be \"none\", \"2.0\" or \"2.2\"";
        return false;
    }
    if (out.slowdown != 30 && out.slowdown != 24 && out.slowdown != 18 && out.slowdown != 12)
    {
        error = "settings.slowdown must be 30, 24, 18 or 12";
        return false;
    }

    /* [start] */
    std::string kind;
    if (!v.get_string("start", "kind", kind, true))
    {
        return false;
    }
    out.room_x = out.room_y = out.pos_x = out.pos_y = 0;
    out.gravity_flipped = false;
    out.dir = 1;
    out.trial = 0;
    if (kind == "spawn")
    {
        out.start = START_SPAWN;
        std::string gravity = "down";
        std::string dir = "right";
        if (!v.get_pair("start", "room", out.room_x, out.room_y)
        || !v.get_pair("start", "pos", out.pos_x, out.pos_y)
        || !v.get_string("start", "gravity", gravity, false)
        || !v.get_string("start", "dir", dir, false))
        {
            return false;
        }
        if (gravity != "down" && gravity != "up")
        {
            error = "start.gravity must be \"down\" or \"up\"";
            return false;
        }
        if (dir != "right" && dir != "left")
        {
            error = "start.dir must be \"right\" or \"left\"";
            return false;
        }
        out.gravity_flipped = gravity == "up";
        out.dir = dir == "right" ? 1 : 0;
    }
    else if (kind == "timetrial")
    {
        out.start = START_TIMETRIAL;
        const TomlValue* t = v.get("start", "trial");
        static const char* const trials[] = {
            "spacestation1", "laboratory", "tower", "spacestation2", "warpzone", "finallevel"
        };
        if (t == NULL)
        {
            error = "start.trial is required for kind = \"timetrial\"";
            return false;
        }
        if (t->kind == TomlValue::INTEGER && t->i >= 0 && t->i < 6)
        {
            out.trial = (int) t->i;
        }
        else if (t->kind == TomlValue::STRING)
        {
            out.trial = -1;
            for (int i = 0; i < 6; ++i)
            {
                if (t->s == trials[i])
                {
                    out.trial = i;
                }
            }
            if (out.trial < 0)
            {
                error = "unknown start.trial '" + t->s + "'";
                return false;
            }
        }
        else
        {
            error = "start.trial must be 0..5 or a trial name";
            return false;
        }
    }
    else if (kind == "newgame")
    {
        out.start = START_NEWGAME;
    }
    else if (kind == "boot")
    {
        out.start = START_BOOT;
    }
    else
    {
        error = "unknown start.kind '" + kind + "'";
        return false;
    }

    const TomlTable* set = doc.table("start.set");
    if (set != NULL)
    {
        for (size_t i = 0; i < set->keys.size(); ++i)
        {
            SetOverride o;
            o.probe = set->keys[i];
            o.value = *set->get(o.probe);
            if (o.value.kind != TomlValue::INTEGER && o.value.kind != TomlValue::BOOLEAN
            && o.value.kind != TomlValue::FLOAT && o.value.kind != TomlValue::STRING)
            {
                error = "[start.set] " + o.probe + ": value must be a scalar";
                return false;
            }
            out.overrides.push_back(o);
        }
    }

    /* [record] */
    out.every = 1;
    if (!v.get_int("record", "every", out.every, false))
    {
        return false;
    }
    if (out.every < 1)
    {
        error = "record.every must be >= 1";
        return false;
    }
    const TomlValue* groups = v.get("record", "groups");
    if (groups != NULL)
    {
        if (groups->kind != TomlValue::ARRAY)
        {
            error = "record.groups must be an array of strings";
            return false;
        }
        for (size_t i = 0; i < groups->arr.size(); ++i)
        {
            if (groups->arr[i].kind != TomlValue::STRING)
            {
                error = "record.groups must be an array of strings";
                return false;
            }
            out.groups.push_back(groups->arr[i].s);
        }
    }

    /* [input] */
    if (!v.get_int("input", "frames", out.frames, true))
    {
        return false;
    }
    if (out.frames < 1)
    {
        error = "input.frames must be >= 1";
        return false;
    }
    const TomlValue* log = v.get("input", "log");
    const TomlValue* file = v.get("input", "file");
    if (log != NULL && file != NULL)
    {
        error = "[input] takes either log or file, not both";
        return false;
    }
    std::string log_text;
    if (log != NULL)
    {
        if (log->kind != TomlValue::STRING)
        {
            error = "input.log must be a string";
            return false;
        }
        log_text = log->s;
    }
    else if (file != NULL)
    {
        if (file->kind != TomlValue::STRING)
        {
            error = "input.file must be a string";
            return false;
        }
        std::string p = dirname_of(path) + file->s;
        if (!read_file(p, log_text))
        {
            error = "cannot read input file '" + p + "'";
            return false;
        }
    }
    if (!parse_input_log(log_text, out, error))
    {
        return false;
    }

    return true;
}

} /* namespace scenario */

#ifndef SCENARIOPROBES_H
#define SCENARIOPROBES_H

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "ScenarioToml.h"

namespace scenario
{

enum ProbeType
{
    PT_I32,
    PT_U32,
    PT_BOOL,
    PT_F32,
    PT_STR
};

const char* probe_type_name(ProbeType type);

struct ProbeValue
{
    ProbeValue(void) : type(PT_I32), bits(0) {}

    ProbeType type;
    /* i32/u32/bool/f32 are all stored as their 32-bit pattern, so equality
     * is bit-exact (floats included). */
    uint32_t bits;
    std::string s;

    bool operator==(const ProbeValue& o) const
    {
        return type == o.type && bits == o.bits && s == o.s;
    }
    bool operator!=(const ProbeValue& o) const { return !(*this == o); }
};

/* A probe family, e.g. "obj.entities[].xp". Concrete probe names replace
 * each "[]" with "[<index>]". */
struct ProbeFamily
{
    std::string name;
    ProbeType type;
    std::string group;
};

/* main.cpp's loop bookkeeping, reported through the harness. */
struct LoopInfo
{
    int gamestate_func_index;
    int num_gamestate_funcs;
    int meta_func_index;
};

typedef std::vector<std::pair<std::string, ProbeValue> > State;

/* All probe families, in collection order. */
const std::vector<ProbeFamily>& probe_families(void);

/* All group names. */
const std::vector<std::string>& probe_groups(void);

/* Collects the current game state for the given groups (empty = all). */
void collect_state(State& out, const std::vector<std::string>& groups, const LoopInfo& loop);

/* Writes a value into a writable probe (used by [start.set]). */
bool set_probe(const std::string& name, const TomlValue& value, std::string& error);

/* Formats a value as JSON. */
void append_json_value(std::string& out, const ProbeValue& v);
void append_json_string(std::string& out, const std::string& s);

} /* namespace scenario */

#endif /* SCENARIOPROBES_H */

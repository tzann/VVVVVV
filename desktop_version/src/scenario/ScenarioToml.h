#ifndef SCENARIOTOML_H
#define SCENARIOTOML_H

/* A small parser for the subset of TOML used by scenario files:
 *   - comments, [table] and [dotted.table] headers
 *   - key = value, where key is bare or quoted ("..." / '...')
 *   - values: integers, floats, booleans, basic/literal strings,
 *     multi-line basic/literal strings, arrays of values
 * Not supported: dotted keys, inline tables, arrays of tables, dates. */

#include <map>
#include <string>
#include <vector>

namespace scenario
{

struct TomlValue
{
    enum Kind
    {
        NONE,
        INTEGER,
        FLOAT,
        BOOLEAN,
        STRING,
        ARRAY
    };

    TomlValue(void) : kind(NONE), i(0), f(0.0), b(false) {}

    Kind kind;
    long long i;
    double f;
    bool b;
    std::string s;
    std::vector<TomlValue> arr;

    const char* kind_name(void) const;
};

/* One [table]: keys in file order. */
struct TomlTable
{
    std::vector<std::string> keys;
    std::map<std::string, TomlValue> values;

    const TomlValue* get(const std::string& key) const;
};

struct TomlDocument
{
    /* Table name ("" for the root table) -> table. */
    std::map<std::string, TomlTable> tables;

    const TomlTable* table(const std::string& name) const;
};

/* Returns false and sets `error` (with line number) on a syntax error. */
bool toml_parse(const std::string& text, TomlDocument& doc, std::string& error);

} /* namespace scenario */

#endif /* SCENARIOTOML_H */

#include "ScenarioToml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace scenario
{

const char* TomlValue::kind_name(void) const
{
    switch (kind)
    {
    case NONE: return "none";
    case INTEGER: return "integer";
    case FLOAT: return "float";
    case BOOLEAN: return "boolean";
    case STRING: return "string";
    case ARRAY: return "array";
    }
    return "?";
}

const TomlValue* TomlTable::get(const std::string& key) const
{
    std::map<std::string, TomlValue>::const_iterator it = values.find(key);
    if (it == values.end())
    {
        return NULL;
    }
    return &it->second;
}

const TomlTable* TomlDocument::table(const std::string& name) const
{
    std::map<std::string, TomlTable>::const_iterator it = tables.find(name);
    if (it == tables.end())
    {
        return NULL;
    }
    return &it->second;
}

namespace
{

class Parser
{
public:
    Parser(const std::string& text) : t(text), pos(0), line(1) {}

    bool parse(TomlDocument& doc, std::string& error);

private:
    const std::string& t;
    size_t pos;
    int line;
    std::string err;

    bool eof(void) const { return pos >= t.size(); }
    char peek(size_t off = 0) const { return pos + off < t.size() ? t[pos + off] : '\0'; }
    bool starts_with(const char* s) const { return t.compare(pos, strlen(s), s) == 0; }

    void advance(size_t n = 1)
    {
        for (size_t k = 0; k < n && pos < t.size(); ++k)
        {
            if (t[pos] == '\n')
            {
                ++line;
            }
            ++pos;
        }
    }

    bool fail(const std::string& msg)
    {
        if (err.empty())
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "line %d: ", line);
            err = buf + msg;
        }
        return false;
    }

    void skip_ws(void)
    {
        while (peek() == ' ' || peek() == '\t')
        {
            advance();
        }
    }

    void skip_comment(void)
    {
        if (peek() == '#')
        {
            while (!eof() && peek() != '\n')
            {
                advance();
            }
        }
    }

    /* Skips whitespace, newlines and comments. */
    void skip_all(void)
    {
        while (!eof())
        {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            {
                advance();
            }
            else if (c == '#')
            {
                skip_comment();
            }
            else
            {
                break;
            }
        }
    }

    bool expect_line_end(void)
    {
        skip_ws();
        skip_comment();
        if (peek() == '\r')
        {
            advance();
        }
        if (eof())
        {
            return true;
        }
        if (peek() != '\n')
        {
            return fail("expected end of line");
        }
        advance();
        return true;
    }

    static bool is_bare(char c)
    {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9') || c == '_' || c == '-';
    }

    bool parse_key(std::string& key);
    bool parse_value(TomlValue& v);
    bool parse_basic_string(std::string& out, bool multiline);
    bool parse_literal_string(std::string& out, bool multiline);
    bool parse_array(TomlValue& v);
    bool parse_scalar(TomlValue& v);
    static void append_utf8(std::string& out, unsigned long cp);
};

void Parser::append_utf8(std::string& out, unsigned long cp)
{
    if (cp < 0x80)
    {
        out += (char) cp;
    }
    else if (cp < 0x800)
    {
        out += (char) (0xC0 | (cp >> 6));
        out += (char) (0x80 | (cp & 0x3F));
    }
    else if (cp < 0x10000)
    {
        out += (char) (0xE0 | (cp >> 12));
        out += (char) (0x80 | ((cp >> 6) & 0x3F));
        out += (char) (0x80 | (cp & 0x3F));
    }
    else
    {
        out += (char) (0xF0 | (cp >> 18));
        out += (char) (0x80 | ((cp >> 12) & 0x3F));
        out += (char) (0x80 | ((cp >> 6) & 0x3F));
        out += (char) (0x80 | (cp & 0x3F));
    }
}

bool Parser::parse_basic_string(std::string& out, bool multiline)
{
    advance(multiline ? 3 : 1);
    if (multiline)
    {
        if (peek() == '\r' && peek(1) == '\n')
        {
            advance(2);
        }
        else if (peek() == '\n')
        {
            advance();
        }
    }
    while (true)
    {
        if (eof())
        {
            return fail("unterminated string");
        }
        char c = peek();
        if (multiline && starts_with("\"\"\""))
        {
            advance(3);
            return true;
        }
        if (!multiline && c == '"')
        {
            advance();
            return true;
        }
        if (!multiline && c == '\n')
        {
            return fail("newline in string");
        }
        if (c == '\\')
        {
            advance();
            char e = peek();
            advance();
            switch (e)
            {
            case 'b': out += '\b'; break;
            case 't': out += '\t'; break;
            case 'n': out += '\n'; break;
            case 'f': out += '\f'; break;
            case 'r': out += '\r'; break;
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case 'u':
            case 'U':
            {
                int n = (e == 'u') ? 4 : 8;
                if (pos + n > t.size())
                {
                    return fail("bad unicode escape");
                }
                std::string hex = t.substr(pos, n);
                advance(n);
                append_utf8(out, strtoul(hex.c_str(), NULL, 16));
                break;
            }
            case '\r':
            case '\n':
            case ' ':
            case '\t':
                if (!multiline)
                {
                    return fail("bad escape");
                }
                /* Line-ending backslash: trim following whitespace. */
                while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n'))
                {
                    advance();
                }
                break;
            default:
                return fail("bad escape");
            }
            continue;
        }
        out += c;
        advance();
    }
}

bool Parser::parse_literal_string(std::string& out, bool multiline)
{
    advance(multiline ? 3 : 1);
    if (multiline)
    {
        if (peek() == '\r' && peek(1) == '\n')
        {
            advance(2);
        }
        else if (peek() == '\n')
        {
            advance();
        }
    }
    while (true)
    {
        if (eof())
        {
            return fail("unterminated string");
        }
        if (multiline && starts_with("'''"))
        {
            advance(3);
            return true;
        }
        char c = peek();
        if (!multiline && c == '\'')
        {
            advance();
            return true;
        }
        if (!multiline && c == '\n')
        {
            return fail("newline in string");
        }
        if (c != '\r')
        {
            out += c;
        }
        advance();
    }
}

bool Parser::parse_key(std::string& key)
{
    key.clear();
    if (peek() == '"')
    {
        return parse_basic_string(key, false);
    }
    if (peek() == '\'')
    {
        return parse_literal_string(key, false);
    }
    while (is_bare(peek()))
    {
        key += peek();
        advance();
    }
    if (key.empty())
    {
        return fail("expected a key");
    }
    return true;
}

bool Parser::parse_array(TomlValue& v)
{
    v.kind = TomlValue::ARRAY;
    advance(); /* [ */
    while (true)
    {
        skip_all();
        if (eof())
        {
            return fail("unterminated array");
        }
        if (peek() == ']')
        {
            advance();
            return true;
        }
        TomlValue elem;
        if (!parse_value(elem))
        {
            return false;
        }
        v.arr.push_back(elem);
        skip_all();
        if (peek() == ',')
        {
            advance();
        }
        else if (peek() != ']')
        {
            return fail("expected ',' or ']' in array");
        }
    }
}

bool Parser::parse_scalar(TomlValue& v)
{
    std::string tok;
    while (!eof())
    {
        char c = peek();
        if (c == ',' || c == ']' || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '#')
        {
            break;
        }
        tok += c;
        advance();
    }
    if (tok == "true" || tok == "false")
    {
        v.kind = TomlValue::BOOLEAN;
        v.b = tok == "true";
        return true;
    }
    if (tok == "inf" || tok == "+inf" || tok == "-inf" || tok == "nan" || tok == "+nan" || tok == "-nan")
    {
        v.kind = TomlValue::FLOAT;
        v.f = strtod(tok.c_str(), NULL);
        return true;
    }
    std::string clean;
    for (size_t k = 0; k < tok.size(); ++k)
    {
        if (tok[k] != '_')
        {
            clean += tok[k];
        }
    }
    if (clean.empty())
    {
        return fail("expected a value");
    }
    const char* s = clean.c_str();
    char* end = NULL;
    bool is_float = clean.find_first_of(".eE") != std::string::npos
        && !(clean.size() > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X'));
    if (is_float)
    {
        v.kind = TomlValue::FLOAT;
        v.f = strtod(s, &end);
    }
    else
    {
        v.kind = TomlValue::INTEGER;
        int base = 10;
        if (clean.size() > 2 && clean[0] == '0')
        {
            if (clean[1] == 'x') { base = 16; s += 2; }
            else if (clean[1] == 'o') { base = 8; s += 2; }
            else if (clean[1] == 'b') { base = 2; s += 2; }
        }
        v.i = strtoll(s, &end, base);
    }
    if (end == NULL || *end != '\0')
    {
        return fail("invalid value '" + tok + "'");
    }
    return true;
}

bool Parser::parse_value(TomlValue& v)
{
    if (starts_with("\"\"\""))
    {
        v.kind = TomlValue::STRING;
        return parse_basic_string(v.s, true);
    }
    if (peek() == '"')
    {
        v.kind = TomlValue::STRING;
        return parse_basic_string(v.s, false);
    }
    if (starts_with("'''"))
    {
        v.kind = TomlValue::STRING;
        return parse_literal_string(v.s, true);
    }
    if (peek() == '\'')
    {
        v.kind = TomlValue::STRING;
        return parse_literal_string(v.s, false);
    }
    if (peek() == '[')
    {
        return parse_array(v);
    }
    if (peek() == '{')
    {
        return fail("inline tables are not supported");
    }
    return parse_scalar(v);
}

bool Parser::parse(TomlDocument& doc, std::string& error)
{
    std::string current;
    doc.tables[current];

    while (true)
    {
        skip_all();
        if (eof())
        {
            break;
        }

        if (peek() == '[')
        {
            if (peek(1) == '[')
            {
                fail("arrays of tables are not supported");
                break;
            }
            advance();
            skip_ws();
            std::string name;
            while (!eof() && peek() != ']' && peek() != '\n')
            {
                if (peek() != ' ' && peek() != '\t')
                {
                    name += peek();
                }
                advance();
            }
            if (peek() != ']')
            {
                fail("unterminated table header");
                break;
            }
            advance();
            if (name.empty())
            {
                fail("empty table name");
                break;
            }
            if (doc.tables.count(name))
            {
                fail("duplicate table [" + name + "]");
                break;
            }
            current = name;
            doc.tables[current];
            if (!expect_line_end())
            {
                break;
            }
            continue;
        }

        std::string key;
        if (!parse_key(key))
        {
            break;
        }
        skip_ws();
        if (peek() == '.')
        {
            fail("dotted keys are not supported; quote the key instead");
            break;
        }
        if (peek() != '=')
        {
            fail("expected '=' after key '" + key + "'");
            break;
        }
        advance();
        skip_ws();

        TomlValue value;
        if (!parse_value(value))
        {
            break;
        }

        TomlTable& table = doc.tables[current];
        if (table.values.count(key))
        {
            fail("duplicate key '" + key + "'");
            break;
        }
        table.keys.push_back(key);
        table.values[key] = value;

        if (!expect_line_end())
        {
            break;
        }
    }

    if (!err.empty())
    {
        error = err;
        return false;
    }
    return true;
}

} /* namespace */

bool toml_parse(const std::string& text, TomlDocument& doc, std::string& error)
{
    Parser p(text);
    return p.parse(doc, error);
}

} /* namespace scenario */

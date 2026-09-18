#include "ScenarioTrace.h"

#include <algorithm>
#include <vector>

namespace scenario
{

bool TraceWriter::open(const std::string& path)
{
    close();
    file = fopen(path.c_str(), "wb");
    if (file == NULL)
    {
        return false;
    }
    setvbuf(file, NULL, _IOFBF, 1 << 20);
    previous.clear();
    return true;
}

void TraceWriter::write_line(const std::string& line)
{
    if (file == NULL)
    {
        return;
    }
    fwrite(line.data(), 1, line.size(), file);
    fputc('\n', file);
}

void TraceWriter::write_frame(const int frame, const State& state)
{
    char buf[32];
    buffer.clear();
    snprintf(buf, sizeof(buf), "{\"f\":%d,\"v\":{", frame);
    buffer += buf;

    bool first = true;
    std::unordered_map<std::string, ProbeValue> current;
    current.reserve(state.size());

    for (size_t i = 0; i < state.size(); ++i)
    {
        const std::string& name = state[i].first;
        const ProbeValue& value = state[i].second;
        current[name] = value;

        std::unordered_map<std::string, ProbeValue>::const_iterator prev = previous.find(name);
        if (prev != previous.end() && prev->second == value)
        {
            continue;
        }
        if (!first)
        {
            buffer += ',';
        }
        first = false;
        append_json_string(buffer, name);
        buffer += ':';
        append_json_value(buffer, value);
    }

    /* Probes that disappeared (e.g. a vector shrank), in sorted order. */
    std::vector<std::string> removed;
    for (std::unordered_map<std::string, ProbeValue>::const_iterator it = previous.begin(); it != previous.end(); ++it)
    {
        if (current.find(it->first) == current.end())
        {
            removed.push_back(it->first);
        }
    }
    std::sort(removed.begin(), removed.end());
    for (size_t i = 0; i < removed.size(); ++i)
    {
        if (!first)
        {
            buffer += ',';
        }
        first = false;
        append_json_string(buffer, removed[i]);
        buffer += ":null";
    }

    buffer += "}}";
    write_line(buffer);

    previous.swap(current);
}

void TraceWriter::close(void)
{
    if (file != NULL)
    {
        fclose(file);
        file = NULL;
    }
}

} /* namespace scenario */

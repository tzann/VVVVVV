#ifndef SCENARIOTRACE_H
#define SCENARIOTRACE_H

#include <stdio.h>
#include <string>
#include <unordered_map>

#include "ScenarioProbes.h"

namespace scenario
{

/* Writes a delta-encoded JSON Lines trace (see FORMAT.md). */
class TraceWriter
{
public:
    TraceWriter(void) : file(NULL) {}
    ~TraceWriter(void) { close(); }

    bool open(const std::string& path);
    void write_line(const std::string& line);

    /* Writes one frame record containing only probes that changed since the
     * previous record (removed probes are written as null). */
    void write_frame(int frame, const State& state);

    void close(void);

private:
    FILE* file;
    std::unordered_map<std::string, ProbeValue> previous;
    std::string buffer;
};

} /* namespace scenario */

#endif /* SCENARIOTRACE_H */

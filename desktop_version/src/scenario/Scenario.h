#ifndef SCENARIO_H
#define SCENARIO_H

/* Reference scenario harness.
 *
 * Runs a scenario file (see scenarios/FORMAT.md in the fe6 repository)
 * against the unmodified game and records per-frame game state into a
 * trace file. The harness is inert unless `-scenario <file>` is passed on
 * the command line.
 *
 * The game's own code only calls the four functions below; everything else
 * lives in this directory. */

/* Strips the harness' own command line arguments from argv, and adds a
 * temporary -basedir if the scenario is active and none was given.
 * Call right after vlog_init(). */
void SCENARIO_parse_args(int* argc, char*** argv);

/* True if a scenario was requested on the command line. */
bool SCENARIO_active(void);

/* Applies pinned settings and the scenario's start procedure. Must be called
 * after all game initialisation and before the main loop is entered.
 * No-op if no scenario is active. */
void SCENARIO_setup(void);

/* Runs the scenario to completion and returns the process exit code.
 *
 * step: advances the game by exactly one fixed timestep (one call to the
 *       game's real deltaloop() with a virtual clock).
 * loop_state: reports main.cpp's loop bookkeeping for the trace. */
int SCENARIO_run(
    void (*step)(void),
    void (*loop_state)(int* gamestate_func_index, int* num_gamestate_funcs, int* meta_func_index)
);

#endif /* SCENARIO_H */

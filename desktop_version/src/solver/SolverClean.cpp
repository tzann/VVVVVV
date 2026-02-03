#include "SolverClean.h"

#include "CustomLevels.h"
#include "DeferCallbacks.h"
#include "Editor.h"
#include "Enums.h"
#include "Entity.h"
#include "Exit.h"
#include "FileSystemUtils.h"
#include "Font.h"
#include "Game.h"
#include "Graphics.h"
#include "Input.h"
#include "InterimVersion.h"
#include "KeyPoll.h"
#include "Logic.h"
#include "Map.h"
#include "Music.h"
#include "Network.h"
#include "preloader.h"
#include "ReleaseVersion.h"
#include "Render.h"
#include "RenderFixed.h"
#include "Screen.h"
#include "Script.h"
#include "UtilityClass.h"
#include "VFormat.h"
#include "Vlogging.h"

#include <chrono>
#include <queue>
#include <functional>
#include <unordered_set>
#include <algorithm>
#include <iostream>

namespace SolverClean {
    using namespace Geometry;
    using namespace Scenarios;
    using Exceptions::VVV_assert;
    using Exceptions::assert;
    using Terrain::RoomPosition;
    using Terrain::GlobalPosition;

    // Hash functions for various data types
    std::hash<bool> hash_bool;
    std::hash<int> hash_int;
    std::hash<uint64_t> hash_uint64;
    std::hash<float> hash_float;

    void runSolver(void) {
        RawScenario rs = WZ::SCENARIOS::MAZE_EXIT_TO_OBEY;
        SolverConfig solver;
        solver.clean_inputs = true;
        solver.debug_checks = false;
        solver.render_delay = 340;
        solver.max_solutions = 1;
        solver.mode = SolverMode::SIMPLE;

        CheckedScenario scenario = checkScenario(rs);

        solveScenario(solver, scenario);
    }

    static void solveScenario(SolverConfig& solver, const CheckedScenario& scenario) {
        /// States that have been processed - just enough information to reconstruct them later
        std::unordered_map<uint64_t, StateSummary, CustomHash> processed_states;
        /// The main data structure: A queue containing all not-yet processed states, ordered according to the solver config
        std::priority_queue<CachedSolverState, std::vector<CachedSolverState>, SolverConfig> queue(solver);
        /// A vector of solution states, in case we want to find several (or all) optimal solutions
        std::vector<StateSummary> solution_states;

        // Load the scenario
        loadScenario(scenario);

        // Create the initial state to solve from
        CachedSolverState init_state = cacheCurrentState(solver);
        uint64_t init_hash = init_state.hash();
        uint16_t init_heuristic = updateHeuristic(solver, scenario, init_state);
        queue.emplace(init_state);

        // Render the initial frame before starting
        key.clearKeys();
        solver.debug_info.clear();
        solver.solution_info.clear();
        render(solver);
        SDL_Delay(340);

        int solution_measure = INT_MAX;

        auto start_time = std::chrono::system_clock::now();
        auto last_render_time = std::chrono::system_clock::now();
        while (!queue.empty()) {
            CachedSolverState state = CachedSolverState(queue.top());
            queue.pop();

            uint64_t state_hash = state.hash();
            if (processed_states.find(state_hash) != processed_states.end()) {
                // We've already seen this state, skip it
                continue;
            } else if (state.getMeasure(solver.mode) > solution_measure) {
                // We've already found a solution better than this, so we must be finished
                break;
            }
            // Store the state summary, which allows us to reconstruct the solution later
            processed_states.emplace(state_hash, state.summary());

            bool should_render = false;
            // Update debug info
            {
                auto now_time = std::chrono::system_clock::now();
                solver.debug_info.millis = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_time).count();
                solver.debug_info.states = processed_states.size();
                solver.debug_info.min_heuristic = state.heuristic;
                solver.debug_info.max_measure = SDL_max(solver.debug_info.max_measure, state.getMeasure(solver.mode));
                solver.debug_info.queue_size = queue.size();
                solver.debug_info.cache_size = solver.cache.size();
                
                // Occasionally render debug info
                uint64_t millis_since_last_render = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_render_time).count();
                uint64_t rng = solver.render_delay <= 0 ? 0 : std::rand() % solver.render_delay;
                if (millis_since_last_render >= solver.render_delay / 2 + rng) {
                    should_render = true;
                    last_render_time = now_time;
                }
            }

            // Have we reached the goal?
            if (state.next_corner == scenario.corners.size()) {
                // Yes! We've passed the last corner. Add this state to the solution list
                solution_states.push_back(state.summary());
                assert(state.getMeasure(solver.mode) == solution_measure || solution_measure == INT_MAX);
                solution_measure = state.getMeasure(solver.mode);
                if (solution_states.size() >= solver.max_solutions) {
                    break;
                }
            }

            bool flip_held_too_long = state.game.jumpheld && state.game.jumppressed == 0;
            bool cant_flip = flip_held_too_long || !state.canFlip();
            uint8_t max_input = cant_flip ? 4 : 8;
            // Generate all possible subsequent states by iterating over all possible inputs
            for (uint8_t input = 0; input < max_input; input++) {
                bool left = input & 1;
                bool right = input & 2;
                bool flip = input & 4;
                // TODO: R, interact

                // Load the state we are continuing from
                loadState(solver, state);
                // Set the inputs accordingly
                key.clearKeys();
                key.setKey(KEYBOARD_LEFT, left);
                key.setKey(KEYBOARD_RIGHT, right);
                key.setKey(KEYBOARD_v, flip);
                // Advance the game by one frame
                doGameStep();

                // Hack: if we died, ignore this branch
                // This means we will never find death strats, but that's fine for now
                if (game.deathseq > 0) {
                    continue;
                }
                if (!scenario.ignore_rooms.empty()) {
                    // Skip rooms we know don't matter
                    RoomPosition rp = RoomPosition::FromNativeRoomCoords(game.roomx, game.roomy);
                    bool skip = false;
                    for (const RoomPosition& r : scenario.ignore_rooms) {
                        if (rp == r) {
                            skip = true;
                            break;
                        }
                    }
                    if (skip) continue;
                }

                // Render occasionally
                if (should_render) {
                    render(solver);
                    should_render = false;
                }
                // Cache the new state that we've reached (this already updates all counter variables and the heuristic)
                CachedSolverState new_state = cacheCurrentStateWithDeltaFromPrev(solver, scenario, state, state_hash);
                if (solver.debug_checks) {
                    assert(new_state.inputs == input);
                }

                // IMPORTANT: Check that the new heuristic is NOT lower than the old one
                // That would imply the heuristic is INADMISSIBLE, which means we are not guaranteed to find the optimal solution!
                if (new_state.heuristic < state.heuristic) {
                    render(solver);
                    cacheCurrentStateWithDeltaFromPrev(solver, scenario, state, state_hash);
                    updateHeuristic(solver, scenario, state);
                    updateHeuristic(solver, scenario, new_state);
                    Exceptions::inadmissible_heuristic();
                }

                // Finally: Add the new state to the queue. The insertion will preserve the ordering.
                queue.push(new_state);
            }
        }

        // Now we can clear the queue
        // Priority queues don't have a clear() function for some reason, so just reinitialize
        queue = std::priority_queue<CachedSolverState, std::vector<CachedSolverState>, SolverConfig>(solver);

        // Now we reconstruct the solution(s)! We just want a list of the inputs given each frame
        std::vector<std::vector<uint8_t>> solution_inputs;
        for (StateSummary& solution : solution_states) {
            std::vector<uint8_t> inputs;
            inputs.emplace_back(solution.inputs);

            // Walk back up the previous states, collecting inputs along the way
            uint64_t hash = solution.prev_hash;
            while (hash != init_hash) {
                // The previous state must always exist, since we stop when we reach the initial state
                StateSummary& prev = processed_states.at(hash);
                inputs.emplace_back(prev.inputs);
                hash = prev.prev_hash;
            }

            // We collected the inputs in reverse order, so reverse the inputs to account for it
            // TODO: we might be able to do this and copy into `solution_inputs` at the same time with an iterator that has negative stride
            std::reverse(inputs.begin(), inputs.end());

            // Make sure the input vectors have the right length
            if (solver.debug_checks) {
                assert(inputs.size() == solver.debug_info.max_measure);
                assert(inputs.size() == solver.debug_info.min_heuristic);
            }

            solution_inputs.emplace_back(inputs);
        }

        // Now we can also discard all the processed states we gathered
        processed_states.clear();

        // Reload the initial state before clearing the entity cache
        loadState(solver, init_state);
        solver.cache.clear();

        // Cache the initial state again, to repopulate the entity cache with only the necessary entries
        init_state = cacheCurrentState(solver);

        // Now we will repeatedly play back the solutions we've found
        while (true) {
            for (int idx = 0; idx < solution_inputs.size(); idx++) {
                const std::vector<uint8_t>& inputs = solution_inputs[idx];

                // Reset displayed solution info
                {
                    solver.solution_info.solution_idx = idx;
                    solver.solution_info.num_solutions = solution_inputs.size();
                    solver.solution_info.length = inputs.size();
                    solver.solution_info.frame = 0;
                    solver.solution_info.input = 0;
                    solver.solution_info.measure = 0;
                }

                // Render the initial frame, delay a bit longer
                loadState(solver, init_state);
                key.clearKeys();
                render(solver);
                SDL_Delay(10 * REGULAR_FRAME_DELAY);

                for (int f = 0; f < inputs.size(); f++) {
                    const uint8_t& i = f < inputs.size() ? inputs[f] : inputs.back();
                    bool left = i & 1;
                    bool right = i & 2;
                    bool flip = i & 4;

                    // Set inputs accordingly
                    key.clearKeys();
                    key.setKey(KEYBOARD_LEFT, left);
                    key.setKey(KEYBOARD_RIGHT, right);
                    key.setKey(KEYBOARD_v, flip);

                    // Update displayed solution info (will be rendered after next game step)
                    {
                        solver.solution_info.frame = f + 1;
                        solver.solution_info.input = i;
                        // TODO: get the actual measure here, which is a bit tricky - we have to know it before stepping to the next frame
                        solver.solution_info.measure = f + 1;
                    }

                    // Sanity checks
                    if (solver.debug_checks) {
                        if (f == inputs.size() - 1) {
                            // Compare on the second last frame since that's the final hash we store
                            uint64_t current_hash = cacheCurrentState(solver).hash();
                            assert(solution_states[idx].prev_hash == current_hash);
                        }
                    }

                    // Advance the game by one frame, don't disable rendering
                    doGameStep();
                    render(solver);
                    SDL_Delay(REGULAR_FRAME_DELAY);
                }
            }
        }
    }

    static uint16_t updateHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, CachedSolverState& state) {
        // assert(state.heuristic == 0);
        uint16_t h = state.getMeasure(solver.mode) + calcHeuristic(solver, scenario, state.next_corner, state.player, state.game);
        state.heuristic = h;
        return h;
    }

    static uint16_t calcHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game) {
        GlobalPosition pos = GlobalPosition(RoomPosition::FromNativeRoomCoords(game.roomx, game.roomy), IntVector(player.x, player.y));

        if (solver.debug_checks) {
            assert(0 <= next_corner && next_corner <= scenario.corners.size());
            if (next_corner < scenario.corners.size()) {
                const CheckedCorner& next = scenario.corners[next_corner];
                assert(!next.isPosAfter(pos));
            }
            if (next_corner >= 1) {
                const CheckedCorner& last = scenario.corners[next_corner - 1];
                assert(!last.isPosBefore(pos));
            }
        }

        // Advance to the next corner that we are strictly before
        int c_start_idx = next_corner;
        // TODO: handle warp cases more rigorously here
        while (c_start_idx < scenario.corners.size() && scenario.corners[c_start_idx].isPosNotStrictlyBefore(pos)) {
            const RoomData& pr = GetRoomData(game.getRoomPos());
            const RoomData& cr = GetRoomData(scenario.corners[c_start_idx].room);
            if (pr.warpx || pr.warpy || cr.warpx || cr.warpy) {
                break;
            }
            c_start_idx++;
        }

        // Call the appropriate heuristic function for the solver mode
        switch (solver.mode) {
        case SolverMode::SIMPLE:
            return calcSimpleHeuristic(solver, scenario, c_start_idx, player, game);
        case SolverMode::ACCEL_BASED_1:
            return calcAccelHeuristic1(solver, scenario, c_start_idx, player, game);
        default:
            Exceptions::todo();
        }
    }

    static uint16_t calcSimpleHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game) {
        int total_frames = 0;

        RoomPosition room_pos = game.getRoomPos();

        Region pos = Region(IntVector(
            ROOM_W * room_pos.rx + player.x,
            ROOM_H * room_pos.ry + player.y
        ));

        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            const CheckedCorner& c = scenario.corners[c_idx];
            int frames = calcSimpleHeuristicWarpCorner(room_pos, pos, c);
            total_frames += frames;

            // Sanity checks
            assert(!pos.is_bottom() && pos.is_bounded());
        }

        // We are now no longer before the last corner, but we may not yet be past it
        if (scenario.corners.back().isRegular()) {
            const CheckedCorner& c = scenario.corners.back();
            int frames = calcSimpleHeuristicFinalCorner(room_pos, pos, c);
            total_frames += frames;
        }

        return total_frames;
    }

    static uint16_t calcSimpleHeuristicCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c) {
        // This is the region of the corner that we must pass through to proceed
        const Region& c_r = c.region;
        // Account for map wrap-around, take nearest distance
        // TODO: this doesn't necessarily make sense in extreme cases
        if (c.room.rx - room_pos.rx > 10) {
            room_pos.rx += 20;
            pos.x += 20 * ROOM_W;
        }
        else if (room_pos.rx - c.room.rx > 10) {
            room_pos.rx -= 20;
            pos.x -= 20 * ROOM_W;
        }
        if (c.room.ry - room_pos.ry > 10) {
            room_pos.ry += 20;
            pos.y += 20 * ROOM_H;
        }
        else if (room_pos.ry - c.room.ry > 10) {
            room_pos.ry -= 20;
            pos.y -= 20 * ROOM_H;
        }
        // Factor in room offsets
        IntInterval c_x = c_r.x + (ROOM_W * c.room.rx);
        IntInterval c_y = c_r.y + (ROOM_H * c.room.ry);

        // Minimum absolute distance per dimension
        int x_d = IntInterval::min_diff_signed(pos.x, c_x);
        int y_d = IntInterval::min_diff_signed(pos.y, c_y);

        const RoomData& r1 = Terrain::GetRoomData(room_pos);
        const RoomData& r2 = Terrain::GetRoomData(c.room);

        // Factor in vertical corner cuts
        bool verticalCutUp = c.dir == UP_LEFT || c.dir == UP_RIGHT;
        bool verticalCutDown = c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
        IntInterval verticalCornerCutDist = IntInterval(verticalCutUp ? -MAX_VY : 0, verticalCutDown ? MAX_VY : 0);

        // How long will it take to get past the corner?
        int x_frames = div_ceil(std::abs(x_d), MAX_VX);
        int y_frames = div_ceil(std::abs(y_d), MAX_VY);
        int frames = SDL_max(x_frames, y_frames);

        // How far can we move during that time?
        IntInterval x_dist = VX_INT_RANGE * frames;
        IntInterval y_dist = VY_INT_RANGE * frames;

        // Update player position
        room_pos = c.room;
        pos.x += x_dist;
        pos.y += y_dist;

        // The space of reachable positions one frame after passing the corner
        Region postCornerRegion = Region(c_x, c_y + verticalCornerCutDist);

        // Restrict player position to post-corner region
        pos.intersect(postCornerRegion);

        // Sanity checks
        assert(!pos.is_bottom() && pos.is_bounded());

        // Update total frame count
        return frames;
    }

    static uint16_t calcSimpleHeuristicWarpCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c) {
        // This is the region of the corner that we must pass through to proceed
        const Region& c_r = c.region;
        // Account for map wrap-around, take nearest distance
        // TODO: this doesn't necessarily make sense in extreme cases
        if (c.room.rx - room_pos.rx > 10) {
            room_pos.rx += 20;
            pos.x += 20 * ROOM_W;
        }
        else if (room_pos.rx - c.room.rx > 10) {
            room_pos.rx -= 20;
            pos.x -= 20 * ROOM_W;
        }
        if (c.room.ry - room_pos.ry > 10) {
            room_pos.ry += 20;
            pos.y += 20 * ROOM_H;
        }
        else if (room_pos.ry - c.room.ry > 10) {
            room_pos.ry -= 20;
            pos.y -= 20 * ROOM_H;
        }
        // Factor in room offsets
        IntInterval c_x = c_r.x + (ROOM_W * c.room.rx);
        IntInterval c_y = c_r.y + (ROOM_H * c.room.ry);

        // Minimum absolute distance per dimension
        int x_d = IntInterval::min_diff_signed(pos.x, c_x);
        int x_d2 = 0;
        int y_d = IntInterval::min_diff_signed(pos.y, c_y);
        int y_d2 = 0;

        if (x_d == 0 && y_d == 0) {
            // Factor in vertical corner cuts
            bool verticalCutUp = c.dir == UP_LEFT || c.dir == UP_RIGHT;
            bool verticalCutDown = c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
            IntInterval verticalCornerCutDist = IntInterval(verticalCutUp ? -MAX_VY : 0, verticalCutDown ? MAX_VY : 0);

            room_pos = c.room;
            // The space of reachable positions one frame after passing the corner
            Region postCornerRegion = Region(c_x, c_y + verticalCornerCutDist);

            // Restrict player position to post-corner region
            pos.intersect(postCornerRegion);

            // Sanity checks
            assert(!pos.is_bottom() && pos.is_bounded());

            // Update total frame count
            return 0;
        }

        const RoomData& r1 = Terrain::GetRoomData(room_pos);
        const RoomData& r2 = Terrain::GetRoomData(c.room);

        bool notFromUp = c.dir == UP_LEFT || c.dir == UP_RIGHT || c.dir == LEFT_UP || c.dir == RIGHT_UP;
        bool notFromDown = c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT || c.dir == LEFT_DOWN || c.dir == RIGHT_DOWN;
        bool notFromLeft = c.dir == LEFT_UP || c.dir == LEFT_DOWN || c.dir == UP_LEFT || c.dir == DOWN_LEFT;
        bool notFromRight = c.dir == RIGHT_UP || c.dir == RIGHT_DOWN || c.dir == UP_RIGHT || c.dir == DOWN_RIGHT;
        bool is_vertical_cut = c.dir == UP_LEFT || c.dir == UP_RIGHT || c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
        bool is_horizontal_cut = c.dir == LEFT_UP || c.dir == LEFT_DOWN || c.dir == RIGHT_UP || c.dir == RIGHT_DOWN;
        
        // Factor in room warping
        if (r1.warpx || r1.warpy || r2.warpx || r2.warpy) {
            if (room_pos == c.room) {
                // Same room is special case, since we have to deal with X and Y warping simultaneously, but no screen edge turnarounds
                if (r1.warpx) {
                    if (!notFromLeft && x_d > 0) {
                        IntInterval c_x_off = c_x + ROOM_W;
                        int x_d_off = IntInterval::min_diff_signed(pos.x, c_x_off);
                        if ((is_vertical_cut && notFromRight) || std::abs(x_d_off) < std::abs(x_d)) {
                            x_d = x_d_off;
                            pos.x -= ROOM_W;
                        }
                    } else if (!notFromRight && x_d < 0) {
                        IntInterval c_x_off = c_x - ROOM_W;
                        int x_d_off = IntInterval::min_diff_signed(pos.x, c_x_off);
                        if ((is_vertical_cut && notFromLeft) || std::abs(x_d_off) < std::abs(x_d)) {
                            x_d = x_d_off;
                            pos.x += ROOM_W;
                        }
                    }
                }
                if (r1.warpy) {
                    if (!notFromUp && y_d > 0) {
                        IntInterval c_y_off = c_y + WARP_ROOM_H;
                        int y_d_off = IntInterval::min_diff_signed(pos.y, c_y_off);
                        if ((is_horizontal_cut && notFromDown) || std::abs(y_d_off) < std::abs(y_d)) {
                            y_d = y_d_off;
                            pos.y -= WARP_ROOM_H;
                        }
                    }
                    else if (!notFromDown && y_d < 0) {
                        IntInterval c_y_off = c_y - WARP_ROOM_H;
                        int y_d_off = IntInterval::min_diff_signed(pos.y, c_y_off);
                        if ((is_horizontal_cut && notFromUp) || std::abs(y_d_off) < std::abs(y_d)) {
                            y_d = y_d_off;
                            pos.y += WARP_ROOM_H;
                        }
                    }
                }
            }
            else if (room_pos.ry == c.room.ry) {
                // Can't handle more than 1 room offset for now - otherwise we have to deal with intermediate warping rooms...
                assert(std::abs(room_pos.rx - c.room.rx) <= 1);
                assert(room_pos.rx != c.room.rx);
                assert(!r1.warpx);
                if (!r1.warpx && r2.warpx) {
                    // Screen edge turnaround
                    if (room_pos.rx > c.room.rx && !notFromLeft) {
                        int screen_transition_x = ROOM_W * room_pos.rx + r1.GetMinXPos() - 1;
                        int turnaround_x = screen_transition_x - ROOM_W;
                        int x_d_1 = IntInterval::min_diff_signed(pos.x, screen_transition_x);
                        int x_d_2 = IntInterval::min_diff_signed(turnaround_x, c_x);
                        if ((is_vertical_cut && notFromRight) || std::abs(x_d_1) + std::abs(x_d_2) < std::abs(x_d)) {
                            x_d = x_d_1;
                            x_d2 = x_d_2;
                        }
                    }
                    else if (room_pos.rx < c.room.rx && !notFromRight) {
                        int screen_transition_x = ROOM_W * room_pos.rx + r1.GetMaxXPos() + 1;
                        int turnaround_x = screen_transition_x + ROOM_W;
                        int x_d_1 = IntInterval::min_diff_signed(pos.x, screen_transition_x);
                        int x_d_2 = IntInterval::min_diff_signed(turnaround_x, c_x);
                        if ((is_vertical_cut && notFromLeft) || std::abs(x_d_1) + std::abs(x_d_2) < std::abs(x_d)) {
                            x_d = x_d_1;
                            x_d2 = x_d_2;
                        }
                    }
                    else {
                        // Handle normally?
                        // Exceptions::unreachable();
                    }
                }
                // Handle y warps normally
                if (r1.warpy || r2.warpy) {
                    if (!notFromUp && y_d > 0) {
                        IntInterval c_y_off = c_y + WARP_ROOM_H;
                        int y_d_off = IntInterval::min_diff_signed(pos.y, c_y_off);
                        if ((is_horizontal_cut && notFromDown) || std::abs(y_d_off) < std::abs(y_d)) {
                            y_d = y_d_off;
                            pos.y -= WARP_ROOM_H;
                        }
                    }
                    else if (!notFromDown && y_d < 0) {
                        IntInterval c_y_off = c_y - WARP_ROOM_H;
                        int y_d_off = IntInterval::min_diff_signed(pos.y, c_y_off);
                        if ((is_horizontal_cut && notFromUp) || std::abs(y_d_off) < std::abs(y_d)) {
                            y_d = y_d_off;
                            pos.y += WARP_ROOM_H;
                        }
                    }
                }
            }
            else if (room_pos.rx == c.room.rx) {
                // Can't handle more than 1 room offset for now - otherwise we have to deal with intermediate warping rooms...
                assert(std::abs(room_pos.ry - c.room.ry) <= 1);
                assert(room_pos.ry != c.room.ry);
                assert(!r1.warpy);
                if (!r1.warpy && r2.warpy) {
                    // Screen edge turnaround
                    if (room_pos.ry > c.room.ry && !notFromUp) {
                        int screen_transition_y = ROOM_H * room_pos.ry + r1.GetMinYPos() - 1;
                        int turnaround_y = screen_transition_y - WARP_ROOM_H;
                        int y_d_1 = IntInterval::min_diff_signed(pos.y, screen_transition_y);
                        int y_d_2 = IntInterval::min_diff_signed(turnaround_y, c_y);
                        if ((is_horizontal_cut && notFromDown) || std::abs(y_d_1) + std::abs(y_d_2) < std::abs(y_d)) {
                            y_d = y_d_1;
                            y_d2 = y_d_2;
                        }
                    }
                    else if (room_pos.ry < c.room.ry && !notFromDown) {
                        int screen_transition_y = ROOM_H * room_pos.ry + r1.GetMaxYPos() + 1;
                        int turnaround_y = screen_transition_y + WARP_ROOM_H;
                        int y_d_1 = IntInterval::min_diff_signed(pos.y, screen_transition_y);
                        int y_d_2 = IntInterval::min_diff_signed(turnaround_y, c_y);
                        if ((is_horizontal_cut && notFromUp) || std::abs(y_d_1) + std::abs(y_d_2) < std::abs(y_d)) {
                            y_d = y_d_1;
                            y_d2 = y_d_2;
                        }
                    }
                    else {
                        // Handle normally?
                        // Exceptions::unreachable();
                    }
                }
                // Handle x warps normally
                if (r1.warpx) {
                    if (!notFromLeft && x_d > 0) {
                        IntInterval c_x_off = c_x + ROOM_W;
                        int x_d_off = IntInterval::min_diff_signed(pos.x, c_x_off);
                        if ((is_vertical_cut && notFromRight) || std::abs(x_d_off) < std::abs(x_d)) {
                            x_d = x_d_off;
                            pos.x -= ROOM_W;
                        }
                    }
                    else if (!notFromRight && x_d < 0) {
                        IntInterval c_x_off = c_x - ROOM_W;
                        int x_d_off = IntInterval::min_diff_signed(pos.x, c_x_off);
                        if ((is_vertical_cut && notFromLeft) || std::abs(x_d_off) < std::abs(x_d)) {
                            x_d = x_d_off;
                            pos.x += ROOM_W;
                        }
                    }
                }
            }
            else {
                Exceptions::todo();
            }
        }

        // Factor in vertical corner cuts
        bool verticalCutUp = c.dir == UP_LEFT || c.dir == UP_RIGHT;
        bool verticalCutDown = c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
        IntInterval verticalCornerCutDist = IntInterval(verticalCutUp ? -MAX_VY : 0, verticalCutDown ? MAX_VY : 0);

        // How long will it take to get past the corner?
        int x_frames = div_ceil(std::abs(x_d), MAX_VX);
        int y_frames = div_ceil(std::abs(y_d), MAX_VY);
        
        // How far can we move during that time?
        IntInterval x_dist = VX_INT_RANGE * x_frames;
        IntInterval y_dist = VY_INT_RANGE * y_frames;

        // Update player position
        pos.x += x_dist;
        pos.y += y_dist;

        if (x_d2 != 0) {
            if (x_d2 < 0) {
                // transition left, then go right
                int screen_transition_x = ROOM_W * room_pos.rx + r1.GetMinXPos() - 1;
                pos.x.addUpperBound(screen_transition_x);
                pos.x -= ROOM_W;
            }
            else {
                // transition right, then go left
                int screen_transition_x = ROOM_W * room_pos.rx + r1.GetMaxXPos() + 1;
                pos.x.addLowerBound(screen_transition_x);
                pos.x += ROOM_W;
            }

            int x_frames2 = div_ceil(std::abs(x_d2), MAX_VX);
            IntInterval x_dist2 = VX_INT_RANGE * x_frames2;
            x_frames += x_frames2;
            pos.x += x_dist2;
        }
        if (y_d2 != 0) {
            if (y_d2 < 0) {
                // transition up, then go down
                int screen_transition_y = ROOM_H * room_pos.ry + r1.GetMaxYPos() + 1;
                pos.y.addLowerBound(screen_transition_y);
                pos.y -= WARP_ROOM_H;
            }
            else {
                // transition down, then go up
                int screen_transition_y = ROOM_H * room_pos.ry + r1.GetMinYPos() - 1;
                pos.y.addUpperBound(screen_transition_y);
                pos.y += WARP_ROOM_H;
            }

            int y_frames2 = div_ceil(std::abs(y_d2), MAX_VY);
            IntInterval y_dist2 = VY_INT_RANGE * y_frames2;
            y_frames += y_frames2;
            pos.y += y_dist2;
        }

        int frames = SDL_max(x_frames, y_frames);
        int x_frames_left = frames - x_frames;
        int y_frames_left = frames - y_frames;

        x_dist = VX_INT_RANGE * x_frames_left;
        y_dist = VY_INT_RANGE * y_frames_left;

        // Update player position
        room_pos = c.room;
        pos.x += x_dist;
        pos.y += y_dist;
         
        // The space of reachable positions one frame after passing the corner
        Region postCornerRegion = Region(c_x, c_y + verticalCornerCutDist);

        // Restrict player position to post-corner region
        pos.intersect(postCornerRegion);

        // Sanity checks
        assert(!pos.is_bottom() && pos.is_bounded());

        // Update total frame count
        return frames;
    }

    static uint16_t calcSimpleHeuristicFinalCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c) {
        // This is the region of the corner that we have to reach
        Region c_r = c.getRegionAfter();
        // Account for map wrap-around, take nearest distance
        // TODO: this doesn't necessarily make sense in extreme cases
        if (c.room.rx - room_pos.rx > 10) {
            room_pos.rx += 20;
            pos.x += 20 * ROOM_W;
        }
        else if (room_pos.rx - c.room.rx > 10) {
            room_pos.rx -= 20;
            pos.x -= 20 * ROOM_W;
        }
        if (c.room.ry - room_pos.ry > 10) {
            room_pos.ry += 20;
            pos.y += 20 * ROOM_H;
        }
        else if (room_pos.ry - c.room.ry > 10) {
            room_pos.ry -= 20;
            pos.y -= 20 * ROOM_H;
        }
        // Factor in room offsets
        IntInterval c_x = c_r.x + (ROOM_W * c.room.rx);
        IntInterval c_y = c_r.y + (ROOM_H * c.room.ry);

        // Minimum absolute distance per dimension
        int x_d = (pos.x - c_x).abs().min;
        int y_d = (pos.y - c_y).abs().min;

        // How long will it take to get past the corner?
        int x_frames = div_ceil(x_d, MAX_VX);
        int y_frames = div_ceil(y_d, MAX_VY);
        int frames = SDL_max(x_frames, y_frames);

        return frames;
    }

    static uint16_t calcAccelHeuristic1(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game) {
        int total_frames = 0;

        RoomPosition room_pos = game.getRoomPos();
        Region pos = Region(IntVector(
            ROOM_W * room_pos.rx + player.x,
            ROOM_H * room_pos.ry + player.y
        ));

        FloatInterval vx = FloatInterval(player.vx);
        FloatInterval vy = FloatInterval(player.vy);
        // 1 is regular gravity, -1 inverted
        IntInterval gravity3 = game.gravitycontrol ? -3 : 3;
        IntInterval gravity4 = game.gravitycontrol ? -4 : 4;

        bool can_flip = canFlip(player, game);
        bool can_double_flip = canDoubleFlip(player, game);

        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            const CheckedCorner& c = scenario.corners[c_idx];

            // This is the region of the corner that we must pass through to proceed
            const Region& c_r = c.region;
            // Account for map wrap-around, take nearest distance
            // TODO: this doesn't necessarily make sense in extreme cases
            if (c.room.rx - room_pos.rx > 10) {
                room_pos.rx += 20;
                pos.x += 20 * ROOM_W;
            }
            else if (room_pos.rx - c.room.rx > 10) {
                room_pos.rx -= 20;
                pos.x -= 20 * ROOM_W;
            }
            if (c.room.ry - room_pos.ry > 10) {
                room_pos.ry += 20;
                pos.y += 20 * ROOM_H;
            }
            else if (room_pos.ry - c.room.ry > 10) {
                room_pos.ry -= 20;
                pos.y -= 20 * ROOM_H;
            }
            // Factor in room offsets
            IntInterval c_x = c_r.x + (ROOM_W * c.room.rx);
            IntInterval c_y = c_r.y + (ROOM_H * c.room.ry);

            // Minimum absolute distance per dimension
            int x_d = IntInterval::min_diff_signed(pos.x, c_x);
            int y_d = IntInterval::min_diff_signed(pos.y, c_y);
            // assert(x_d != 0 || y_d != 0);
            bool is_trinket_or_warp = c.isTrinketOrWarp();
            bool is_vertical_cut = c.dir == UP_LEFT || c.dir == UP_RIGHT || c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
            bool is_horizontal_cut = c.dir == LEFT_UP || c.dir == LEFT_DOWN || c.dir == RIGHT_UP || c.dir == RIGHT_DOWN;
            bool is_limited_by_x_d = is_vertical_cut || is_trinket_or_warp;
            bool is_limited_by_y_d = is_horizontal_cut || is_trinket_or_warp;

            bool goingLeft = x_d > 0;
            bool goingRight = x_d < 0;
            bool goingUp = y_d > 0;
            bool goingDown = y_d < 0;

            // Assume we can always instantly stop (since we might hit a wall)
            vx.join(0.0f);
            vy.join(0.0f);

            // Accelerate to full speed
            while (
                (x_d != 0 || y_d != 0) &&
                    ((vx.min > -MAX_VX)
                        || (vx.max < MAX_VX)
                        || (vy.min > -MAX_VY)
                        || (vy.max < MAX_VY))) {
                IntInterval ax = IntInterval(-3, 3);
                if (can_flip) {
                    IntInterval tmp = gravity4;
                    gravity4.negate();
                    vy.join(gravity4);

                    gravity3.join(gravity3.negated());
                    gravity4.join(tmp);
                }
                if (can_double_flip && gravity4.max == 4) {
                    // Doesn't change gravity, just sets speed
                    vy.join(4);
                }

                IntInterval ay = gravity3;

                // Update velocities
                vx += ax;
                vy += ay;

                // Apply friction and speed caps
                FloatInterval vx_neg = vx + X_RATE;
                vx_neg.addLowerBound(-MAX_VX);
                vx_neg.max = 0;

                vx -= X_RATE;
                vx.addUpperBound(MAX_VX);
                vx.min = 0;
                vx.join(vx_neg);

                FloatInterval vy_neg = vy + Y_RATE;
                vy_neg.addLowerBound(-MAX_VY);
                vy_neg.max = 0;

                vy -= Y_RATE;
                vy.addUpperBound(MAX_VY);
                vy.min = 0;
                vy.join(vy_neg);

                // Update positions
                // This applies conservative rounding, so we always get max distance
                pos.x += vx.toIntInterval();
                pos.y += vy.toIntInterval();

                // Update x_d and y_d
                x_d = IntInterval::min_diff_signed(pos.x, c_x);
                y_d = IntInterval::min_diff_signed(pos.y, c_y);

                total_frames++;

                can_flip = true;
                can_double_flip = true;
            }

            // How long will it take to get past the corner?
            int x_frames = div_ceil(SDL_abs(x_d), MAX_VX);
            int y_frames = div_ceil(SDL_abs(y_d), MAX_VY);
            int frames = SDL_max(x_frames, y_frames);
            if (is_limited_by_x_d && is_limited_by_y_d) {
                is_limited_by_x_d &= x_frames >= y_frames;
                is_limited_by_y_d &= y_frames >= x_frames;
            }

            // How far can we move during that time?
            if (frames > 0) {
                vx = FULL_X_SPEED_RANGE;
                vy = FULL_Y_SPEED_RANGE;
            }
            IntInterval x_dist = VX_INT_RANGE * frames;
            IntInterval y_dist = VY_INT_RANGE * frames;

            // Update player position
            room_pos = c.room;
            pos.x += x_dist;
            pos.y += y_dist;

            // Update velocity
            if (goingLeft && is_limited_by_x_d) {
                vx.addUpperBound(0);
            }
            if (goingRight && is_limited_by_x_d) {
                vx.addLowerBound(0);
            }
            if (goingUp && is_limited_by_y_d) {
                vy.addUpperBound(0);
            }
            if (goingDown && is_limited_by_y_d) {
                vy.addLowerBound(0);
            }

            // Factor in vertical corner cuts
            bool verticalCutUp = c.dir == UP_LEFT || c.dir == UP_RIGHT;
            bool verticalCutDown = c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT;
            IntInterval verticalCornerCutDist = IntInterval(verticalCutUp ? -MAX_VY : 0, verticalCutDown ? MAX_VY : 0);

            // The space of reachable positions one frame after passing the corner
            Region postCornerRegion = Region(c_x, c_y + verticalCornerCutDist);

            // Restrict player position to post-corner region
            pos.intersect(postCornerRegion);

            // Sanity checks
            assert(!pos.is_bottom() && pos.is_bounded());

            // Update total frame count
            total_frames += frames;

            can_flip = true;
            can_double_flip = true;
        }

        // We are now no longer before the last corner, but we may not yet be past it
        if (scenario.corners.back().isRegular()) {
            const CheckedCorner& c = scenario.corners.back();
            // This is the region of the corner that we have to reach
            Region c_r = c.getRegionAfter();
            // Account for map wrap-around, take nearest distance
            // TODO: this doesn't necessarily make sense in extreme cases
            if (c.room.rx - room_pos.rx > 10) {
                room_pos.rx += 20;
                pos.x += 20 * ROOM_W;
            }
            else if (room_pos.rx - c.room.rx > 10) {
                room_pos.rx -= 20;
                pos.x -= 20 * ROOM_W;
            }
            if (c.room.ry - room_pos.ry > 10) {
                room_pos.ry += 20;
                pos.y += 20 * ROOM_H;
            }
            else if (room_pos.ry - c.room.ry > 10) {
                room_pos.ry -= 20;
                pos.y -= 20 * ROOM_H;
            }
            // Factor in room offsets
            IntInterval c_x = c_r.x + (ROOM_W * c.room.rx);
            IntInterval c_y = c_r.y + (ROOM_H * c.room.ry);

            // Minimum absolute distance per dimension
            int x_d = (pos.x - c_x).abs().min;
            int y_d = (pos.y - c_y).abs().min;

            // How long will it take to get past the corner?
            int x_frames = div_ceil(x_d, MAX_VX);
            int y_frames = div_ceil(y_d, MAX_VY);
            int frames = SDL_max(x_frames, y_frames);

            total_frames += frames;
        }

        return total_frames;
    }

    static CheckedScenario checkScenario(const RawScenario& raw_scenario) {
        using Terrain::RoomPosition;

        std::vector<CheckedCorner> checked_corners;
        checked_corners.reserve(raw_scenario.corners.size());

        // Initialize first and last rooms (to bound connected rooms in-between)
        RoomPosition first_room = RoomPosition::FromNativeRoomCoords(raw_scenario.init_rx, raw_scenario.init_ry);
        const RawCorner& last_c = raw_scenario.corners.back();
        RoomPosition last_room = RoomPosition::FromNativeRoomCoords(last_c.rx, last_c.ry);
        Terrain::InitializeBasicRoomData(first_room);
        Terrain::InitializeBasicRoomData(last_room);

        // TODO: we could also check for ill-formed corner sequences here
        // Although that gets tricky in the presence of warps

        RoomPosition current_room = RoomPosition(-1, -1);
        for (int c_idx = 0; c_idx < raw_scenario.corners.size(); c_idx++) {
            const RawCorner& c = raw_scenario.corners[c_idx];
            bool shiny_or_warp = c.dir == CornerDir::TRINKET || c.dir == CornerDir::WARP_TOKEN;

            RoomPosition c_room = RoomPosition::FromNativeRoomCoords(c.rx, c.ry);;
            if (c_room != current_room) {
                current_room = c_room;
                // Load the next room, initialize data
                Terrain::LoadRoom(current_room);
                Terrain::InitializeBasicRoomData(current_room);
            }

            // Does the room warp?
            bool warpx = map.warpx;
            bool warpy = map.warpy;

            int min_x_pos = warpx ? -9 : -14;
            int min_y_pos = warpy ? -11 : -2;
            int max_x_pos = warpx ? 310 : 307;
            int max_y_pos = warpy ? 226 : 237;

            IntVector min = IntVector(min_x_pos - 1, min_y_pos - 1);
            IntVector max = IntVector(max_x_pos + 1, max_y_pos + 1);

            // The corner should have valid coordinates for this room
            VVV_assert(min.x < c.x && c.x < max.x, 572701);
            VVV_assert(min.y < c.y && c.y < max.y, 572702);

            uint8_t* collision_bitmap = Terrain::GetCurrentRoomPlayerCollisionBitmap(min, max);
            int bitmap_width = max.x - min.x + 1;

            if (!shiny_or_warp) {
                // Simple case: just double check we have the right corner type (and pos)
                Terrain::CornerType c_ty;
                switch (c.dir) {
                case UP_LEFT:
                case RIGHT_DOWN:
                    c_ty = Terrain::CornerType::BottomLeft;
                    break;
                case UP_RIGHT:
                case LEFT_DOWN:
                    c_ty = Terrain::CornerType::BottomRight;
                    break;
                case DOWN_LEFT:
                case RIGHT_UP:
                    c_ty = Terrain::CornerType::TopLeft;
                    break;
                case DOWN_RIGHT:
                case LEFT_UP:
                    c_ty = Terrain::CornerType::TopRight;
                    break;
                }

                // The corner position itself should not be occupied (for this solver)
                VVV_assert(!collision_bitmap[bitmap_width * (c.y - min.y) + (c.x - min.x)], 570000 + c_idx);

                int occupied_count = 0;
                bool above_occupied = false;
                bool left_occupied = false;
                bool below_occupied = false;
                bool right_occupied = false;
                for (int px = c.x - 1; px <= c.x + 1; px++) {
                    for (int py = c.y - 1; py <= c.y + 1; py++) {
                        if (collision_bitmap[bitmap_width * (py - min.y) + (px - min.x)]) {
                            occupied_count++;
                            above_occupied |= py < c.y;
                            left_occupied |= px < c.x;
                            below_occupied |= py > c.y;
                            right_occupied |= px > c.x;
                        }
                    }
                }
                // A correct corner position for a TopLeft corner would only have the bottom right pixel of the 3x3 range around it occupied
                // Hence any physical corner should only have one occupied pixel
                VVV_assert(occupied_count == 1, 571000 + c_idx);
                // Exactly two of these conditions hold when a corner is occupied
                VVV_assert(above_occupied + left_occupied + below_occupied + right_occupied == 2, 572000 + c_idx);
                // Make sure the corner type matches the physical corner
                if (above_occupied && left_occupied) {
                    VVV_assert(c_ty == Terrain::CornerType::BottomRight && !below_occupied && !right_occupied, 573000 + c_idx);
                }
                else if (above_occupied && right_occupied) {
                    VVV_assert(c_ty == Terrain::CornerType::BottomLeft && !below_occupied && !left_occupied, 573000 + c_idx);
                }
                else if (below_occupied && left_occupied) {
                    VVV_assert(c_ty == Terrain::CornerType::TopRight && !above_occupied && !right_occupied, 573000 + c_idx);
                }
                else if (below_occupied && right_occupied) {
                    VVV_assert(c_ty == Terrain::CornerType::TopLeft && !above_occupied && !left_occupied, 573000 + c_idx);
                }
                else {
                    Exceptions::unreachable();
                }

                // Now let's check the vertical and horizontal space next to the corner
                int x_increment = left_occupied ? 1 : -1;
                int y_increment = above_occupied ? 1 : -1;
                int x_gap = c.x;
                for (; min.x < x_gap && x_gap < max.x; x_gap += x_increment) {
                    int y = c.y - y_increment;
                    if (collision_bitmap[bitmap_width * (y - min.y) + (x_gap - min.x)]) {
                        // Cell is occupied, gap ends here
                        break;
                    }
                }
                IntInterval x_ival = IntInterval(SDL_min(c.x, x_gap), SDL_max(c.x, x_gap));
                if (x_gap <= min.x) {
                    x_ival.removeLowerBound();
                }
                else if (x_gap >= max.x) {
                    x_ival.removeUpperBound();
                }
                int y_gap = c.y;
                for (; min.y < y_gap && y_gap < max.y; y_gap += y_increment) {
                    int x = c.x - x_increment;
                    if (collision_bitmap[bitmap_width * (y_gap - min.y) + (x - min.x)]) {
                        // Cell is occupied, gap ends here
                        break;
                    }
                }
                IntInterval y_ival = IntInterval(SDL_min(c.y, y_gap), SDL_max(c.y, y_gap));
                if (y_gap <= min.y) {
                    y_ival.removeLowerBound();
                }
                if (y_gap >= max.y) {
                    y_ival.removeUpperBound();
                }

                Region reg = Region(x_ival, y_ival);
                CheckedCorner cc = CheckedCorner(c_room, reg, c.dir);
                // TODO emplace instead of push
                checked_corners.push_back(cc);
            }
            else {
                // Trinket or warp token - let's see if the hitbox is partially OoB and update it
                // In some cases, the trinket even overlaps with a corner of terrain, but in these cases we should probably just leave it as is
                // So just find the smallest AABB that contains the entire trinket hitbox (i.e. the range of positions the player can be at while collecting it)
                Region box = Region::bottom();

                // This isn't efficient but it's a one-time cost at the start of execution so who cares
                // TODO: do we have to worry about warping trinket hitboxes around? hopefully not
                for (int px = c.x + TRINKET_X_MIN; px <= c.x + TRINKET_X_MAX; px++) {
                    if (min.x > px || px > max.x) continue;
                    for (int py = c.y + TRINKET_Y_MIN; py <= c.y + TRINKET_Y_MAX; py++) {
                        if (min.y > py || py > max.y) continue;
                        if (!collision_bitmap[bitmap_width * (py - min.y) + (px - min.x)]) {
                            // The trinket can be collected, i.e. this pos is not OoB
                            Region point = Region(IntVector(px, py));
                            box.join(point);
                        }
                    }
                }

                // Sanity check: At least one pixel of the trinket is not OoB
                VVV_assert(!box.is_bottom() && box.is_bounded(), 573000 + c_idx);

                CheckedCorner cc = CheckedCorner(c_room, box, c.dir);
                // TODO emplace instead of push
                checked_corners.push_back(cc);
            }

            // Free the collision bitmap
            SDL_free((void*)collision_bitmap);
        }

        RoomPosition init_room = RoomPosition::FromNativeRoomCoords(raw_scenario.init_rx, raw_scenario.init_ry);
        IntVector init_pos = IntVector(raw_scenario.init_x, raw_scenario.init_y);
        CheckedScenario res = CheckedScenario(init_room, init_pos, raw_scenario.init_gravity != 0, raw_scenario.frames_to_advance, checked_corners);
        return res;
    }

    static void loadScenario(const CheckedScenario& scenario) {
        // Get into gamemode gracefully
        game.setstate(0);
        // Give back trinkets
        for (int i = 0; i < 100; i++) {
            obj.collect[i] = false;
        }
        // Don't do cutscenes or trinket textboxes
        game.nocutscenes = true;
        game.intimetrial = true;
        // Clear keys
        key.clearKeys();
        // Don't poll the keyboard
        key.actually_poll = false;
        // Make the game think it has focus
        key.isActive = true;

        // Save my ears from permanent damage
        game.muted = true;
        music.updatemutestate();

        // Turn off screen effects
        game.colourblindmode = true;
        game.noflashingmode = true;

        // Set up the initial player state
        game.savex = scenario.init_pos.x;
        game.savey = scenario.init_pos.y;
        game.saverx = scenario.init_room.rx;
        game.savery = scenario.init_room.ry;
        game.savegc = scenario.inv_gravity;
        game.savedir = 1;
        game.savepoint = 0;
        game.gravitycontrol = game.savegc;

        game.state = 0;
        game.deathseq = -1;
        game.lifeseq = 0;

        // Reset entities and create player at correct location
        obj.entities.clear();
        obj.createentity(game.savex, game.savey, 0, 0);

        // Do as the game would do
        map.resetplayer();
        Terrain::LoadRoom(scenario.init_room);
        map.initmapdata();

        // Skip fadein
        graphics.fademode = FADE_NONE;

        // Reset button states just in case
        game.press_action = false;
        game.press_left = false;
        game.press_right = false;
        game.press_interact = false;
        game.press_map = false;

        game.jumppressed = false;
        game.jumpheld = 0;
        game.interactheld = false;
        game.tapleft = 0;
        game.tapright = 0;

        /* If we are spawning in a tower, ensure variables are set correctly */
        if (map.towermode)
        {
            map.resetplayer();

            map.ypos = obj.entities[0].yp - 120;
            map.oldypos = map.ypos;

            map.setbgobjlerp(graphics.towerbg);
            map.cameramode = 0;
            map.colsuperstate = 0;
        }

        // Hack: Advance frames to fix enemy cycles to right position
        for (int i = 0; i < scenario.advance_frames; i++) {
            key.clearKeys();
            doGameStep();
        }

        key.clearKeys();
    }

    static CachedSolverState cacheCurrentStateWithDeltaFromPrev(SolverConfig& solver, const CheckedScenario& scenario, const CachedSolverState& prev, uint64_t prevHash) {
        CachedSolverState state = cacheCurrentState(solver);
        // The global coordinates of the player
        const GlobalPosition pos = state.getGlobalPos();
        // The current inputs to the game
        bool left = key.isDown(SDLK_LEFT);
        bool right = key.isDown(SDLK_RIGHT);
        bool flip = key.isDown(SDLK_v);
        bool reset = key.isDown(SDLK_r);
        bool talk = key.isDown(SDLK_RETURN);
        uint8_t inputs = 0;
        inputs = (inputs << 1) | talk;
        inputs = (inputs << 1) | reset;
        inputs = (inputs << 1) | flip;
        inputs = (inputs << 1) | right;
        inputs = (inputs << 1) | left;

        int next_corner = prev.next_corner;
        // Have we passed the next corner in the scenario?
        if (next_corner < scenario.corners.size()) {
            const CheckedCorner& next = scenario.corners[next_corner];
            if (next.isPosAfter(pos)) {
                next_corner++;
            }
        }
        // Did we go back around the previous corner?
        if (next_corner == prev.next_corner && next_corner > 0) {
            const CheckedCorner& last = scenario.corners[next_corner - 1];
            if (last.isPosBefore(pos)) {
                next_corner--;
            }
        }
        state.next_corner = next_corner;

        // Update frame counter
        state.frame_count = prev.frame_count + 1;

        // Update input counters
        uint8_t changed_inputs = prev.inputs ^ inputs;
        uint16_t num_changed_inputs = std::_Popcount(changed_inputs);
        assert(0 <= num_changed_inputs && num_changed_inputs <= 3);
        uint16_t num_input_frames = std::_Popcount(inputs);
        assert(0 <= num_input_frames && num_input_frames <= 3);
        state.input_change_count += num_changed_inputs;
        state.input_frame_count += num_input_frames;

        // Update flip counter, although this is non-trivial to deduce - might be worth testing
        if (flip) {
            if (prev.canDoubleFlip()) {
                state.flip_count += 2;
            }
            else if (prev.canFlip()) {
                state.flip_count++;
            }
        }

        // Count the number of frames where both left and right were pressed at the same time
        // We want to avoid this if possible, because we want to know if it's a forced element of the solution
        if (left && right) {
            state.num_l_plus_r++;
        }

        // Store current inputs, which should not yet have changed
        state.inputs = inputs;

        // Store hash of previous state for reconstruction
        state.prevHash = prevHash == 0 ? prev.hash() : prevHash;

        // Finally, recalculate the heuristic for this new 
        // Calling this already sets `state.heuristic`
        updateHeuristic(solver, scenario, state);
        return state;
    }

    static CachedSolverState cacheCurrentState(SolverConfig& solver) {
        CachedSolverState state;

        state.game.roomx = game.roomx;
        state.game.roomy = game.roomy;

        state.player.x = obj.entities[0].xp;
        state.player.y = obj.entities[0].yp;
        // TODO: approximate floats as 1-decimal fixed-point values
        state.player.vx = obj.entities[0].vx;
        state.player.vy = obj.entities[0].vy;
        state.player.ax = obj.entities[0].ax;
        state.player.ay = obj.entities[0].ay;

        // Any value less than 0 is equivalent to 0
        int8_t effective_onground = SDL_max(obj.entities[0].onground, 0);
        int8_t effective_onroof = SDL_max(obj.entities[0].onroof, 0);
        state.player.onground = effective_onground;
        state.player.onroof = effective_onroof;
        state.player.dir = obj.entities[0].dir;

        // s.player.rule = obj.entities[0].rule;
        state.player.tile = obj.entities[0].tile;

        // Any value less than 0 is equivalent to 0
        int8_t effective_framedelay = SDL_max(obj.entities[0].framedelay, 0);
        state.player.framedelay = effective_framedelay;
        state.player.drawframe = obj.entities[0].drawframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_visualonground = SDL_max(obj.entities[0].visualonground, 0);
        int8_t effective_visualonroof = SDL_max(obj.entities[0].visualonroof, 0);
        state.player.visualonground = effective_visualonground;
        state.player.visualonroof = effective_visualonroof;
        state.player.walkingframe = obj.entities[0].walkingframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_collisionframedelay = SDL_max(obj.entities[0].collisionframedelay, 0);
        state.player.collisiondrawframe = obj.entities[0].collisiondrawframe;
        state.player.collisionframedelay = effective_collisionframedelay;
        state.player.collisionwalkingframe = obj.entities[0].collisionwalkingframe;

        state.player.newxp = obj.entities[0].newxp;
        state.player.newyp = obj.entities[0].newyp;

        state.game.state = game.state;
        state.game.deathseq = game.deathseq;
        state.game.lifeseq = game.lifeseq;
        state.game.gravitycontrol = game.gravitycontrol;
        state.game.press_action = game.press_action;
        state.game.jumpheld = game.jumpheld;
        state.game.jumppressed = game.jumppressed;
        state.game.press_right = game.press_right;
        state.game.press_left = game.press_left;

        // Any value greater than 5 is equivalent to 5
        int8_t effective_tapright = SDL_min(game.tapright, 5);
        int8_t effective_tapleft = SDL_min(game.tapleft, 5);
        state.game.tapright = effective_tapright;
        state.game.tapleft = effective_tapleft;

        if (state.game.roomx == 113 && state.game.roomy == 104) {
            // Hack: don't save entities or blocks in Comms Relay
        }
        else {
            state.cache_entry = createCacheEntry(solver);
        }

        state.collect = 0;
        for (int i = 0; i < 20; i++) {
            state.collect |= obj.collect[i] << i;
        }

        // Init everything else with zeroes
        state.heuristic = 0.0;
        state.next_corner = 0;

        state.frame_count = 0;
        state.input_change_count = 0;
        state.input_frame_count = 0;
        state.flip_count = 0;

        state.num_l_plus_r = 0;
        state.inputs = 0;
        state.prevHash = 0;

        return state;
    }

    static CacheEntry createCacheEntry(SolverConfig& solver) {
        std::vector<uint64_t> entities;
        entities.reserve(obj.entities.size() - 1);
        for (size_t i = 1; i < obj.entities.size(); i++) {
            // Skip deleted entities
            // TODO: is this safe? any unexpected side effects?
            // -> Yes, this breaks trinket collecting
            /* if (obj.entities[i].invis && obj.entities[i].size == -1 && obj.entities[i].type == -1 && obj.entities[i].rule == -1 && !obj.entities[i].isplatform) {
                continue;
            } */
            EntityState e;
            e.type = obj.entities[i].type;
            e.rule = obj.entities[i].rule;
            e.x = obj.entities[i].xp;
            e.y = obj.entities[i].yp;
            e.vx = obj.entities[i].vx;
            e.vy = obj.entities[i].vy;
            // e.ax = obj.entities[i].ax;
            // e.ay = obj.entities[i].ay;
            e.behave = obj.entities[i].behave;
            // e.para = obj.entities[i].para;
            e.state = obj.entities[i].state;
            e.onwall = obj.entities[i].onwall;
            // Any value less than 0 is equivalent to 0
            int effective_statedelay = SDL_max(obj.entities[i].statedelay, 0);
            e.statedelay = effective_statedelay;

            e.tile = obj.entities[i].tile;
            // e.animate = obj.entities[i].animate;
            // Any value less than 0 is equivalent to 0
            int effective_framedelay = SDL_max(obj.entities[i].framedelay, 0);
            e.framedelay = effective_framedelay;
            e.walkingframe = obj.entities[i].walkingframe;
            e.drawframe = obj.entities[i].drawframe;

            e.invis = obj.entities[i].invis;
            e.life = obj.entities[i].life;
            e.onentity = obj.entities[i].onentity;

            entities.push_back(solver.cache.cacheEntity(e));
        }
        uint64_t entity_set = solver.cache.cacheEntitySet(entities);

        std::vector<uint64_t> blocks;
        blocks.reserve(obj.blocks.size());
        for (size_t i = 0; i < obj.blocks.size(); i++) {
            // Skip deleted blocks
            if (obj.blocks[i].wp == 0 && obj.blocks[i].hp == 0 && obj.blocks[i].rect.w == 0 && obj.blocks[i].rect.h == 0) {
                continue;
            }
            // These blocks can't change state
            if (obj.blocks[i].type == DAMAGE || obj.blocks[i].type == DIRECTIONAL || obj.blocks[i].type == SAFE) {
                continue;
            }
            // TODO: are these skips safe? any unexpected side effects?

            BlockState b;
            b.rect_x = obj.blocks[i].rect.x;
            b.rect_y = obj.blocks[i].rect.y;
            b.rect_w = obj.blocks[i].rect.w;
            b.rect_h = obj.blocks[i].rect.h;
            b.type = obj.blocks[i].type;
            b.trigger = obj.blocks[i].trigger;
            // b.xp = obj.blocks[i].xp;
            // b.yp = obj.blocks[i].yp;
            // b.wp = obj.blocks[i].wp;
            // b.hp = obj.blocks[i].hp;
            // b.script = obj.blocks[i].script;
            // b.prompt = obj.blocks[i].prompt;
            // b.r = obj.blocks[i].r;
            // b.g = obj.blocks[i].g;
            // b.b = obj.blocks[i].b;
            // b.activity_y = obj.blocks[i].activity_y;

            blocks.push_back(solver.cache.cacheBlock(b));
        }
        uint64_t block_set = solver.cache.cacheBlockSet(blocks);

        return CacheEntry(entity_set, block_set);
    }

    static void loadState(const SolverConfig& solver, const CachedSolverState& state) {
        // Only change rooms if it's necessary.
        if (game.roomx != state.game.roomx || game.roomy != state.game.roomy) {
            // Reset trinket states to keep entity slots consistent
            for (int i = 0; i < 20; i++) {
                obj.collect[i] = 0;
            }
            map.gotoroom(state.game.roomx, state.game.roomy);
        }
        else {
            // Delete trinkets that should no longer be around
            for (int e = 0; e < obj.entities.size(); e++) {
                if (obj.entities[e].rule == 3 && obj.entities[e].type == 7) {
                    int trinket_idx = obj.entities[e].para;
                    if ((state.collect >> trinket_idx) & 1) {
                        // Delete this entity as the game itself would
                        obj.disableentity(e);
                    }
                }
            }
        }
        // Set the actual collect state
        for (int i = 0; i < 20; i++) {
            obj.collect[i] = (state.collect >> i) & 1;
        }

        // Load player data
        obj.entities[0].xp = state.player.x;
        obj.entities[0].yp = state.player.y;
        obj.entities[0].vx = state.player.vx;
        obj.entities[0].vy = state.player.vy;
        obj.entities[0].ax = state.player.ax;
        obj.entities[0].ay = state.player.ay;
        obj.entities[0].onground = state.player.onground;
        obj.entities[0].onroof = state.player.onroof;
        obj.entities[0].dir = state.player.dir;

        // obj.entities[0].rule = s.player.rule;
        obj.entities[0].rule = 0;
        obj.entities[0].tile = state.player.tile;
        obj.entities[0].framedelay = state.player.framedelay;
        obj.entities[0].drawframe = state.player.drawframe;
        obj.entities[0].visualonground = state.player.visualonground;
        obj.entities[0].visualonroof = state.player.visualonroof;
        obj.entities[0].walkingframe = state.player.walkingframe;
        obj.entities[0].collisiondrawframe = state.player.collisiondrawframe;
        obj.entities[0].collisionframedelay = state.player.collisionframedelay;
        obj.entities[0].collisionwalkingframe = state.player.collisionwalkingframe;

        obj.entities[0].newxp = state.player.newxp;
        obj.entities[0].newyp = state.player.newyp;

        // TODO: not these Hacky fixes (restores player state in case they died last frame)
        obj.entities[0].invis = false;
        obj.entities[0].colour = 0;
        obj.entities[0].type = 0;
        game.hascontrol = true;

        // Load game data
        game.state = state.game.state;
        game.deathseq = state.game.deathseq;
        game.lifeseq = state.game.lifeseq;
        game.gravitycontrol = state.game.gravitycontrol;
        game.press_action = state.game.press_action;
        game.jumpheld = state.game.jumpheld;
        game.jumppressed = state.game.jumppressed;
        game.press_right = state.game.press_right;
        game.press_left = state.game.press_left;
        game.tapright = state.game.tapright;
        game.tapleft = state.game.tapleft;

        // Load cached entity and block data
        loadCacheEntry(solver, state.cache_entry);

        // Look through restored entities to find any trinkets that should be reactivated
        for (int e = 0; e < obj.entities.size(); e++) {
            if (obj.entities[e].rule == 3 && obj.entities[e].type == 7) {
                int trinket_idx = obj.entities[e].para;
                if (((state.collect >> trinket_idx) & 1) == 0 && obj.entities[e].size == -1) {
                    obj.entities[e].invis = false;
                    obj.entities[e].size = 0;
                }
            }
        }
    }
    static void loadCacheEntry(const SolverConfig& solver, const CacheEntry& entry) {
        if (entry.entity_hash != 0) {
            const std::vector<std::size_t>& cached_entities = solver.cache.getEntitySet(entry.entity_hash);
            
            // Load entity data
            for (int i = 0; i < cached_entities.size(); i++) {
                const EntityState& e = solver.cache.getEntity(cached_entities[i]);
                // obj.entities[0] is player, don't overwrite it
                obj.entities[i + 1].type = e.type;
                obj.entities[i + 1].rule = e.rule;
                obj.entities[i + 1].xp = e.x;
                obj.entities[i + 1].yp = e.y;
                obj.entities[i + 1].vx = e.vx;
                obj.entities[i + 1].vy = e.vy;
                // obj.entities[i + 1].ax = e.ax;
                // obj.entities[i + 1].ay = e.ay;
                obj.entities[i + 1].behave = e.behave;
                // obj.entities[i + 1].para = e.para;
                obj.entities[i + 1].state = e.state;
                obj.entities[i + 1].onwall = e.onwall;
                obj.entities[i + 1].statedelay = e.statedelay;

                obj.entities[i + 1].tile = e.tile;
                // obj.entities[i + 1].animate = e.animate;
                obj.entities[i + 1].framedelay = e.framedelay;
                obj.entities[i + 1].walkingframe = e.walkingframe;
                obj.entities[i + 1].drawframe = e.drawframe;

                obj.entities[i + 1].invis = e.invis;
                obj.entities[i + 1].life = e.life;
                obj.entities[i + 1].onentity = e.onentity;
            }
        }
        if (entry.block_hash != 0) {
            const std::vector<std::size_t>& cached_blocks = solver.cache.getBlockSet(entry.block_hash);
            // Load block data
            for (int i = 0; i < cached_blocks.size(); i++) {
                const BlockState& b = solver.cache.getBlock(cached_blocks[i]);
                obj.blocks[i].rect.x = b.rect_x;
                obj.blocks[i].rect.y = b.rect_y;
                obj.blocks[i].rect.w = b.rect_w;
                obj.blocks[i].rect.h = b.rect_h;
                obj.blocks[i].type = b.type;
                obj.blocks[i].trigger = b.trigger;
                obj.blocks[i].xp = b.rect_x;
                obj.blocks[i].yp = b.rect_y;
                // obj.blocks[i].wp = b.wp;
                // obj.blocks[i].hp = b.hp;
                // obj.blocks[i].script = b.script;
                // obj.blocks[i].prompt = b.prompt;
                // obj.blocks[i].r = b.r;
                // obj.blocks[i].g = b.g;
                // obj.blocks[i].b = b.b;
                // obj.blocks[i].activity_y = b.activity_y;
            }
        }
    }

    static void doGameStep() {
        {
            // graphics.renderfixedpost();
            // loop_end
            key.linealreadyemptykludge = false;
            // loop_begin
            // loop_assign_active_funcs
            // loop_run_active_funcs
            {
                // gameinput
                if (key.actually_poll) {
                    key.Poll();
                }
                gameinput();
                // gamelogic
                gamelogic();
                // focused_end
                // game.gameclock();
                // music.processmusic();
                graphics.processfade();
                // focused_begin
                map.nexttowercolour_set = false;
                // run_script
                if (!script.running) {
                    script.run();
                }
                // gamerenderfixed
                gamerenderfixed();
                // graphics.renderfixedpre
                // graphics.renderfixedpre();
            }
        }
        // Skip rendering to improve runtime
    }

    static void render(const SolverConfig& solver) {
        graphics.clear();
        graphics.set_render_target(graphics.gameTexture);
        gamerender();

        // Clear previous HUD text so we can just display debug info
        graphics.set_render_target(graphics.gameTexture);
        graphics.clear();
        graphics.copy_texture(graphics.gameplayTexture, NULL, NULL);

        renderHUD(solver);

        graphics.render();

        gameScreen.RenderPresent();
    }

    static void renderHUD(const SolverConfig& solver) {
        const char* tempstring; int label_len;
        char buffer[SCREEN_WIDTH_CHARS + 1];

        if (solver.solution_info.length > 0) {
            // We are displaying a solution, render that info
            const SolutionInfo& info = solver.solution_info;

            { // Format sol string
                vformat_buf(
                    buffer, sizeof(buffer),
                    "{d} / {h}", "d:int, h:int",
                    (info.solution_idx + 1), info.num_solutions
                );
            }
            tempstring = "SOLVE:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 6, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 6, buffer, 196, 196, 196);

            { // Format frame string
                vformat_buf(
                    buffer, sizeof(buffer),
                    "{d} / {h}", "d:int, h:int",
                    info.frame, info.length
                );
            }
            tempstring = "FRAME:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 18, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 18, buffer, 196, 196, 196);

            if (solver.mode == MIN_INPUT_CHANGES || solver.mode == MIN_INPUT_FRAMES || solver.mode == MIN_FLIPS) {
                { // Format value string
                    vformat_buf(
                        buffer, sizeof(buffer),
                        "{d} / {h}", "d:int, h:int",
                        info.measure, info.solution_measure
                    );
                }
                tempstring = "VALUE:";
                label_len = font::len(0, tempstring);
                font::print(PR_BOR, 6, 30, tempstring, 255, 255, 255);
                font::print(PR_BOR, 8 + label_len, 30, buffer, 196, 196, 196);
            }

            bool left = key.isDown(KEYBOARD_LEFT);
            bool right = key.isDown(KEYBOARD_RIGHT);
            bool flip = key.isDown(KEYBOARD_v);
            tempstring = "INPUT:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 42, tempstring, 255, 255, 255);
            int c = left ? 196 : 96;
            font::print(PR_BOR, 8 + label_len, 42, "L", c, c, c);
            c = flip ? 196 : 96;
            font::print(PR_BOR, 8 + label_len + 16, 42, "V", c, c, c);
            c = right ? 196 : 96;
            font::print(PR_BOR, 8 + label_len + 32, 42, "R", c, c, c);
        }
        else if (solver.debug_info.queue_size > 0) {
            // We are currently solving the scenario, display debug info
            const DebugInfo& info = solver.debug_info;

            { // Format time string
                int millis = solver.debug_info.millis % 1000;
                int seconds = solver.debug_info.millis / 1000;
                int s = seconds % 60;
                int m = (seconds / 60) % 60;
                int h = seconds / 3600;

                if (h > 0) {
                    vformat_buf(buffer, sizeof(buffer),
                        "{hrs}:{min|digits=2}:{sec|digits=2}.{mil|digits=3}",
                        "hrs:int, min:int, sec:int, mil:int",
                        h, m, s, millis
                    );
                }
                else if (m > 0) {
                    vformat_buf(buffer, sizeof(buffer),
                        "{min}:{sec|digits=2}.{mil|digits=3}",
                        "min:int, sec:int, mil:int",
                        m, s, millis
                    );
                }
                else {
                    vformat_buf(buffer, sizeof(buffer),
                        "{sec}.{mil|digits=3}",
                        "sec:int, mil:int",
                        s, millis
                    );
                }
            }

            tempstring = "TIME: ";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 6, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len,  6, buffer, 196, 196, 196);

            { // Format depth string
                vformat_buf(
                    buffer, sizeof(buffer),
                    "{d} / {h}", "d:int, h:int",
                    info.max_measure, info.min_heuristic
                );
            }
            tempstring = "DEPTH:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 18, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 18, buffer, 196, 196, 196);

            tempstring = "STATE:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 30, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 30, help.String(info.states), 196, 196, 196);

            tempstring = "QUEUE:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 42, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 42, help.String(info.queue_size), 196, 196, 196);

            tempstring = "CACHE:";
            label_len = font::len(0, tempstring);
            font::print(PR_BOR, 6, 54, tempstring, 255, 255, 255);
            font::print(PR_BOR, 8 + label_len, 54, help.String(info.cache_size), 196, 196, 196);
        }
    }

    /// Returns true if state `a` should come *after* state `b` in processing order
    bool SolverConfig::compareStates(const CachedSolverState& a, const CachedSolverState& b) const {
        // The heuristic must always have the highest priority in state comparisons
        if (a.heuristic != b.heuristic) {
            // Prioritize low heuristic -> fastest solution will be found first if heuristic is admissible
            return a.heuristic > b.heuristic;
        }

        if (minimize_frames && (a.frame_count != b.frame_count)) {
            // Prioritize low frame count
            return a.frame_count > b.frame_count;
        }

        uint16_t a_measure = a.getMeasure(mode);
        uint16_t b_measure = b.getMeasure(mode);
        if (a_measure != b_measure) {
            // Prioritize high depth - we always continue processing the deepest branch (i.e. DFS)
            return a_measure < b_measure;
        }

        if (a.num_l_plus_r != b.num_l_plus_r) {
            // Prioritize low L+R input count, we only want it to occur if absolutely necessary
            return a.num_l_plus_r > b.num_l_plus_r;
        }

        if (clean_inputs && a.flip_count != b.flip_count) {
            // Prioritize low L+R input count, we only want it to occur if absolutely necessary
            return a.flip_count > b.flip_count;
        }
        if (clean_inputs && (a.input_change_count != b.input_change_count)) {
            // Prioritize low changes in inputs, so the resulting solution is more human-viable
            return a.input_change_count > b.input_change_count;
        }
        if (clean_inputs && (a.input_frame_count != b.input_frame_count)) {
            // Prioritize input frame count, so doing nothing is better than constantly moving
            return a.input_frame_count > b.input_frame_count;
        }

        return true;
    }

    bool CheckedCorner::isPosAfter(const Terrain::GlobalPosition& pos) const {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "after" the corner means being sure that
        // we have already passed it. For trinkets and warp tokens, we can't
        // "really" know just based off the position, except if we're touching it
        if (this->isTrinketOrWarp()) {
            int rx_delta = (pos.room.rx - room.rx);
            int ry_delta = (pos.room.ry - room.ry);
            if (rx_delta > 10) {
                rx_delta -= 20;
            }
            else if (rx_delta < -10) {
                rx_delta += 20;
            }
            if (ry_delta > 10) {
                ry_delta -= 20;
            }
            else if (ry_delta < -10) {
                ry_delta += 20;
            }
            int glob_x = ROOM_W * rx_delta + pos.pos.x;
            int glob_y = ROOM_H * ry_delta + pos.pos.y;
            return region.contains(IntVector(glob_x, glob_y));
        }
        else if (this->isRegular()) {
            int rx_delta = (pos.room.rx - room.rx);
            int ry_delta = (pos.room.ry - room.ry);
            if (rx_delta > 10) {
                rx_delta -= 20;
            }
            else if (rx_delta < -10) {
                rx_delta += 20;
            }
            if (ry_delta > 10) {
                ry_delta -= 20;
            }
            else if (ry_delta < -10) {
                ry_delta += 20;
            }
            int x_delta = ROOM_W * rx_delta + pos.pos.x;
            int y_delta = ROOM_H * ry_delta + pos.pos.y;
            IntVector delta(x_delta, y_delta);
            Region region = this->region;
            switch (this->dir) {
            case UP_LEFT:
            case UP_RIGHT:
                // Keep X range the same, modify Y range
                region.y = region.y.getIntervalBelow();
                break;
            case DOWN_LEFT:
            case DOWN_RIGHT:
                // Keep X range the same, modify Y range
                region.y = region.y.getIntervalAbove();
                break;
            case LEFT_UP:
            case LEFT_DOWN:
                // Keep Y range the same, modify X range
                region.x = region.x.getIntervalBelow();
                break;
            case RIGHT_UP:
            case RIGHT_DOWN:
                // Keep Y range the same, modify X range
                region.x = region.x.getIntervalAbove();
                break;
            case TRINKET:
            case WARP_TOKEN:
                // Do nothing
                break;
            default:
                Exceptions::unreachable();
            }
            return region.contains(delta);
        }

        Exceptions::todo();
    }
    bool CheckedCorner::isPosBefore(const Terrain::GlobalPosition& pos) const {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "before" the corner means being sure that
        // we still need to pass it. For trinkets and warp tokens, we can't
        // "really" know just based off the position, so always return false 
        if (this->isTrinketOrWarp()) {
            return false;
        }
        else if (this->isRegular()) {
            int rx_delta = (pos.room.rx - room.rx);
            int ry_delta = (pos.room.ry - room.ry);
            if (rx_delta > 10) {
                rx_delta -= 20;
            }
            else if (rx_delta < -10) {
                rx_delta += 20;
            }
            if (ry_delta > 10) {
                ry_delta -= 20;
            }
            else if (ry_delta < -10) {
                ry_delta += 20;
            }
            IntVector c_pos = this->getRegularPos();
            int x_delta = ROOM_W * rx_delta + pos.pos.x - c_pos.x;
            int y_delta = ROOM_H * ry_delta + pos.pos.y - c_pos.y;
            switch (this->dir) {
            case UP_LEFT:
                return x_delta > 0 && y_delta >= 0;
            case UP_RIGHT:
                return x_delta < 0 && y_delta >= 0;
            case DOWN_LEFT:
                return x_delta > 0 && y_delta <= 0;
            case DOWN_RIGHT:
                return x_delta < 0 && y_delta <= 0;
            case LEFT_UP:
                return x_delta >= 0 && y_delta > 0;
            case LEFT_DOWN:
                return x_delta >= 0 && y_delta < 0;
            case RIGHT_UP:
                return x_delta <= 0 && y_delta > 0;
            case RIGHT_DOWN:
                return x_delta <= 0 && y_delta < 0;
            default:
                Exceptions::unreachable();
            }
        }

        Exceptions::todo();
    }
    bool CheckedCorner::isPosInside(const Terrain::GlobalPosition& pos) const {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "inside" the corner means being out of bounds
        // So, touching a trinket or warp token doesn't count as being "inside"
        if (this->isTrinketOrWarp()) {
            return false;
        } else if (this->isRegular()) {
            int rx_delta = (pos.room.rx - room.rx);
            int ry_delta = (pos.room.ry - room.ry);
            if (rx_delta > 10) {
                rx_delta -= 20;
            }
            else if (rx_delta < -10) {
                rx_delta += 20;
            }
            if (ry_delta > 10) {
                ry_delta -= 20;
            }
            else if (ry_delta < -10) {
                ry_delta += 20;
            }
            IntVector c_pos = this->getRegularPos();
            int x_delta = ROOM_W * rx_delta + pos.pos.x - c_pos.x;
            int y_delta = ROOM_H * ry_delta + pos.pos.y - c_pos.y;
            switch (this->getType()) {
                case Terrain::CornerType::BottomRight:
                    return x_delta < 0 && y_delta < 0;
                case Terrain::CornerType::BottomLeft:
                    return x_delta > 0 && y_delta < 0;
                case Terrain::CornerType::TopRight:
                    return x_delta < 0 && y_delta > 0;
                case Terrain::CornerType::TopLeft:
                    return x_delta > 0 && y_delta > 0;
                default:
                    Exceptions::unreachable();
            }
        }

        Exceptions::todo();
    }
    bool CheckedCorner::isPosNotStrictlyBefore(const Terrain::GlobalPosition& pos) const {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "before" the corner means being sure that we
        // still need to pass it. For trinkets and warp tokens, we can't "really"
        // know just based off the position, so only return true if we are touching
        if (this->isTrinketOrWarp()) {
            return isPosAfter(pos);
        }
        else if (this->isRegular()) {
            int rx_delta = (pos.room.rx - room.rx);
            int ry_delta = (pos.room.ry - room.ry);
            if (rx_delta > 10) {
                rx_delta -= 20;
            }
            else if (rx_delta < -10) {
                rx_delta += 20;
            }
            if (ry_delta > 10) {
                ry_delta -= 20;
            }
            else if (ry_delta < -10) {
                ry_delta += 20;
            }
            IntVector c_pos = this->getRegularPos();
            int x_delta = ROOM_W * rx_delta + pos.pos.x - c_pos.x;
            int y_delta = ROOM_H * ry_delta + pos.pos.y - c_pos.y;
            switch (this->dir) {
            case UP_LEFT:
            case DOWN_LEFT:
                return x_delta <= 0;
            case UP_RIGHT:
            case DOWN_RIGHT:
                return x_delta >= 0;
            case LEFT_UP:
            case RIGHT_UP:
                return y_delta <= 0;
            case LEFT_DOWN:
            case RIGHT_DOWN:
                return y_delta >= 0;
            default:
                Exceptions::unreachable();
            }
        }

        Exceptions::todo();
    }

    Region CheckedCorner::getUnboundedRegion() const {
        // Simply remove the "outside" bounds on regular corner regions
        Region region = Region(this->region);
        if (this->isRegular()) {
            switch (this->getType()) {
            case Terrain::CornerType::BottomLeft:
                region.removeXLowerBound();
                region.removeYUpperBound();
                break;
            case Terrain::CornerType::BottomRight:
                region.removeXUpperBound();
                region.removeYUpperBound();
                break;
            case Terrain::CornerType::TopLeft:
                region.removeXLowerBound();
                region.removeYLowerBound();
                break;
            case Terrain::CornerType::TopRight:
                region.removeXUpperBound();
                region.removeYLowerBound();
                break;
            default:
                Exceptions::unreachable();
            }
        }

        return region;
    }
    Region CheckedCorner::getRegionAfter() const {
        // In this context, being "after" the corner means being sure that
        // we have already passed it. For trinkets and warp tokens, we can't
        // "really" know just based off the position, except if we're touching it
        Region region = this->region;
        switch (this->dir) {
        case UP_LEFT:
        case UP_RIGHT:
            // Keep X range the same, modify Y range
            region.y.invert();
            break;
        case DOWN_LEFT:
        case DOWN_RIGHT:
            // Keep X range the same, modify Y range
            region.y.invert();
            break;
        case LEFT_UP:
        case LEFT_DOWN:
            // Keep Y range the same, modify X range
            region.x.invert();
            break;
        case RIGHT_UP:
        case RIGHT_DOWN:
            // Keep Y range the same, modify X range
            region.x.invert();
            break;
        case TRINKET:
        case WARP_TOKEN:
            // Do nothing
            break;
        default:
            Exceptions::unreachable();
        }

        return region;
    }
    Region CheckedCorner::getRegionBefore() const {
        // In this context, being "before" the corner means being sure that
        // we still need to pass it. For trinkets and warp tokens, we can't
        // "really" know just based off the position, so always return false
        Region region = this->getUnboundedRegion();
        switch (this->dir) {
        case TRINKET:
        case WARP_TOKEN:
            region.make_bottom();
            break;
        case UP_LEFT:
        case UP_RIGHT:
        case DOWN_LEFT:
        case DOWN_RIGHT:
            // Keep Y range the same, invert X range
            region.x.invert();
            break;
        case LEFT_UP:
        case LEFT_DOWN:
        case RIGHT_UP:
        case RIGHT_DOWN:
            // Keep X range the same, invert Y range
            region.y.invert();
            break;
        default:
            Exceptions::unreachable();
        }

        return region;
    }
    Region CheckedCorner::getRegionInside() const {
        // In this context, being "inside" the corner means being out of bounds
        // So, touching a trinket or warp token doesn't count as being "inside"
        Region region = this->getUnboundedRegion();
        switch (this->dir) {
        case TRINKET:
        case WARP_TOKEN:
            region.make_bottom();
            break;
        case UP_LEFT:
        case UP_RIGHT:
        case DOWN_LEFT:
        case DOWN_RIGHT:
        case LEFT_UP:
        case LEFT_DOWN:
        case RIGHT_UP:
        case RIGHT_DOWN:
            // Invert both x and y ranges
            region.y.invert();
            break;
        default:
            Exceptions::unreachable();
        }

        return region;
    }
}
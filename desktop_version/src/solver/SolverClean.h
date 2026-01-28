#ifndef SOLVER_CLEAN_H
#define SOLVER_CLEAN_H

#include "solver/Exceptions.h"
#include "solver/Geometry.h"
#include "solver/Terrain.h"

#include <SDL.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

using namespace Geometry;

namespace Solver {

    // Hash functions for various data types
    std::hash<bool> h_b;
    std::hash<int> h_i;
    std::hash<uint64_t> h_u;
    std::hash<float> h_f;
    uint64_t combineHashes(uint64_t h1, uint64_t h2) {
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }

    enum SolverMode {
        /// Simple heuristic that uses Manhattan distance and assumes max speed at all times
        SIMPLE,
        /// More complicated sim-based heuristic, factors in acceleration and velocity changes
        ACCEL_BASED,
        MIN_INPUT_CHANGES,
        MIN_INPUT_FRAMES,
        MIN_FLIPS,
        // TODO: more modes, e.g. human viability?
    };

    /// This struct contains the debug information that's displayed on the HUD during solving
    struct DebugInfo {
        /// The number of states explored so far
        uint64_t states;
        /// The current minimum heuristic, i.e. a lower bound for the measure of the optimal solution
        uint16_t min_heuristic;
        /// The maximum measure of any state so far, i.e. an indication of how close to the lower bound we are.
        uint16_t max_measure;
        /// The size of the state queue
        uint64_t queue_size;
        /// The size of the entity cache
        uint64_t cache_size;
    };

    /// This struct contains the information that's displayed on the HUD when displaying a solution
    struct SolutionInfo {
        /// The index of the frame currently being displayed
        uint16_t frame;
        /// The inputs of the current frame, where the bits in low-to-high order are Left, Right, Flip, (R, Enter)
        uint8_t input;
        /// The value being optimized for, e.g. number of frames or number of inputs
        uint16_t measure;
    };

    struct CheckedCorner {
        Terrain::RoomPosition room;
        Region region;
        CornerDir dir;

        CheckedCorner(Terrain::RoomPosition room, Region region, CornerDir dir) : room(room), region(region), dir(dir) {}
    
        bool isRegular() {
            switch (dir) {
            case UP_LEFT:
            case UP_RIGHT:
            case DOWN_LEFT:
            case DOWN_RIGHT:
            case LEFT_UP:
            case LEFT_DOWN:
            case RIGHT_UP:
            case RIGHT_DOWN:
                return true;
            default:
                return false;
            }
        }
        bool isTrinketOrWarp() {
            switch (dir) {
            case TRINKET:
            case WARP_TOKEN:
                return true;
            default:
                return false;
            }
        }
        Terrain::CornerType getType() {
            switch (dir) {
            case UP_LEFT:
            case RIGHT_DOWN:
                return Terrain::CornerType::BottomLeft;
            case UP_RIGHT:
            case LEFT_DOWN:
                return Terrain::CornerType::BottomRight;
            case DOWN_LEFT:
            case RIGHT_UP:
                return Terrain::CornerType::TopLeft;
            case DOWN_RIGHT:
            case LEFT_UP:
                return Terrain::CornerType::TopRight;
            }
            Exceptions::invalid_argument();
        }
        IntVector getRegularPos() {
            switch (this->getType()) {
            case Terrain::CornerType::BottomRight:
                return region.getMin();
            case Terrain::CornerType::BottomLeft:
                return IntVector(region.x.max, region.y.min);
            case Terrain::CornerType::TopRight:
                return IntVector(region.x.min, region.y.max);
            case Terrain::CornerType::TopLeft:
                return region.getMax();
            default:
                Exceptions::unreachable();
            }
        }

        bool isPosAfter(Terrain::GlobalPosition& pos);
        bool isPosBefore(Terrain::GlobalPosition& pos);
        bool isPosInside(Terrain::GlobalPosition& pos);
    };

    struct CheckedScenario {
        Terrain::RoomPosition init_room;
        IntVector init_pos;
        bool inv_gravity;
        int advance_frames;
        std::vector<CheckedCorner> corners;

        CheckedScenario(Terrain::RoomPosition init_room, IntVector init_pos, bool inv_gravity, int advance_frames, std::vector<CheckedCorner> corners) : init_room(init_room), init_pos(init_pos), inv_gravity(inv_gravity), advance_frames(advance_frames), corners(corners) {}
    };

    // TODO: does this belong in scenario or in solver config?
    enum PlayerTileMode {
        REGULAR,
        SAD,
        TERMINAL,
        PACMAN,
    };

    /// -------------------------------
    /// State structs
    /// -------------------------------
    /// 
    /// Note: the sizes of these structs are *critical* to minimize memory usage!
    /// Note: Effective min (or max) means any value less (or greater) than the value is equivalent to it

    struct GameState {
        /// 0 <= state <= 4099
        int16_t state;
        /// -1 <= deathseq <= 30
        int8_t deathseq;
        /// 0 <= lifeseq <= 10
        int8_t lifeseq;

        /// 0 <= gravitycontrol <= 1
        bool gravitycontrol;
        /// 0 <= roomx <= 120 (although if we correct for dimension offset it's just 0 to 20)
        int8_t roomx;
        /// 0 <= roomy <= 120 (although if we correct for dimension offset it's just 0 to 20)
        int8_t roomy;

        /// 0 <= press_action <= 1
        bool press_action;
        /// 0 <= jumppressed <= 5
        int8_t jumppressed;
        /// 0 <= jumpheld <= 1
        bool jumpheld;

        /// 0 <= press_right <= 1
        bool press_right;
        /// 0 <= press_left <= 1
        bool press_left;
        /// 0 <= tapright <= INF (effective max is 5)
        int8_t tapright;
        /// 0 <= tapleft <= INF (effective max is 5)
        int8_t tapleft;

        uint64_t hash() {
            // TODO: these values can be packed to minimize hash calls
            uint64_t result = 0;

            result = combine_hashes(result, h_i(state));
            result = combine_hashes(result, h_i(deathseq));
            result = combine_hashes(result, h_i(lifeseq));
            result = combine_hashes(result, h_b(gravitycontrol));
            result = combine_hashes(result, h_i(roomx));
            result = combine_hashes(result, h_i(roomy));
            result = combine_hashes(result, h_b(press_action));
            result = combine_hashes(result, h_i(jumppressed));
            result = combine_hashes(result, h_b(jumpheld));
            result = combine_hashes(result, h_b(press_right));
            result = combine_hashes(result, h_b(press_left));

            // Any value greater than 5 is equivalent to 5
            int effective_tapright = SDL_min(tapright, 5);
            int effective_tapleft = SDL_min(tapleft, 5);
            result = combine_hashes(result, h_i(effective_tapright));
            result = combine_hashes(result, h_i(effective_tapleft));

            return result;
        }
    };

    struct PlayerState {
        /// -20 <= x <= 320
        int16_t x;
        /// -20 <= y <= 240
        int16_t y;

        // TODO: these floats can be packed better
        /// -6 <= vx <= 6, usually quantized to 0.1 (except for rounding errors)
        float vx;
        /// -10 <= vy <= 10, usually quantized to 0.1 (except for rounding errors)
        float vy;
        /// ax in { -6, -3, 0, 3, 6 }
        int8_t ax;
        /// ay in { -6, -3, 0, 3 }
        int8_t ay;

        /// -INF <= onground <= 2 (effective min is 0)
        int8_t onground;
        /// -INF <= onroof <= 2 (effective min is 0)
        int8_t onroof;
        /// 0 <= dir <= 1
        bool dir;

        /// tile in { 0, 6, 144, 150 } (144 = sad, 6 = terminal, 150 = pacman)
        uint8_t tile;
        /// -INF <= framedelay <= 4 (effective min is 1)
        int8_t framedelay;
        /// drawframe in the interval [0, 17] or the interval [144, 161] (basically [tile, tile+11])
        uint8_t drawframe;
        /// 0 <= walkingframe <= 1
        bool walkingframe;

        /// -INF <= visualonground <= 2 (effective min is 0)
        int8_t visualonground;
        /// -INF <= visualonroof <= 2 (effective min is 0)
        int8_t visualonroof;

        /// collisiondrawframe in [0, 17] or [144, 161] (basically [tile, tile+11])
        uint8_t collisiondrawframe;
        /// -INF <= collisionframedelay <= 4  (effective min is 1)
        int8_t collisionframedelay;
        /// 0 <= collisionwalkingframe <= 1
        bool collisionwalkingframe;

        // TODO: these floats can be packed better
        /// newxp in { 0, 152, `x`, `x + vx` }
        float newxp;
        /// newyp in { 0, `y`, `y + vy` }
        float newyp;

        // TODO: add these variables maybe?
        // int oldxp, oldyp;    // relevant for gravity lines (when?)
        // int size;            // size in { 0, 13 } (13 = big viridian)
        // int cx, cy, w, h;    // relevant for big viridian / gravitron zipping
        // TODO: is it possible to be big viridian with a different sprite? i.e. pacman or terminal

        uint64_t hash() {
            // TODO: these values can be packed to minimize hash calls
            uint64_t result = h_i(x);
            result = combine_hashes(result, h_i(y));

            result = combine_hashes(result, h_f(vx));
            result = combine_hashes(result, h_f(vy));

            // Should always be integers
            result = combine_hashes(result, h_i((int) ax));
            result = combine_hashes(result, h_i((int) ay));

            // Any value less than 0 is equivalent to 0
            int effective_onground = SDL_max(onground, 0);
            int effective_onroof = SDL_max(onroof, 0);
            result = combine_hashes(result, h_i(effective_onground));
            result = combine_hashes(result, h_i(effective_onroof));

            result = combine_hashes(result, h_b(dir));

            result = combine_hashes(result, h_i(tile));

            // Any value less than 0 is equivalent to 0
            int effective_framedelay = SDL_max(framedelay, 0);
            result = combine_hashes(result, h_i(effective_framedelay));
            result = combine_hashes(result, h_i(drawframe));

            // Any value less than 0 is equivalent to 0
            int effective_visualonground = SDL_max(visualonground, 0);
            int effective_visualonroof = SDL_max(visualonroof, 0);
            result = combine_hashes(result, h_i(effective_visualonground));
            result = combine_hashes(result, h_i(effective_visualonroof));

            result = combine_hashes(result, h_i(walkingframe));
            result = combine_hashes(result, h_i(collisiondrawframe));

            // Any value less than 0 is equivalent to 0
            int effective_collisionframedelay = SDL_max(collisionframedelay, 0);
            result = combine_hashes(result, h_i(effective_collisionframedelay));
            result = combine_hashes(result, h_b(collisionwalkingframe));

            result = combine_hashes(result, h_f(newxp));
            result = combine_hashes(result, h_f(newyp));

            return result;
        }

        bool isAfter(CheckedCorner& corner);
        bool isBefore(CheckedCorner& corner);
        bool isInside(CheckedCorner& corner);
    };

    struct EntityState {
        /// -1 <= type <= 100
        int8_t type;
        /// -1 <= rule <= 7
        int8_t rule;
        /// -20 <= x <= 320
        int16_t x;
        /// -20 <= y <= 240
        int16_t y;

        // TODO: these can be packed better
        // TODO: what are the ranges for these?
        float vx;
        float vy;
        // TODO: ax and ay are needed for crew members (e.g. im1)
        // float ax;               // ax in { -6, -3, 0, 3, 6 }
        // float ay;               // ay in { -6, -3, 0, 3 }

        /// -1 <= behave <= 18
        int8_t behave;
        // TODO: entity type 12 updates para (but is that even necessary anywhere?)
        // float para;             // -1 <= para <= 48 (505167 for checkpoints)
        /// 0 <= state <= 4
        int8_t state;
        /// 0 <= onwall <= 3
        int8_t onwall;
        /// 0 <= statedelay <= 120
        int8_t statedelay;

        /// 0 <= tile <= 1115
        int16_t tile;
        // 0 <= animate <= 100
        // int8_t animate;
        /// 0 <= framedelay <= 10
        int8_t framedelay;
        /// -5 <= walkingframe <= 5
        int8_t walkingframe;
        /// -10 <= drawframe <= 721
        int16_t drawframe;

        // TODO: do we need actionframe? only matters for animate == 0, not sure if any (relephant) entities have that property
        // TODO: newxp and newyp necessary?

        // For disappearing platforms and gravity lines:
        /// 0 <= invis <= 1
        bool invis;
        /// 0 <= life <= 12
        int8_t life;
        /// 0 <= onentity <= 3
        int8_t onentity;

        uint64_t hash() {
            uint64_t packed_value = 0;
            packed_value = (packed_value << 7) | (type + 1);
            packed_value = (packed_value << 4) | (rule + 1);

            uint64_t e_hash = h_i(x);
            e_hash = combine_hashes(e_hash, h_i(y));
            e_hash = combine_hashes(e_hash, h_f(vy));
            e_hash = combine_hashes(e_hash, h_f(vx));

            packed_value = (packed_value << 5) | (behave + 1);
            packed_value = (packed_value << 3) | state;
            packed_value = (packed_value << 2) | onwall;

            // Any value less than 0 is equivalent to 0
            int effective_statedelay = SDL_max(statedelay, 0);
            packed_value = (packed_value << 7) | effective_statedelay;

            packed_value = (packed_value << 11) | tile;

            // Any value less than 0 is equivalent to 0
            int effective_framedelay = SDL_max(framedelay, 0);
            packed_value = (packed_value << 4) | effective_framedelay;
            packed_value = (packed_value << 4) | (walkingframe + 5);
            packed_value = (packed_value << 10) | (drawframe + 10);

            packed_value = (packed_value << 1) | invis;
            packed_value = (packed_value << 4) | life;
            packed_value = (packed_value << 2) | invis;

            e_hash = combine_hashes(e_hash, h_u(packed_value));

            return e_hash;
        }
        static uint64_t hash(std::vector<EntityState> es) {
            uint64_t result = h_u(es.size());

            for (int i = 0; i < es.size(); i++) {
                if (es[i].rule == 3 && es[i].type == 13) {
                    // Hack: Ignore terminals
                    // TODO: do we care about final level terminal? probably not
                    continue;
                }
                if (es[i].rule == 3 && es[i].type == 8) {
                    // Hack: Ignore checkpoints
                    // TODO what about death strats?
                    continue;
                }
                if (es[i].rule == 3 && es[i].type == 7) {
                    // Hack: Ignore trinkets, they should be restored by setting obj.collect
                    continue;
                }
                result = combine_hashes(result, es[i].hash());
            }

            return result;
        }
    };

    struct BlockState {
        int16_t rect_x;
        int16_t rect_y;
        int16_t rect_w;
        int16_t rect_h;
        /// 0 <= type <= 5
        int8_t type;
        /// 0 <= trigger <= 3500
        int16_t trigger;
        // int xp, yp, wp, hp;
        // std::string script, prompt;
        // int r, g, b;
        // int activity_y;

        uint64_t hash() {
            uint64_t packed_value = rect_x + 100;
            packed_value = (packed_value << 10) | (rect_y + 100);
            packed_value = (packed_value << 9) | rect_w;
            packed_value = (packed_value << 9) | rect_h;
            packed_value = (packed_value << 3) | type;
            packed_value = (packed_value << 12) | trigger;
            return h_u(packed_value);
        }
        static uint64_t hash(std::vector<BlockState> blocks) {
            uint64_t result = h_u(blocks.size());

            for (int i = 0; i < blocks.size(); i++) {
                if (blocks[i].type == BLOCK || blocks[i].type == TRIGGER || blocks[i].type == ACTIVITY) {
                    result = combine_hashes(result, blocks[i].hash());
                }
            }

            return result;
        }
    };

    struct CacheEntry {
        uint64_t entity_hash;
        uint64_t block_hash;

        CacheEntry() {}
        CacheEntry(uint64_t entity_hash, uint64_t block_hash) : entity_hash(entity_hash), block_hash(block_hash) {}

        uint64_t hash() {
            return combine_hashes(entity_hash, block_hash);
        }
    };

    /// Once a state has been processed, this is all the information we need to reconstruct it when a solution is found
    struct StateSummary {
        uint64_t prev_hash;
        int8_t inputs;

        StateSummary(uint64_t prev_hash, int8_t inputs) : prev_hash(prev_hash), inputs(inputs) {}
    };

    struct CachedSolverState {
        PlayerState player;
        CacheEntry cache_entry;
        GameState game;

        /// Contains one bit for each trinket
        /// 0 <= collect < 2^21
        int32_t collect;

        /// A lower bound for the cost of a solution starting from this state
        uint16_t heuristic;
        /// The index of the first not yet reached corner in the current scenario
        uint8_t next_corner;

        /// The number of elapsed frames total
        uint16_t frame_count;
        /// The sum of the number of frames pressed for each button
        uint16_t input_frame_count;
        /// The number of times an input changed from not pressed to pressed (et vice versa)
        uint16_t input_change_count;
        /// The number of flips so far
        uint16_t flip_count;

        // The number of elapsed frames since the last room transition
        // Could be used instead of entity cache in some cases, especially for 
        // uint16_t num_frames_in_room;
        /// The number of frames that left and right were pressed simultaneously (so we can minimize it)
        uint8_t num_l_plus_r;

        /// The inputs used to reach this state
        int8_t inputs;
        /// The hash of the state prior to this one
        uint64_t prevHash;

        /// Returns the measure associated with the given solver mode (e.g. frame count for SIMPLE)
        uint16_t getMeasure(SolverMode mode) {
            switch (mode) {
            case SolverMode::SIMPLE:
            case SolverMode::ACCEL_BASED:
                return frame_count;
            case SolverMode::MIN_INPUT_CHANGES:
                return input_change_count;
            case SolverMode::MIN_INPUT_FRAMES:
                return input_frame_count;
            case SolverMode::MIN_FLIPS:
                return flip_count;
            default:
                Exceptions::todo();
                break;
            }
        }

        /// Returns the summary of this state, i.e. enough information to reconstruct it later
        StateSummary summary() {
            return StateSummary(prevHash, inputs);
        }

        uint64_t hash() {
            uint64_t result = player.hash();
            result = combine_hashes(result, game.hash());
            result = combine_hashes(result, cache_entry.hash());
            result = combine_hashes(result, h_i(collect));

            // TODO: should we hash next_corner too?

            return result;
        }
    };

    struct CustomHash {
        /* TODO diagnostics to see if this is actually faster
        static std::size_t splitmix64(std::size_t x)
        {

            // 0x9e3779b97f4a7c15,
            // 0xbf58476d1ce4e5b9,
            // 0x94d049bb133111eb are numbers
            // that are obtained by dividing
            // high powers of two with Phi
            // (1.6180..) In this way the
            // value of x is modified
            // to evenly distribute
            // keys in hash table
            x += 0x9e3779b97f4a7c15;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
            x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
            return x ^ (x >> 31);
        }
        */

        std::size_t operator()(std::size_t x) const {
            return x;
        }
    };
    struct EntityCache {
        std::unordered_map<uint64_t, EntityState, CustomHash> entities;
        std::unordered_map<uint64_t, BlockState, CustomHash> blocks;
        std::unordered_map<uint64_t, std::vector<uint64_t>, CustomHash> entity_sets;
        std::unordered_map<uint64_t, std::vector<uint64_t>, CustomHash> block_sets;

        uint64_t cacheEntity(EntityState& entity) {
            uint64_t hash = entity.hash();
            if (entities.find(hash) == entities.end()) {
                entities.emplace(hash, entity);
            }
            return hash;
        }
        EntityState& getEntity(uint64_t hash) {
            Exceptions::assert(entities.find(hash) != entities.end());
            return entities.at(hash);
        }
        uint64_t cacheEntitySet(std::vector<uint64_t> entity_hashes) {
            uint64_t set_hash = h_u(entity_hashes.size());
            for (uint64_t h : entity_hashes) {
                set_hash = combine_hashes(set_hash, h);
            }

            if (entity_sets.find(set_hash) == entity_sets.end()) {
                entity_sets.emplace(set_hash, entity_hashes);
            }
            return set_hash;
        }
        std::vector<uint64_t>& getEntitySet(uint64_t set_hash) {
            Exceptions::assert(entity_sets.find(set_hash) != entity_sets.end());
            return entity_sets.at(set_hash);
        }

        uint64_t cacheBlock(BlockState& block) {
            uint64_t hash = block.hash();
            if (blocks.find(hash) == blocks.end()) {
                blocks.emplace(hash, block);
            }
            return hash;
        }
        BlockState& getBlock(uint64_t hash) {
            Exceptions::assert(blocks.find(hash) != blocks.end());
            return blocks.at(hash);
        }
        uint64_t cacheBlockSet(std::vector<uint64_t> block_hashes) {
            uint64_t set_hash = h_u(block_hashes.size());
            for (uint64_t h : block_hashes) {
                set_hash = combine_hashes(set_hash, h);
            }

            if (block_sets.find(set_hash) == block_sets.end()) {
                block_sets.emplace(set_hash, block_hashes);
            }
            return set_hash;
        }
        std::vector<uint64_t>& getBlockSet(uint64_t set_hash) {
            Exceptions::assert(block_sets.find(set_hash) != block_sets.end());
            return block_sets.at(set_hash);
        }
    };

    struct SolverConfig {
        /// Determines the cost function to be optimized for
        SolverMode mode;
        /// Forces the solver to use minimum frames as a secondary heuristic, so solutions are also time-optimal.
        bool minimize_frames;
        /// Forces the solver to use minimum input changes as a tertiary heuristic, so solution inputs are more human-viable.
        bool clean_inputs;
        /// When set, this flag tells the solver to do some extra sanity / consistency checks to aid with debugging.
        bool debug_checks;

        /// The entity cache is designed to reduce memory usage by only storing unique entity configurations
        EntityCache cache;
        /// These are simply used for rendering data on the HUD
        DebugInfo debug_info;
        SolutionInfo solution_info;

        SolverConfig() : mode(SolverMode::SIMPLE), minimize_frames(false), debug_checks(false) {}
        SolverConfig(SolverMode mode) : mode(mode), minimize_frames(false), debug_checks(false) {}
        SolverConfig(SolverMode mode, bool minimize_frames) : mode(mode), minimize_frames(minimize_frames), debug_checks(false) {}
        SolverConfig(SolverMode mode, bool minimize_frames, bool debug_checks) : mode(mode), minimize_frames(minimize_frames), debug_checks(debug_checks) {}

        bool compareStates(CachedSolverState& a, CachedSolverState& b);
    };

    static CheckedScenario checkScenario(RawScenario& raw_scenario);
    static void loadScenario(CheckedScenario& scenario);
    static void solveScenario(SolverConfig& sovler, CheckedScenario& scenario);

    static uint16_t calcHeuristic(SolverConfig& solver, CheckedScenario& scenario, int next_corner, PlayerState& player, GameState& game);
    static uint16_t calcSimpleHeuristic(SolverConfig& solver, CheckedScenario& scenario, int next_corner, PlayerState& player, GameState& game);

    /// Creates an instance which contains the current game state
    static CachedSolverState cacheCurrentState(SolverConfig& solver);
    static CacheEntry createCacheEntry(SolverConfig& solver);

    static void loadState(SolverConfig& solver, CachedSolverState& state);
    static void loadCacheEntry(SolverConfig& solver, CacheEntry& entry);
}

#endif /* SOLVER_CLEAN_H */

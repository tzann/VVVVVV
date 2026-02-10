#ifndef SOLVER_CLEAN_H
#define SOLVER_CLEAN_H

#include "Exceptions.h"
#include "Geometry.h"
#include "Terrain.h"
#include "Scenarios.h"

#include <SDL.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

using namespace Geometry;
using namespace Terrain;

namespace SolverClean {
    using namespace Scenarios;

    // Hash functions for various data types
    extern std::hash<bool> hash_bool;
    extern std::hash<int> hash_int;
    extern std::hash<uint64_t> hash_uint64;
    extern std::hash<float> hash_float;
    inline uint64_t combineHashes(uint64_t h1, uint64_t h2) {
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }

    enum SolverMode {
        /// Simple heuristic that uses Manhattan distance and assumes max speed at all times
        SIMPLE,
        /// More complicated sim-based heuristic, factors in acceleration (but assumes instant decel)
        ACCEL_BASED_1,
        /// More complicated sim-based heuristic, factors in acceleration and deceleration for velocity changes
        ACCEL_BASED_2,
        MIN_INPUT_CHANGES,
        MIN_INPUT_FRAMES,
        MIN_FLIPS,
        // TODO: more modes, e.g. human viability?
    };

    /// This struct contains the debug information that's displayed on the HUD during solving
    struct DebugInfo {
        /// Elapsed milliseconds so far
        uint64_t millis;
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

        DebugInfo() {
            clear();
        }

        void clear() {
            millis = 0;
            states = 0;
            min_heuristic = 0;
            max_measure = 0;
            queue_size = 0;
            cache_size = 0;
        }
    };

    /// This struct contains the information that's displayed on the HUD when displaying a solution
    struct SolutionInfo {
        /// The index of the solution currently playing, if there is more than one
        uint64_t solution_idx;
        /// The total number of solutions found
        uint64_t num_solutions;
        /// How many frames are in the entire solution
        uint16_t length;
        /// The index of the frame currently being displayed
        uint16_t frame;
        /// The inputs of the current frame, where the bits in low-to-high order are Left, Right, Flip, (R, Enter)
        uint8_t input;
        /// The running total of the value being optimized for
        uint16_t measure;
        /// The total of the value being optimized for across the whole solution
        uint16_t solution_measure;

        SolutionInfo() {
            clear();
        }

        void clear() {
            solution_idx = 0;
            num_solutions = 0;
            length = 0;
            frame = 0;
            input = 0;
            measure = 0;
            solution_measure = 0;
        }
    };

    struct CheckedCorner {
        Terrain::RoomPosition room;
        Region region;
        CornerDir dir;

        CheckedCorner(Terrain::RoomPosition room, Region region, CornerDir dir) : room(room), region(region), dir(dir) {}
    
        bool isRegular() const {
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
        bool isTrinketOrWarp() const {
            switch (dir) {
            case TRINKET:
            case WARP_TOKEN:
                return true;
            default:
                return false;
            }
        }
        Terrain::CornerType getType() const {
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
        IntVector getRegularPos() const {
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

        bool isPosAfter(const Terrain::GlobalPosition& pos) const;
        bool isPosBefore(const Terrain::GlobalPosition& pos) const;
        bool isPosInside(const Terrain::GlobalPosition& pos) const;
        bool isPosNotStrictlyBefore(const Terrain::GlobalPosition& pos) const;

        Region getUnboundedRegion() const;
        Region getRegionAfter() const;
        Region getRegionBefore() const;
        Region getRegionInside() const;
    };

    struct CheckedScenario {
        Terrain::RoomPosition init_room;
        IntVector init_pos;
        bool inv_gravity;
        int advance_frames;
        std::vector<CheckedCorner> corners;

        /// Rooms that are guaranteed not to occur in the optimal solution
        std::vector<RoomPosition> ignore_rooms;

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

        Terrain::RoomPosition getRoomPos() const {
            // TODO: what about game.outside?
            return Terrain::RoomPosition::FromNativeRoomCoords(roomx, roomy);
        }

        uint64_t hash() const {
            // TODO: these values can be packed to minimize hash calls
            uint64_t result = 0;

            result = combineHashes(result, hash_int(state));
            result = combineHashes(result, hash_int(deathseq));
            result = combineHashes(result, hash_int(lifeseq));
            result = combineHashes(result, hash_bool(gravitycontrol));
            result = combineHashes(result, hash_int(roomx));
            result = combineHashes(result, hash_int(roomy));
            result = combineHashes(result, hash_bool(press_action));
            result = combineHashes(result, hash_int(jumppressed));
            result = combineHashes(result, hash_bool(jumpheld));
            result = combineHashes(result, hash_bool(press_right));
            result = combineHashes(result, hash_bool(press_left));

            // Any value greater than 5 is equivalent to 5
            int effective_tapright = SDL_min(tapright, 5);
            int effective_tapleft = SDL_min(tapleft, 5);
            result = combineHashes(result, hash_int(effective_tapright));
            result = combineHashes(result, hash_int(effective_tapleft));

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

        IntVector getPos() const {
            return IntVector(x, y);
        }

        uint64_t hash() const {
            // TODO: these values can be packed to minimize hash calls
            uint64_t result = hash_int(x);
            result = combineHashes(result, hash_int(y));

            result = combineHashes(result, hash_float(vx));
            result = combineHashes(result, hash_float(vy));

            // Should always be integers
            result = combineHashes(result, hash_int((int) ax));
            result = combineHashes(result, hash_int((int) ay));

            // Any value less than 0 is equivalent to 0
            int effective_onground = SDL_max(onground, 0);
            int effective_onroof = SDL_max(onroof, 0);
            result = combineHashes(result, hash_int(effective_onground));
            result = combineHashes(result, hash_int(effective_onroof));

            result = combineHashes(result, hash_bool(dir));

            result = combineHashes(result, hash_int(tile));

            // Any value less than 0 is equivalent to 0
            int effective_framedelay = SDL_max(framedelay, 0);
            result = combineHashes(result, hash_int(effective_framedelay));
            result = combineHashes(result, hash_int(drawframe));

            // Any value less than 0 is equivalent to 0
            int effective_visualonground = SDL_max(visualonground, 0);
            int effective_visualonroof = SDL_max(visualonroof, 0);
            result = combineHashes(result, hash_int(effective_visualonground));
            result = combineHashes(result, hash_int(effective_visualonroof));

            result = combineHashes(result, hash_int(walkingframe));
            result = combineHashes(result, hash_int(collisiondrawframe));

            // Any value less than 0 is equivalent to 0
            int effective_collisionframedelay = SDL_max(collisionframedelay, 0);
            result = combineHashes(result, hash_int(effective_collisionframedelay));
            result = combineHashes(result, hash_bool(collisionwalkingframe));

            result = combineHashes(result, hash_float(newxp));
            result = combineHashes(result, hash_float(newyp));

            return result;
        }
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

        uint64_t hash() const {
            uint64_t packed_value = 0;
            packed_value = (packed_value << 7) | (type + 1);
            packed_value = (packed_value << 4) | (rule + 1);

            uint64_t e_hash = hash_int(x);
            e_hash = combineHashes(e_hash, hash_int(y));
            e_hash = combineHashes(e_hash, hash_float(vy));
            e_hash = combineHashes(e_hash, hash_float(vx));

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

            e_hash = combineHashes(e_hash, hash_uint64(packed_value));

            return e_hash;
        }
        static uint64_t hash(const std::vector<EntityState>& es) {
            uint64_t result = hash_uint64(es.size());

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
                result = combineHashes(result, es[i].hash());
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

        uint64_t hash() const {
            uint64_t packed_value = rect_x + 100;
            packed_value = (packed_value << 10) | (rect_y + 100);
            packed_value = (packed_value << 9) | rect_w;
            packed_value = (packed_value << 9) | rect_h;
            packed_value = (packed_value << 3) | type;
            packed_value = (packed_value << 12) | trigger;
            return hash_uint64(packed_value);
        }
        static uint64_t hash(const std::vector<BlockState>& blocks) {
            uint64_t result = hash_uint64(blocks.size());

            for (int i = 0; i < blocks.size(); i++) {
                if (blocks[i].type == BLOCK || blocks[i].type == TRIGGER || blocks[i].type == ACTIVITY) {
                    result = combineHashes(result, blocks[i].hash());
                }
            }

            return result;
        }
    };

    struct CacheEntry {
        uint64_t entity_hash;
        uint64_t block_hash;

        CacheEntry() : entity_hash(0), block_hash(0) {}
        CacheEntry(uint64_t entity_hash, uint64_t block_hash) : entity_hash(entity_hash), block_hash(block_hash) {}

        uint64_t hash() const {
            return combineHashes(entity_hash, block_hash);
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
        uint32_t collect;

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
        uint8_t inputs;
        /// The hash of the state prior to this one
        uint64_t prevHash;

        /// Returns the measure associated with the given solver mode (e.g. frame count for SIMPLE)
        uint16_t getMeasure(SolverMode mode) const {
            switch (mode) {
            case SolverMode::SIMPLE:
            case SolverMode::ACCEL_BASED_1:
            case SolverMode::ACCEL_BASED_2:
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

        /// Returns the global position of the player
        Terrain::GlobalPosition getGlobalPos() const {
            return Terrain::GlobalPosition(game.getRoomPos(), player.getPos());
        }

        /// Returns true if the player is guaranteed to be able to flip on the next frame
        bool canFlip() const {
            return (!game.jumpheld || game.jumppressed > 0) && (player.onground > 0 && game.gravitycontrol == 0 || player.onroof > 0 && game.gravitycontrol == 1);
        }
        /// Returns true if the player is guaranteed to be able to double-flip on the next frame
        bool canDoubleFlip() const {
            return (!game.jumpheld || game.jumppressed > 0) && (player.onground > 0 && game.gravitycontrol == 0 && player.onroof > 0);
        }

        /// Returns the summary of this state, i.e. enough information to reconstruct it later
        StateSummary summary() const {
            return StateSummary(prevHash, inputs);
        }

        uint64_t hash() const {
            uint64_t result = player.hash();
            result = combineHashes(result, game.hash());
            result = combineHashes(result, cache_entry.hash());
            result = combineHashes(result, hash_uint64(collect));

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

        uint64_t size() const {
            uint64_t entity_size = sizeof(uint64_t) + sizeof(EntityState);
            uint64_t block_size = sizeof(uint64_t) + sizeof(BlockState);
            // TODO: what about entity sets?
            return entities.size() * entity_size + blocks.size() * block_size;
        }

        void clear() {
            entities.clear();
            blocks.clear();
            entity_sets.clear();
            block_sets.clear();
        }

        uint64_t cacheEntity(EntityState& entity) {
            uint64_t hash = entity.hash();
            if (entities.find(hash) == entities.end()) {
                entities.emplace(hash, entity);
            }
            return hash;
        }
        const EntityState& getEntity(uint64_t hash) const {
            Exceptions::assert(entities.find(hash) != entities.end());
            return entities.at(hash);
        }
        uint64_t cacheEntitySet(std::vector<uint64_t> entity_hashes) {
            uint64_t set_hash = hash_uint64(entity_hashes.size());
            for (uint64_t h : entity_hashes) {
                set_hash = combineHashes(set_hash, h);
            }

            if (entity_sets.find(set_hash) == entity_sets.end()) {
                entity_sets.emplace(set_hash, entity_hashes);
            }
            return set_hash;
        }
        const std::vector<uint64_t>& getEntitySet(uint64_t set_hash) const {
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
        const BlockState& getBlock(uint64_t hash) const {
            Exceptions::assert(blocks.find(hash) != blocks.end());
            return blocks.at(hash);
        }
        uint64_t cacheBlockSet(std::vector<uint64_t> block_hashes) {
            uint64_t set_hash = hash_uint64(block_hashes.size());
            for (uint64_t h : block_hashes) {
                set_hash = combineHashes(set_hash, h);
            }

            if (block_sets.find(set_hash) == block_sets.end()) {
                block_sets.emplace(set_hash, block_hashes);
            }
            return set_hash;
        }
        const std::vector<uint64_t>& getBlockSet(uint64_t set_hash) const {
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
        /// How many solutions do we want to find? (INT_MAX means all optimal solutions)
        int max_solutions;

        /// The entity cache is designed to reduce memory usage by only storing unique entity configurations
        EntityCache cache;
        /// These are simply used for rendering data on the HUD
        DebugInfo debug_info;
        SolutionInfo solution_info;
        /// How often the solver should render current state and debug info while solving
        uint64_t render_delay;

        SolverConfig() : mode(SolverMode::SIMPLE), minimize_frames(false), clean_inputs(false), debug_checks(false), max_solutions(1), render_delay(1000) {}
        SolverConfig(SolverMode mode) : mode(mode), minimize_frames(false), clean_inputs(false), debug_checks(false), max_solutions(1), render_delay(1000) {}
        SolverConfig(SolverMode mode, bool minimize_frames) : mode(mode), minimize_frames(minimize_frames), clean_inputs(false), debug_checks(false), max_solutions(1), render_delay(1000) {}
        SolverConfig(SolverMode mode, bool minimize_frames, bool clean_inputs) : mode(mode), minimize_frames(minimize_frames), clean_inputs(clean_inputs), debug_checks(false), max_solutions(1), render_delay(1000) {}
        SolverConfig(SolverMode mode, bool minimize_frames, bool clean_inputs, bool debug_checks) : mode(mode), minimize_frames(minimize_frames), clean_inputs(clean_inputs), debug_checks(debug_checks), max_solutions(1), render_delay(1000) {}
        SolverConfig(SolverMode mode, bool minimize_frames, bool clean_inputs, bool debug_checks, int max_solutions) : mode(mode), minimize_frames(minimize_frames), clean_inputs(clean_inputs), debug_checks(debug_checks), max_solutions(max_solutions), render_delay(1000) {}

        bool compareStates(const CachedSolverState& a, const CachedSolverState& b) const;
        std::size_t operator()(const CachedSolverState& a, const CachedSolverState& b) const {
            return compareStates(a, b);
        }
    };

    void runSolver(void);

    static CheckedScenario checkScenario(const RawScenario& raw_scenario);
    static void loadScenario(const CheckedScenario& scenario);
    static void solveScenario(SolverConfig& sovler, const CheckedScenario& scenario);
    
    static uint16_t updateHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, CachedSolverState& state);
    static uint16_t calcHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game);
    
    static uint16_t calcSimpleHeuristic(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game);
    static uint16_t calcSimpleHeuristicCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c);
    static uint16_t calcSimpleHeuristicWarpCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c);
    static uint16_t calcSimpleHeuristicFinalCorner(RoomPosition& room_pos, Region& pos, const CheckedCorner& c);
    static uint16_t calcSimpleHeuristicSegment(Region& fromPos, const Region& toPos);

    static uint16_t calcAccelHeuristic1(const SolverConfig& solver, const CheckedScenario& scenario, int next_corner, const PlayerState& player, const GameState& game);

    /// Creates an instance which contains the current game state
    static CachedSolverState cacheCurrentState(SolverConfig& solver);
    static CacheEntry createCacheEntry(SolverConfig& solver);

    static CachedSolverState cacheCurrentStateWithDeltaFromPrev(SolverConfig& solver, const CheckedScenario& scenario, const CachedSolverState& prev, uint64_t prevHash);

    static void loadState(const SolverConfig& solver, const CachedSolverState& state);
    static void loadCacheEntry(const SolverConfig& solver, const CacheEntry& entry);

    static void doGameStep();
    static void render(const SolverConfig& solver);
    static void renderHUD(const SolverConfig& solver);

    /// Returns true if the player is guaranteed to be able to flip on the next frame
    inline static bool canFlip(const PlayerState& player, const GameState& game) {
        return (!game.jumpheld || game.jumppressed > 0) && (player.onground > 0 && game.gravitycontrol == 0 || player.onroof > 0 && game.gravitycontrol == 1);
    }
    /// Returns true if the player is guaranteed to be able to double-flip on the next frame
    inline static bool canDoubleFlip(const PlayerState& player, const GameState& game) {
        return (!game.jumpheld || game.jumppressed > 0) && (player.onground > 0 && game.gravitycontrol == 0 && player.onroof > 0);
    }
}

#endif /* SOLVER_CLEAN_H */

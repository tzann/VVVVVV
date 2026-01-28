#include "solver/SolverClean.h"

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

#include <queue>
#include <functional>
#include <unordered_set>
#include <algorithm>
#include <iostream>

namespace Solver {
    using namespace Geometry;
    using Exceptions::VVV_assert;
    using Exceptions::assert;
    using Terrain::RoomPosition;
    using Terrain::GlobalPosition;

    void solveScenario(SolverConfig& solver, CheckedScenario& scenario) {

    }

    static uint16_t calcHeuristic(SolverConfig& solver, CheckedScenario& scenario, int next_corner, PlayerState& player, GameState& game) {
        GlobalPosition pos = GlobalPosition(RoomPosition::FromNativeRoomCoords(game.roomx, game.roomy), IntVector(player.x, player.y));

        if (solver.debug_checks) {
            assert(0 <= next_corner && next_corner <= scenario.corners.size());
            if (next_corner < scenario.corners.size()) {
                CheckedCorner& next = scenario.corners[next_corner];
                assert(!next.isPosAfter(pos));
            }
            if (next_corner >= 1) {
                CheckedCorner& last = scenario.corners[next_corner - 1];
                assert(!last.isPosBefore(pos));
            }
        }

        // Advance to the next corner that we are strictly before
        int c_start_idx = next_corner;
        while (c_start_idx < scenario.corners.size() && !scenario.corners[c_start_idx].isPosBefore(pos)) {
            c_start_idx++;
        }

        // Call the appropriate heuristic function for the solver mode
        switch (solver.mode) {
        case SolverMode::SIMPLE:
            return calcSimpleHeuristic(solver, scenario, c_start_idx, player, game);
        default:
            Exceptions::todo();
        }
    }

    static uint16_t calcSimpleHeuristic(SolverConfig& solver, CheckedScenario& scenario, int next_corner, PlayerState& player, GameState& game) {
        int total_frames = 0;

        IntVector pos_vec = IntVector(room_adjusted_x(game.roomx, player.x), room_adjusted_y(game.roomy, player.y));

        Region pos = Region(pos_vec, pos_vec);

        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            CheckedCorner& c = scenario.corners[c_idx];

            Region& c_r = c.region;

            IntInterval x_diff = pos.x - c_r.x;
            IntInterval y_diff = pos.y - c_r.y;

            int x_d = 0;
            if (x_diff.contains(0)) {
                x_d = 0;
            }
            else if (x_diff.is_negative()) {
                x_d = x_diff.getUpperBound();
            }
            else if (x_diff.is_positive()) {
                x_d = x_diff.getLowerBound();
            }
            int y_d = 0;
            if (y_diff.contains(0)) {
                y_d = 0;
            }
            else if (y_diff.is_negative()) {
                y_d = y_diff.getUpperBound();
            }
            else if (y_diff.is_positive()) {
                y_d = y_diff.getLowerBound();
            }

            int frame_count = 0;
            IntInterval x_dist = VX_INT_RANGE;
            IntInterval y_dist = VY_INT_RANGE; 

            // Note that if we are "inside" the corner (e.g. x_d > 0 && y_d < 0 for UP_LEFT),
            //   then we just pretend we can walk through walls. The max corner cut distance constraint
            //   makes sure we don't completely wreck our heuristic
            // Note also that if we are already past the corner (i.e. x_d <= 0 for UP_LEFT),
            //   then we do nothing as we want to preserve min_x, max_x, min_y and max_y for the next corner
            switch (c.dir) {
            case UP_LEFT: // Limiting factor: leftwards movement
                if (x_d > 0) {
                    // How long until we pass the corner?
                    frame_count = div_ceil(x_d, MAX_VX);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Corner pos is upper bound (we need to be to the left)
                    pos.x.intersect(c_r.x);
                    // Can't cut more than 10 pixels past the corner vertically
                    pos.y.intersect(c_r.y - MAX_VY);
                }
                break;
            case UP_RIGHT: // Limiting factor: rightwards movement
                if (x_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = div_ceil(-x_d, MAX_VX);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Corner pos is lower bound (we need to be to the right)
                    pos.x.intersect(c_r.x);
                    // Can't cut more than 10 pixels past the corner vertically
                    pos.y.intersect(c_r.y - MAX_VY);
                }
                break;
            case LEFT_UP: // Limiting factor: upwards movement
                if (y_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = div_ceil(y_d, MAX_VY);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Can't cut past the corner horizontally
                    pos.x.intersect(c_r.x);
                    // Corner pos is upper bound (we need to be above)
                    pos.y.intersect(c_r.y);
                }
                break;
            case LEFT_DOWN: // Limiting factor: downwards movement
                if (y_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = div_ceil(-y_d, MAX_VY);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Can't cut past the corner horizontally
                    pos.x.intersect(c_r.x);
                    // Corner pos is lower bound (we need to be above)
                    pos.y.intersect(c_r.y);
                }
                break;
            case DOWN_LEFT:
                if (x_d > 0) {
                    // How long until we pass the corner?
                    frame_count = div_ceil(x_d, MAX_VX);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Corner pos is upper bound (we need to be to the left)
                    pos.x.addUpperBound(c_r.x.max);
                    // Can't cut more than 10 pixels past the corner vertically
                    pos.y.addLowerBound(c_r.y.min + MAX_VY);
                }
                break;
            case DOWN_RIGHT:
                if (x_d < 0) {
                    // How long until we pass the corner?
                    frame_count = div_ceil(-x_d, MAX_VX);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Corner pos is upper bound (we need to be to the left)
                    pos.x.addUpperBound(c_r.x.max);
                    // Can't cut more than 10 pixels past the corner vertically
                    pos.y.addLowerBound(c_r.y.min + MAX_VY);
                }
                break;
            case RIGHT_UP:
                if (y_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = div_ceil(y_d, MAX_VY);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Can't cut past the corner horizontally
                    pos.x.intersect(c_r.x);
                    // Corner pos is upper bound (we need to be above)
                    pos.y.intersect(c_r.y);
                }
                break;
            case RIGHT_DOWN:
                if (y_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = div_ceil(-y_d, MAX_VY);
                    pos.x += x_dist * frame_count;
                    pos.y += y_dist * frame_count;

                    // Can't cut past the corner horizontally
                    pos.x.intersect(c_r.x);
                    // Corner pos is upper bound (we need to be above)
                    pos.y.intersect(c_r.y);
                }
                break;
            case TRINKET:
                // Could be in any direction
                // At least how many frames will it take to reach the trinket?
                int x_frames = div_ceil(SDL_abs(x_d), MAX_VX);
                int y_frames = div_ceil(SDL_abs(y_d), MAX_VY);
                frame_count = SDL_max(x_frames, y_frames);
                pos.x += x_dist * frame_count;
                pos.y += y_dist * frame_count;

                // What are the min and max positions reachable while still collecting the trinket?
                pos.x.intersect(c_r.x);
                pos.y.intersect(c_r.y);
            }

            total_frames += frame_count;
        }

        // TODO: account for final distance to get past the last corner, not just no longer be before it

        return total_frames;
    }

    CheckedScenario checkScenario(RawScenario& raw_scenario) {
        using Terrain::RoomPosition;

        std::vector<CheckedCorner> checked_corners;
        checked_corners.reserve(raw_scenario.corners.size());

        // TODO: we could also check for ill-formed corner sequences here

        RoomPosition current_room = RoomPosition(-1, -1);
        for (int c_idx = 0; c_idx < raw_scenario.corners.size(); c_idx++) {
            RawCorner& c = raw_scenario.corners[c_idx];
            bool shiny_or_warp = c.dir == CornerDir::TRINKET || c.dir == CornerDir::WARP_TOKEN;

            RoomPosition c_room = RoomPosition::FromNativeRoomCoords(c.rx, c.ry);;
            if (c_room != current_room) {
                current_room = c_room;
                // Load the next room
                Terrain::LoadRoom(current_room);
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
                VVV_assert(!collision_bitmap[bitmap_width * (c.y - min.y - 1) + (c.x - min.x - 1)], 570000 + c_idx);

                int occupied_count = 0;
                bool above_occupied = false;
                bool left_occupied = false;
                bool below_occupied = false;
                bool right_occupied = false;
                for (int px = c.x - 1; px <= c.x + 1; px++) {
                    for (int py = c.y - 1; py <= c.y + 1; py++) {
                        if (collision_bitmap[bitmap_width * (py - min.y - 1) + (px - min.x - 1)]) {
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
                    if (collision_bitmap[bitmap_width * (y - min.y - 1) + (x_gap - min.x - 1)]) {
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
                    if (collision_bitmap[bitmap_width * (y_gap - min.y - 1) + (x - min.x - 1)]) {
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

                checked_corners.emplace_back(std::forward_as_tuple(c_room, reg, c.dir));
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
                        if (!collision_bitmap[bitmap_width * (py - min.y - 1) + (px - min.x - 1)]) {
                            // The trinket can be collected, i.e. this pos is not OoB
                            Region point = Region(IntVector(px, py));
                            box.join(point);
                        }
                    }
                }

                // Sanity check: At least one pixel of the trinket is not OoB
                VVV_assert(!box.is_bottom() && box.is_bounded(), 573000 + c_idx);

                checked_corners.emplace_back(std::forward_as_tuple(c_room, box, c.dir));
            }

            // Free the collision bitmap
            SDL_free((void*)collision_bitmap);
        }

        RoomPosition init_room = RoomPosition::FromNativeRoomCoords(raw_scenario.init_rx, raw_scenario.init_ry);
        IntVector init_pos = IntVector(raw_scenario.init_x, raw_scenario.init_y);
        CheckedScenario res = CheckedScenario(init_room, init_pos, raw_scenario.init_gravity != 0, raw_scenario.frames_to_advance, checked_corners);
        return res;
    }

    void loadScenario(CheckedScenario& scenario) {
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

        if (obj.entities.empty())
        {
            obj.createentity(game.savex, game.savey, 0, 0);
        }
        map.resetplayer();
        Terrain::LoadRoom(scenario.init_room);
        map.initmapdata();

        graphics.fademode = FADE_NONE;
        game.jumppressed = false;

        // Hack: Advance frames to fix enemy cycles to right position
        for (int i = 0; i < scenario.advance_frames; i++) {
            key.clearKeys();
            do_game_step(true);
            SDL_Delay(34);
        }

        key.clearKeys();
    }

    static CachedSolverState cacheCurrentState(SolverConfig& solver)
    {
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
        entities.reserve(obj.entities.size());
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

    static void loadState(SolverConfig& solver, CachedSolverState& state) {
        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            // hacky fix pt1
            obj.collect[i] = false;
        }

        // Load room
        if (game.roomx != state.game.roomx || game.roomy != state.game.roomy) {
            // Only load if it's necessary. this might behave weirdly in rooms where sprites are deleted?
            gotoroom(state.game.roomx, state.game.roomy);
            // TODO: restore collected trinkets if not reloading room
        }

        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            // hacky fix pt2
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
    }
    static void loadCacheEntry(SolverConfig& solver, CacheEntry& entry) {
        if (entry.entity_hash != 0) {
            std::vector<std::size_t>& cached_entities = solver.cache.getEntitySet(entry.entity_hash);
            // Load entity data
            for (int i = 0; i < cached_entities.size(); i++) {
                EntityState& e = solver.cache.getEntity(cached_entities[i]);
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
            std::vector<std::size_t>& cached_blocks = solver.cache.getBlockSet(entry.block_hash);
            // Load block data
            for (int i = 0; i < cached_blocks.size(); i++) {
                BlockState& b = solver.cache.getBlock(cached_blocks[i]);
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

    /// Returns true if state `a` should come *after* state `b` in processing order
    bool SolverConfig::compareStates(CachedSolverState& a, CachedSolverState& b) {
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

        if (clean_inputs && (a.input_change_count != b.input_change_count)) {
            // Prioritize low changes in inputs, so the resulting solution is more human-viable
            return a.input_change_count > b.input_change_count;
        }

        return true;
    }

    bool CheckedCorner::isPosAfter(Terrain::GlobalPosition& pos) {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "after" the corner means being sure that
        // we have already passed it. For trinkets and warp tokens, we can't
        // "really" know just based off the position, except if we're touching it
        if (this->isTrinketOrWarp()) {
            int glob_x = ROOM_W * (pos.room.rx - room.rx) + pos.pos.x;
            int glob_y = ROOM_H * (pos.room.ry - room.ry) + pos.pos.y;
            return region.contains(IntVector(glob_x, glob_y));
        }
        else if (this->isRegular()) {
            int rx_delta = ROOM_W * (pos.room.rx - room.rx);
            int ry_delta = ROOM_H * (pos.room.ry - room.ry);
            IntVector c_pos = this->getRegularPos();
            int x_delta = rx_delta + pos.pos.x - c_pos.x;
            int y_delta = rx_delta + pos.pos.y - c_pos.y;
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
    bool CheckedCorner::isPosBefore(Terrain::GlobalPosition& pos) {
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
            int rx_delta = ROOM_W * (pos.room.rx - room.rx);
            int ry_delta = ROOM_H * (pos.room.ry - room.ry);
            IntVector c_pos = this->getRegularPos();
            int x_delta = rx_delta + pos.pos.x - c_pos.x;
            int y_delta = rx_delta + pos.pos.y - c_pos.y;
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
    bool CheckedCorner::isPosInside(Terrain::GlobalPosition& pos) {
        if (room.outside != pos.room.outside) {
            Exceptions::invalid_argument();
        }

        // In this context, being "inside" the corner means being out of bounds
        // So, touching a trinket or warp token doesn't count as being "inside"
        if (this->isTrinketOrWarp()) {
            return false;
        } else if (this->isRegular()) {
            int rx_delta = ROOM_W * (pos.room.rx - room.rx);
            int ry_delta = ROOM_H * (pos.room.ry - room.ry);
            IntVector c_pos = this->getRegularPos();
            int x_delta = rx_delta + pos.pos.x - c_pos.x;
            int y_delta = rx_delta + pos.pos.y - c_pos.y;
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
}
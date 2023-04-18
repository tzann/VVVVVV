#include "Solver.h"

#include <SDL.h>

#include "CustomLevels.h"
#include "DeferCallbacks.h"
#include "Editor.h"
#include "Enums.h"
#include "Entity.h"
#include "Exit.h"
#include "FileSystemUtils.h"
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
#include "Vlogging.h"

#include <queue>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <iostream>

namespace Solver {

    const int X_SPEED = 6;
    const int Y_SPEED = 10;

    // TODO diagnostics to see if this is actually faster
    struct modified_hash {
        /*
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

        std::size_t operator()(std::size_t x) const
        {
            return x;
        }
    };

    namespace SS1 {
        namespace CORNERS {
            // Solitude
            const corner SOLITUDE(115, 105, 114, 126, UP_RIGHT);

            // Traffic Jam
            namespace TRAFFIC_JAM {
                const corner ENTRY_1(115, 103, 250, 182, UP_RIGHT);
                const corner ENTRY_2(115, 103, 250, 97, LEFT_UP);
            }

            // Atmospheric Filtering Unit (part 1)
            namespace ATMOSPHERIC_FILTERING_UNIT {
                const corner ENTRY_1(114, 103, 258, 73, LEFT_UP);
                const corner ENTRY_2(114, 103, 222, 73, DOWN_LEFT);
            }

            // It's a Secret to Nobody
            namespace ITS_A_SECRET_TO_NOBODY {
                const corner ENTRY(114, 104, 202, 78, LEFT_DOWN);
                const corner TRINKET(114, 104, 16, 136, corner_dir::TRINKET);
                const corner EXIT(114, 104, 202, 78, UP_RIGHT);
            }

            // Atmospheric Filtering Unit (part 2)
            namespace ATMOSPHERIC_FILTERING_UNIT {
                const corner EXIT_1(114, 103, 74, 142, LEFT_DOWN);
                const corner EXIT_2(114, 103, 38, 142, UP_LEFT);
                const corner EXIT_3(114, 103, 18, 97, LEFT_UP);
            }

            // Linear collider
            namespace LINEAR_COLLIDER {
                const corner ENTRY_1(113, 103, 278, 97, DOWN_LEFT);
                const corner ENTRY_2(113, 103, 264, 182, LEFT_DOWN);
                const corner ENTRY_3(113, 103, 230, 182, UP_LEFT);
                const corner ENTRY_4(113, 103, 218, 129, LEFT_UP);
                const corner EXIT_1(113, 103, 78, 78, UP_LEFT);
                const corner EXIT_2(113, 103, 66, 25, LEFT_UP);
                const corner EXIT_3(113, 103, 30, 25, DOWN_LEFT);
                const corner EXIT_4(113, 103, 18, 110, DOWN_LEFT);
            }

            // Security Sweep
            namespace SECURITY_SWEEP {
                const corner ENTRY_1(112, 103, 294, 110, UP_LEFT);
                const corner ENTRY_2(112, 103, 258, 33, LEFT_UP);
                const corner FLIP_DOWN(112, 103, 206, 33, DOWN_LEFT);
                const corner GO_LEFT(112, 103, 186, 142, LEFT_DOWN);
                const corner EXIT(112, 103, 78, 161, DOWN_LEFT);
            }

            // Gantry and Dolly (part 1)
            namespace GANTRY_AND_DOLLY {
                const corner ENTRY_1(112, 104, 78, 46, RIGHT_DOWN);
                const corner ENTRY_2(112, 104, 86, 70, RIGHT_DOWN);
            }

            // Comms Relay
            namespace COMMS_RELAY {
                const corner STEPS(113, 104, 10, 65, DOWN_RIGHT);
                const corner LEDGE(113, 104, 242, 97, DOWN_RIGHT);
                const corner SNAKE_1(113, 104, 242, 190, LEFT_DOWN);
                const corner SNAKE_2(113, 104, 206, 190, UP_LEFT);
                const corner SNAKE_3(113, 104, 194, 161, LEFT_UP);
                const corner SNAKE_4(113, 104, 134, 161, DOWN_LEFT);
                const corner SNAKE_5(113, 104, 122, 190, LEFT_DOWN);
                const corner SNAKE_6(113, 104, 86, 190, UP_LEFT);
                const corner SNAKE_7(113, 104, 74, 161, LEFT_UP);
                const corner SNAKE_8(113, 104, 38, 161, DOWN_LEFT);
            }

            // Gantry and Dolly (part 2)
            namespace GANTRY_AND_DOLLY {
                const corner EXIT(112, 104, 78, 185, DOWN_LEFT);
            }

            // The Yes Men
            namespace THE_YES_MEN {
                const corner ENTRY(112, 105, 78, 38, RIGHT_DOWN);
                const corner DROP(112, 105, 258, 73, DOWN_RIGHT);
                const corner LEFT(112, 105, 258, 134, LEFT_DOWN);
                const corner EXIT(112, 105, 78, 169, DOWN_LEFT);
            }

            // Stop and Reflect (part 1)
            namespace STOP_AND_REFLECT {
                const corner ENTRY(112, 106, 78, 46, RIGHT_DOWN);
                const corner LEDGE(112, 106, 114, 81, DOWN_RIGHT);
            }

            // Trench Warfare
            const corner TRENCH_WARFARE(113, 106, 272, 152, TRINKET);

            // Stop and Reflect (part 2)
            namespace STOP_AND_REFLECT {
                const corner EXIT_LEDGE(112, 106, 114, 126, LEFT_DOWN);
                const corner EXIT_SPIKE(112, 106, 78, 161, DOWN_LEFT);
            }

            // V Stitch
            const corner V_STITCH(112, 107, 78, 22, RIGHT_DOWN);

            // Quicksand
            namespace QUICKSAND {
                const corner ENTRY(116, 106, 86, 41, RIGHT_UP);
                const corner LEDGE(116, 106, 122, 41, DOWN_RIGHT);
                const corner SPIKE(116, 106, 154, 121, DOWN_RIGHT); // Spike corner, not sure if useful
            }

            // The Tomb of Mad Carew
            namespace THE_TOMB_OF_MAD_CAREW {
                const corner ENTRY_1(116, 107, 158, 22, RIGHT_DOWN);
                const corner ENTRY_2(116, 107, 174, 38, RIGHT_DOWN);
            }

            // Brass Sent Us Under The Top
            const corner BRASS_SENT_US_UNDER_THE_TOP(117, 107, 250, 89, DOWN_RIGHT);

            // A Wrinkle in Time
            namespace A_WRINKLE_IN_TIME {
                const corner IL_END(118, 107, 55, 121, DOWN_RIGHT); // IL ending box
                const corner STEPS(118, 107, 98, 121, DOWN_RIGHT); // first teleporter step
            }
        }
        namespace SCENARIOS {
            const Scenario START_TO_FIRST_TRINKET(113, 105, 200, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ITS_A_SECRET_TO_NOBODY::ENTRY,
                CORNERS::ITS_A_SECRET_TO_NOBODY::TRINKET,
                CORNERS::ITS_A_SECRET_TO_NOBODY::EXIT,
                });

            const Scenario START(113, 105, 200, 161, 0, 0, { CORNERS::SOLITUDE });
            
            const Scenario TRAFFIC_JAM_TO_SECRET(115, 105, 57, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                });

            const Scenario ITS_A_SECRET_TO_NOBODY(114, 103, 301, 46, 1, 0, {
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ITS_A_SECRET_TO_NOBODY::ENTRY,
                CORNERS::ITS_A_SECRET_TO_NOBODY::TRINKET,
                CORNERS::ITS_A_SECRET_TO_NOBODY::EXIT,
                });

            const Scenario TRAFFIC_JAM_TO_LINEAR_COLLIDER(115, 105, 57, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_2,
                });
            
            const Scenario LINEAR_COLLIDER(114, 103, -14, 46, 1, 0, {
                CORNERS::LINEAR_COLLIDER::ENTRY_1,
                CORNERS::LINEAR_COLLIDER::ENTRY_2,
                CORNERS::LINEAR_COLLIDER::ENTRY_3,
                CORNERS::LINEAR_COLLIDER::ENTRY_4,
                CORNERS::LINEAR_COLLIDER::EXIT_1,
                CORNERS::LINEAR_COLLIDER::EXIT_2,
                // These two corners make the search run 10x longer and add nothing
                // TODO: Improving the heuristic could fix that
                // CORNERS::LINEAR_COLLIDER::EXIT_3,
                // CORNERS::LINEAR_COLLIDER::EXIT_4,
                });

            const Scenario SECURITY_SWEEP(113, 103, -14, 161, 0, 0, {
                CORNERS::SECURITY_SWEEP::ENTRY_1,
                CORNERS::SECURITY_SWEEP::ENTRY_2,
                CORNERS::SECURITY_SWEEP::FLIP_DOWN,
                CORNERS::SECURITY_SWEEP::GO_LEFT,
                CORNERS::SECURITY_SWEEP::EXIT,
                });

            const Scenario GANTRY_AND_DOLLY_TO_COMMS_RELAY(112, 103, 150, 161, 0, 0, {
                CORNERS::SECURITY_SWEEP::EXIT,
                CORNERS::GANTRY_AND_DOLLY::ENTRY_1,
                CORNERS::GANTRY_AND_DOLLY::ENTRY_2,
                CORNERS::COMMS_RELAY::STEPS,
                CORNERS::COMMS_RELAY::LEDGE,
                CORNERS::COMMS_RELAY::SNAKE_1,
                CORNERS::COMMS_RELAY::SNAKE_2,
                CORNERS::COMMS_RELAY::SNAKE_3,
                CORNERS::COMMS_RELAY::SNAKE_4,
                CORNERS::COMMS_RELAY::SNAKE_5,
                CORNERS::COMMS_RELAY::SNAKE_6,
                CORNERS::COMMS_RELAY::SNAKE_7,
                CORNERS::COMMS_RELAY::SNAKE_8,
                });

            // TODO: gantry and dolly part 2

            const Scenario THE_YES_MEN(112, 104, 113, 185, 0, 0, {
                CORNERS::GANTRY_AND_DOLLY::EXIT,
                CORNERS::THE_YES_MEN::ENTRY,
                CORNERS::THE_YES_MEN::DROP,
                CORNERS::THE_YES_MEN::LEFT,
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                });

            const Scenario THE_YES_MEN_TO_VSTITCH(112, 104, 113, 185, 0, 0, {
                CORNERS::GANTRY_AND_DOLLY::EXIT,
                CORNERS::THE_YES_MEN::ENTRY,
                CORNERS::THE_YES_MEN::DROP,
                CORNERS::THE_YES_MEN::LEFT,
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::STOP_AND_REFLECT::LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_SPIKE,
                CORNERS::V_STITCH,
                });

            const Scenario THE_YES_MEN_TO_TRENCH_WARFARE(112, 104, 113, 185, 0, 0, {
                CORNERS::GANTRY_AND_DOLLY::EXIT,
                CORNERS::THE_YES_MEN::ENTRY,
                CORNERS::THE_YES_MEN::DROP,
                CORNERS::THE_YES_MEN::LEFT,
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::TRENCH_WARFARE,
                });

            const Scenario STOP_AND_REFLECT_TO_TRENCH_WARFARE(112, 105, 235, 134, 1, 48, {
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::TRENCH_WARFARE,
                });

            const Scenario STOP_AND_REFLECT_TO_VSTITCH(112, 105, 235, 134, 1, 48, {
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::STOP_AND_REFLECT::LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_SPIKE,
                CORNERS::V_STITCH,
                });

            const Scenario TRENCH_WARFARE_GRAB(112, 106, 297, 134, 0, 0, {
                CORNERS::TRENCH_WARFARE,
                });

            const Scenario TRENCH_WARFARE_BACK_TO_VSTITCH(113, 106, 255, 152, 0, 40, {
                CORNERS::STOP_AND_REFLECT::EXIT_LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_SPIKE,
                CORNERS::V_STITCH,
                });

            // TODO: V Stitch to Quicksand

            /*
            // Before Quicksand
            static int init_rx = 115;
            static int init_ry = 106;
            static int init_x = 293;
            static int init_y = 22;
            static int init_gravity = 1;
            static int frames_to_advance = 0;
            */

            /*
            // Before The Tomb of Mad Carew
            static int init_rx = 116;
            static int init_ry = 106;
            static int init_x = 154;
            static int init_y = 57;
            static int init_gravity = 0;
            static int frames_to_advance = 94;
            */
        }
    }

    namespace WZ {
        namespace CORNERS {
            namespace THIS_IS_HOW_IT_IS {
                const corner FLIP_UP(114, 101, 122, 46, UP_RIGHT);
                const corner GO_RIGHT(114, 101, 174, 161, RIGHT_UP);
                // const corner SCREEN_EDGE(114, 101, 308, ??, X_ONLY);
            }

            namespace BISECTED_SPIRAL {
                const corner FLIP_UP(115, 101, 222, 158, UP_LEFT);
            }

            // TODO more corners

            namespace I_LOVE_YOU {
                // TODO more corners
                const corner WARP_TOKEN(116, 100, 152, 112, WARP_TOKEN);
            }
            namespace THATS_WHY_I_HAVE_TO_KILL_YOU {
                // TODO more corners
                const corner WARP_TOKEN(114, 102, 152, 112, WARP_TOKEN);
            }
        }
        namespace SCENARIOS {
            const Scenario TWIHTKY_STUPID(114, 102, 9, 120, 0, 0, {
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
            const Scenario TWIHTKY_STUPID_2(116, 100, 40, 102, 1, 97, {
                CORNERS::I_LOVE_YOU::WARP_TOKEN,
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
        }
    }

    // Benchmarks before map & set improvement:
    // Linear Collider: 0:45.5 (1.09M states visited)
    // Security Sweep:  0:31.1 (1.03M states visited)
    // The Yes Men:     3:49.4 (4.62M states visited)
    // Benchmarks after:
    // Linear Collider: 0:42.1 (1.09M states visited)
    // Security Sweep:  0:28.9 (1.03M states visited)
    // The Yes Men:     3:30.3 (4.62M states visited)
    // Benchmarks after game optimizations and gotoroom elision:
    // Linear Collider: 0:12.8 (1.09M states visited)
    // Security Sweep:  0:28.9 (1.03M states visited)
    // The Yes Men:     1:42.4 (4.62M states visited)
    static Scenario scenario = SS1::SCENARIOS::THE_YES_MEN;

    static std::unordered_map<std::size_t, naiveenemystate, modified_hash> entitycache;
    static std::unordered_map<std::size_t, naiveblockstate, modified_hash> blockcache;
    static std::unordered_map<std::size_t, std::vector<std::size_t>, modified_hash> entity_set_cache;
    static std::unordered_map<std::size_t, std::vector<std::size_t>, modified_hash> block_set_cache;

    // TODO: look into better hashing ideas - use what we know about the data to make it faster (without introducing collisions)
    // TODO: naiveblockstate is not optimized

    void entrypoint() {
        // TODO is this really necessary
        // Get into GAMEMODE gracefully
        script.startgamemode(Start_SECRETLAB);
        // Give back trinkets
        for (int i = 0; i < 20; i++) {
            obj.collect[i] = false;
        }
        // Don't do cutscenes
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

        // Uncomment this to skip
        // debug_test();
        // debug_test_cached();
        // squish_test();

        // Solve
        // stateful_solver();
        cached_stateful_solver();
        // stateless_solver(true);
    }

    void squish_test() {
        while (true) {
            game.savedir = 1;
            game.savepoint = 0;
            game.gravitycontrol = 1;

            game.state = 0;
            game.deathseq = -1;
            game.lifeseq = 0;

            if (obj.entities.empty())
            {
                obj.createentity(game.savex, game.savey, 0, 0);
            }

            /* zip squish
            map.resetplayer();
            map.gotoroom(108, 114);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();
            obj.entities[0].xp = 262;
            obj.entities[0].yp = 200;

            obj.createentity(168, 130, 2, 3, 6);
            obj.createentity(162, 111, 2, 3, 6);
            obj.createentity(156, 92, 2, 3, 6);
            obj.createentity(150, 73, 2, 3, 6);
            obj.createentity(144, 54, 2, 3, 6);
            obj.createentity(138, 35, 2, 3, 6);
            obj.createentity(132, 16, 2, 3, 6);
            */

            /* corner cut squish
            map.resetplayer();
            map.gotoroom(105, 108);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();
            key.setKey(KEYBOARD_RIGHT, true);

            game.gravitycontrol = 0;
            obj.entities[0].xp = 96;
            obj.entities[0].yp = 50;
            obj.createentity(7, 102, 2, 3, 12);

            for (int i = 0; i < 20; i++) {
                game.hours = obj.entities[0].vx * 10;
                game.deathcounts = obj.entities[0].xp - obj.entities[1].xp - obj.entities[1].w;
                do_game_step(true);
                SDL_Delay(680);
            }*/

            /* vertical squish clip into ground
            map.resetplayer();
            map.gotoroom(105, 107);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();

            game.gravitycontrol = 1;
            obj.entities[0].xp = 170;
            obj.entities[0].yp = 106;
            // obj.createentity(7, 102, 2, 3, 6);
            obj.createentity(160, 83, 2, 0, 4);

            for (int i = 0; i < 20; i++) {
                key.clearKeys();
                key.setKey(KEYBOARD_v, i == 6);
                game.hours = i == 3;
                game.deathcounts = obj.entities[0].vy * 10;
                do_game_step(true);
                SDL_Delay(340);
            }
            */

            /* levitating viridian
            map.resetplayer();
            map.gotoroom(105, 107);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();

            game.gravitycontrol = 0;
            obj.entities[0].xp = 100;
            obj.entities[0].yp = 57;
            obj.createentity(100, 264, 2, 1, 12, 0, -100, 320, 340);
            obj.createentity(64, 120, 2, 3, 1);
            obj.createentity(64, 100, 2, 3, 1);
            obj.createentity(64, 90, 2, 3, 1);

            for (int i = 0; i < 20; i++) {
                key.clearKeys();
                key.setKey(KEYBOARD_v, false);
                do_game_step(true);
                SDL_Delay(340);
            } */


            map.resetplayer();
            map.gotoroom(105, 107);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();

            game.gravitycontrol = 0;
            obj.entities[0].xp = 100;
            obj.entities[0].yp = 100;
            obj.createentity(100, 240, 2, 1, 6, 0, 132, 320, 340);
            obj.createentity(0, 100, 11, 320);

            for (int i = 0; i < 25; i++) {
                key.clearKeys();
                key.setKey(KEYBOARD_v, i == 10 || i == 20);
                obj.entities[0].dir = key.isDown(KEYBOARD_v);
                game.deathcounts = obj.entities[0].onground;
                game.hours = obj.entities[0].yp;
                do_game_step(true);
                SDL_Delay(340);
            }
        }
    }

    void stateful_solver() {
        load_scenario();
        naivestate initial_state = create_naive_state();
        initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y);

        std::size_t initial_hash = hash_naivestate(initial_state);

        std::unordered_set<std::size_t, modified_hash> hash_set;
        std::unordered_map<std::size_t, stateinfo, modified_hash> prev_state_map;
        naivestate s;
        { // start q scope
            std::priority_queue<naivestate, std::vector<naivestate>, std::function<bool(naivestate, naivestate)>> q(compare_naive_states);

            q.push(initial_state);

            while (q.size() > 0) {
                s = q.top();
                q.pop();

                // Calculate state hash. If we have already seen the same hash, we can skip this branch.
                std::size_t s_hash = hash_naivestate(s);
                if (hash_set.find(s_hash) != hash_set.end()) {
                    // State already exists (with lower or equal h), so we can ignore this one
                    continue;
                }
                // Store state hash for future comparisons
                hash_set.insert(s_hash);

                // Passed last corner, we're done!
                if (s.next_corner == scenario.corners.size()) {
                    load_naive_state(s);
                    break;
                }

                // Some debug info, very primitive
                game.hours = s.h;
                game.minutes = 0;
                game.seconds = 0;
                game.frames = int(hash_set.size() * 3 / 10);

                bool can_flip = (s.player.onground > 0 && s.game.gravitycontrol == 0 || s.player.onroof > 0 && s.game.gravitycontrol == 1);
                int max = can_flip ? 8 : 4;
                for (int8_t i = 0; i < max; i++) {
                    bool left = i & 1;
                    bool right = i & 2;
                    bool flip = i & 4;

                    load_naive_state(s);
                    key.clearKeys();
                    key.setKey(KEYBOARD_LEFT, left);
                    key.setKey(KEYBOARD_RIGHT, right);
                    key.setKey(KEYBOARD_v, flip);
                    do_game_step(i == 0 && hash_set.size() % 10000 == 0);
                    //do_game_step(true);

                    // TODO when is dying useful and how can we tell
                    // Oops we died, skip this branch then
                    if (game.deathseq > 0) {
                        continue;
                    }

                    naivestate new_state = create_naive_state();
                    new_state.f_count = s.f_count + 1;
                    new_state.num_l_plus_r = s.num_l_plus_r + (left && right ? 1 : 0);
                    new_state.next_corner = s.next_corner;
                    if (passed_next_corner(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y)) {
                        new_state.next_corner++;
                    }
                    if (new_state.game.roomx != s.game.roomx || new_state.game.roomy != s.game.roomy) {
                        new_state.num_frames_in_room = 0;
                    }
                    else {
                        new_state.num_frames_in_room = s.num_frames_in_room + 1;
                    }

                    new_state.h = new_state.f_count + get_heuristic(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y);
                    if (new_state.h < s.h) {
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    q.push(new_state);

                    // Remember where we came from to reconstruct the solution later
                    std::size_t new_state_hash = hash_naivestate(new_state);
                    if (prev_state_map.find(new_state_hash) == prev_state_map.end()) {
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(new_state_hash),
                            std::forward_as_tuple(s_hash, i));
                    }
                }
            }
        } // end q scope

        std::vector<std::size_t> state_hashes;
        std::vector<int> inputs;
        std::size_t hash = hash_naivestate(s);
        while (hash != initial_hash) {
            state_hashes.push_back(hash);
            if (prev_state_map.find(hash) == prev_state_map.end()) {
                VVV_exit(69); // Shouldn't happen
            }

            stateinfo i = prev_state_map.at(hash);
            inputs.push_back(i.input);
            hash = i.prev_hash;
        }

        // Reverse order of inputs
        std::reverse(inputs.begin(), inputs.end());

        state_hashes.clear();
        hash_set.clear();
        prev_state_map.clear();

        // Now play back the run a few times at different speeds
        int t = 0;
        while (true) {
            load_naive_state(initial_state);
            game.hours = 0;
            for (int idx = 0; idx < inputs.size(); idx++) {
                int8_t i = inputs[idx];

                bool left = i & 1;
                bool right = i & 2;
                bool flip = i & 4;

                game.hours++;
                game.minutes = left;
                game.seconds = right;
                game.frames = flip;

                key.clearKeys();
                key.setKey(KEYBOARD_LEFT, left);
                key.setKey(KEYBOARD_RIGHT, right);
                key.setKey(KEYBOARD_v, flip);
                do_game_step(true);
                SDL_Delay((1 << t) * 34);
            }
            t = (t + 1) % 4;
        }
    }

    void cached_stateful_solver() {
        load_scenario();
        cachednaivestate initial_state = create_cached_naivestate();
        initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y);

        std::size_t initial_hash = hash_cached_naivestate(initial_state);

        std::unordered_set<std::size_t, modified_hash> hash_set;
        std::unordered_map<std::size_t, stateinfo, modified_hash> prev_state_map;
        cachednaivestate s;
        { // start q scope
            std::priority_queue<cachednaivestate, std::vector<cachednaivestate>, std::function<bool(cachednaivestate, cachednaivestate)>> q(compare_cached_naivestates);

            q.push(initial_state);

            while (q.size() > 0) {
                // TODO: can make s a ptr
                s = q.top();
                q.pop();

                // TODO: not rehash. we can probably just store the hash in the struct?
                // Calculate state hash. If we have already seen the same hash, we can skip this branch.
                std::size_t s_hash = hash_cached_naivestate(s);
                if (hash_set.find(s_hash) != hash_set.end()) {
                    // State already exists (with lower or equal h), so we can ignore this one
                    continue;
                }
                // Store state hash for future comparisons
                hash_set.insert(s_hash);

                // Passed last corner, we're done!
                if (s.next_corner == scenario.corners.size()) {
                    load_cached_naivestate(s);
                    break;
                }

                // Some debug info, very primitive
                game.hours = s.h;
                game.minutes = 0;
                game.seconds = 0;
                game.frames = int(hash_set.size() * 3 / 10);

                game.deathcounts = int(entity_set_cache.size());

                // bool has_control = (s.game.hascontrol && s.game.deathseq == -1 && s.game.lifeseq <= 5);
                // TODO: does this work for line clips? I think so
                bool can_flip = (s.player.onground > 0 && s.game.gravitycontrol == 0 || s.player.onroof > 0 && s.game.gravitycontrol == 1);
                int max = can_flip ? 8 : 4;
                for (int8_t i = 0; i < max; i++) {
                    bool left = i & 1;
                    bool right = i & 2;
                    bool flip = i & 4;

                    load_cached_naivestate(s);
                    key.clearKeys();
                    key.setKey(KEYBOARD_LEFT, left);
                    key.setKey(KEYBOARD_RIGHT, right);
                    key.setKey(KEYBOARD_v, flip);
                    do_game_step(i == 0 && hash_set.size() % 10000 == 0);
                    // do_game_step(true); SDL_Delay(34);
                    // do_game_step(true);

                    // TODO when is dying useful and how can we tell
                    // Oops we died, skip this branch then
                    if (game.deathseq > 0) {
                        continue;
                    }

                    cachednaivestate new_state = create_cached_naivestate();
                    new_state.f_count = s.f_count + 1;
                    new_state.num_l_plus_r = s.num_l_plus_r + (left && right ? 1 : 0);
                    new_state.next_corner = s.next_corner;
                    if (passed_next_corner(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y)) {
                        new_state.next_corner++;
                    }
                    if (new_state.game.roomx != s.game.roomx || new_state.game.roomy != s.game.roomy) {
                        new_state.num_frames_in_room = 0;
                    }
                    else {
                        new_state.num_frames_in_room = s.num_frames_in_room + 1;
                    }

                    new_state.h = new_state.f_count + get_heuristic(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y);
                    if (new_state.h < s.h) {
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    q.push(new_state);

                    // Remember where we came from to reconstruct the solution later
                    std::size_t new_state_hash = hash_cached_naivestate(new_state);
                    if (prev_state_map.find(new_state_hash) == prev_state_map.end()) {
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(new_state_hash),
                            std::forward_as_tuple(s_hash, i));
                    }
                }
            }
        } // end q scope

        std::vector<std::size_t> state_hashes;
        std::vector<int> inputs;
        std::size_t hash = hash_cached_naivestate(s);
        while (hash != initial_hash) {
            state_hashes.push_back(hash);
            if (prev_state_map.find(hash) == prev_state_map.end()) {
                VVV_exit(69); // Shouldn't happen
            }

            stateinfo i = prev_state_map.at(hash);
            inputs.push_back(i.input);
            hash = i.prev_hash;
        }

        // Reverse order of inputs
        std::reverse(inputs.begin(), inputs.end());

        int num_states = hash_set.size();

        state_hashes.clear();
        hash_set.clear();
        prev_state_map.clear();

        // Clear the entity and block caches
        load_cached_naivestate(initial_state);
        naivestate restore_point = create_naive_state();
        blockcache.clear();
        entitycache.clear();

        // Now play back the run a few times at different speeds
        int t = 0;
        while (true) {
            load_naive_state(restore_point);
            game.hours = 0;
            game.deathcounts = num_states;
            for (int idx = 0; idx < inputs.size(); idx++) {
                int8_t i = inputs[idx];

                bool left = i & 1;
                bool right = i & 2;
                bool flip = i & 4;

                game.hours++;
                game.minutes = left;
                game.seconds = right;
                game.frames = flip;

                key.clearKeys();
                key.setKey(KEYBOARD_LEFT, left);
                key.setKey(KEYBOARD_RIGHT, right);
                key.setKey(KEYBOARD_v, flip);
                do_game_step(true);
                SDL_Delay((1 << t) * 34);
            }
            t = (t + 1) % 4;
        }
    }
    
    void stateless_solver(bool debug_checks) {
        load_scenario();

        naivestate initial_state = create_naive_state();
        initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y);

        std::size_t initial_hash = hash_naivestate(initial_state);

        std::unordered_set<std::size_t, modified_hash> hash_set;
        std::unordered_map<std::size_t, stateinfo, modified_hash> prev_state_map;

        statehash s = statehash(initial_state.h, initial_hash, 0, 0);
        std::vector<int8_t> inputs;
        { // start q scope
            std::priority_queue<statehash, std::vector<statehash>, std::function<bool(statehash, statehash)>> q(compare_statehashes);

            q.push(s);
            // Uncomment this to skip

            std::size_t tmp_h;
            naivestate tmp_state;
            while (q.size() > 0) {
                // Take best unexplored state
                s = q.top();
                q.pop();

                if (hash_set.find(s.hash) != hash_set.end()) {
                    // State already exists (with lower or equal h), so we can ignore this one
                    continue;
                }
                // Store state hash for future comparisons
                hash_set.insert(s.hash);

                tmp_h = s.hash;
                // What inputs got us to this state
                while (tmp_h != initial_hash) {
                    if (prev_state_map.find(tmp_h) == prev_state_map.end()) {
                        VVV_exit(69000);
                    }

                    stateinfo i = prev_state_map.at(tmp_h);
                    inputs.push_back(i.input);
                    tmp_h = i.prev_hash;
                }
                // Reverse order
                // TODO: can we skip this? performance impact?
                std::reverse(inputs.begin(), inputs.end());
                if (s.next_corner == scenario.corners.size()) {
                    break;
                }

                // Recreate the state from inputs
                load_naive_state(initial_state);
                for (int8_t i : inputs) {
                    key.setKey(KEYBOARD_LEFT, i & 1);
                    key.setKey(KEYBOARD_RIGHT, i & 2);
                    key.setKey(KEYBOARD_v, i & 4);
                    do_game_step(false);
                    // do_game_step(true); // SDL_Delay(34);

                    // Debug consistency check, comment out if going for efficiency
                    if (debug_checks) {
                        naivestate temp = create_naive_state();
                        std::size_t temp_hash = hash_naivestate(temp);
                        if (prev_state_map.find(temp_hash) == prev_state_map.end()) {
                            VVV_exit(69001);
                        }
                        if (prev_state_map.at(temp_hash).input != i) {
                            VVV_exit(69002);
                        }
                    }
                }
                inputs.clear();
                
                // Some debug info, very primitive
                game.hours = s.heuristic;
                game.minutes = 0;
                game.seconds = 0;
                game.frames = int(hash_set.size() * 3 / 10);
                // Memory usage:
                game.deathcounts = int((q.size() / 1024 * sizeof(statehash) + prev_state_map.size() / 1024 * (sizeof(stateinfo) + sizeof(std::size_t)) + hash_set.size() / 1024 * (sizeof(std::size_t))) / 1024);

                game.hours = obj.entities[0].invis;

                naivestate restore_point = create_naive_state();
                bool can_flip = (restore_point.player.onground > 0 && restore_point.game.gravitycontrol == 0 || restore_point.player.onroof > 0 && restore_point.game.gravitycontrol == 1);
                int max = can_flip ? 8 : 4;

                if (debug_checks) {
                    if (s.hash != hash_naivestate(restore_point)) {
                        VVV_exit(69003);
                    }
                }

                for (int8_t i = 0; i < max; i++) {
                    if (i > 0) {
                        load_naive_state(restore_point);
                    }
                    key.setKey(KEYBOARD_LEFT, i & 1);
                    key.setKey(KEYBOARD_RIGHT, i & 2);
                    key.setKey(KEYBOARD_v, i & 4);
                    do_game_step(i == 0 && hash_set.size() % 1000 == 0);
                    // do_game_step(true);// SDL_Delay(34);

                    // Oops we died, skip this branch then
                    // TODO when is dying useful and how can we tell
                    if (game.deathseq > 0) {
                        continue;
                    }

                    tmp_state = create_naive_state();
                    tmp_state.f_count = s.f_count + 1;
                    tmp_state.next_corner = s.next_corner;
                    if (passed_next_corner(tmp_state.next_corner, tmp_state.game.roomx, tmp_state.game.roomy, tmp_state.player.x, tmp_state.player.y)) {
                        tmp_state.next_corner++;
                    }

                    tmp_state.h = tmp_state.f_count + get_heuristic(tmp_state.next_corner, tmp_state.game.roomx, tmp_state.game.roomy, tmp_state.player.x, tmp_state.player.y);
                    if (tmp_state.h < s.heuristic) {
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    tmp_h = hash_naivestate(tmp_state);
                    q.emplace(tmp_state.h, tmp_h, tmp_state.f_count, tmp_state.next_corner);

                    // Remember where we came from to reconstruct the solution later
                    if (prev_state_map.find(tmp_h) == prev_state_map.end()) {
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(tmp_h),
                            std::forward_as_tuple(s.hash, i));
                    }
                }

                // TODO stuff
            }
        } // end q scope

        // todo

        hash_set.clear();
        prev_state_map.clear();

        int t = 0;
        while (true) {
            load_naive_state(initial_state);
            game.hours = 0;
            for (int idx = 0; idx < inputs.size(); idx++) {
                int8_t i = inputs[idx];

                bool left = i & 1;
                bool right = i & 2;
                bool flip = i & 4;

                game.hours++;
                game.minutes = left;
                game.seconds = right;
                game.frames = flip;

                key.clearKeys();
                key.setKey(KEYBOARD_LEFT, left);
                key.setKey(KEYBOARD_RIGHT, right);
                key.setKey(KEYBOARD_v, flip);
                do_game_step(true);
                SDL_Delay((1 << t) * 34);
            }
            t = (t + 1) % 4;
        }
    }
    
    void debug_test() {
        key.actually_poll = true;
        load_scenario();
        naivestate tmp;
        int i = 0;
        while (true) {
            game.hours = int(obj.entities.size());
            do_game_step(true);
            SDL_Delay(102);
            i++;
            if (i == 30) {
                tmp = create_naive_state();
            }
            else if (i == 60) {
                load_naive_state(tmp);
                i = 0;
            }
        }
    }

    void debug_test_cached() {
        key.actually_poll = true;
        load_scenario();
        cachednaivestate tmp;
        int i = 0;
        while (true) {
            game.hours = int(block_set_cache.size());
            game.deathcounts = int(entity_set_cache.size());
            do_game_step(true);
            SDL_Delay(102);
            i++;
            if (i == 30) {
                tmp = create_cached_naivestate();
            }
            else if (i == 60) {
                load_cached_naivestate(tmp);
                i = 0;
            }
        }
    }

    void load_scenario() {
        game.savex = scenario.init_x;
        game.savey = scenario.init_y;
        game.saverx = scenario.init_rx;
        game.savery = scenario.init_ry;
        game.savegc = scenario.init_gravity;
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
        map.gotoroom(game.saverx, game.savery);
        map.initmapdata();

        graphics.fademode = FADE_NONE;
        game.jumppressed = false;

        // Hack: Advance frames to fix enemy cycles to right position
        for (int i = 0; i < scenario.frames_to_advance; i++) {
            key.clearKeys();
            do_game_step(true);
            SDL_Delay(34);
        }

        key.clearKeys();
    }

    naivestate create_naive_state() {
        naivestate s = naivestate();

        s.game.roomx = game.roomx;
        s.game.roomy = game.roomy;

        s.player.x = obj.entities[0].xp;
        s.player.y = obj.entities[0].yp;
        // TODO: approximate floats as 1-decimal fixed-point values
        s.player.vx = obj.entities[0].vx;
        s.player.vy = obj.entities[0].vy;
        s.player.ax = obj.entities[0].ax;
        s.player.ay = obj.entities[0].ay;

        // Any value less than 0 is equivalent to 0
        int8_t effective_onground = SDL_max(obj.entities[0].onground, 0);
        int8_t effective_onroof = SDL_max(obj.entities[0].onroof, 0);
        s.player.onground = effective_onground;
        s.player.onroof = effective_onroof;
        s.player.dir = obj.entities[0].dir;

        // s.player.rule = obj.entities[0].rule;
        s.player.tile = obj.entities[0].tile;

        // Any value less than 0 is equivalent to 0
        int8_t effective_framedelay = SDL_max(obj.entities[0].framedelay, 0);
        s.player.framedelay = effective_framedelay;
        s.player.drawframe = obj.entities[0].drawframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_visualonground = SDL_max(obj.entities[0].visualonground, 0);
        int8_t effective_visualonroof = SDL_max(obj.entities[0].visualonroof, 0);
        s.player.visualonground = effective_visualonground;
        s.player.visualonroof = effective_visualonroof;
        s.player.walkingframe = obj.entities[0].walkingframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_collisionframedelay = SDL_max(obj.entities[0].collisionframedelay, 0);
        s.player.collisiondrawframe = obj.entities[0].collisiondrawframe;
        s.player.collisionframedelay = effective_collisionframedelay;
        s.player.collisionwalkingframe = obj.entities[0].collisionwalkingframe;

        s.player.newxp = obj.entities[0].newxp;
        s.player.newyp = obj.entities[0].newyp;

        s.game.state = game.state;
        s.game.deathseq = game.deathseq;
        s.game.lifeseq = game.lifeseq;
        s.game.gravitycontrol = game.gravitycontrol;
        s.game.press_action = game.press_action;
        s.game.jumpheld = game.jumpheld;
        s.game.jumppressed = game.jumppressed;
        s.game.press_right = game.press_right;
        s.game.press_left = game.press_left;

        // Any value greater than 5 is equivalent to 5
        int8_t effective_tapright = SDL_min(game.tapright, 5);
        int8_t effective_tapleft = SDL_min(game.tapleft, 5);
        s.game.tapright = effective_tapright;
        s.game.tapleft = effective_tapleft;

        if (s.game.roomx == 113 && s.game.roomy == 104) {
            // Hack: don't save entities or blocks in Comms Relay
        }
        else if (s.game.roomx == 114 && (s.game.roomy == 103 || s.game.roomy == 104)) {
            // Hack: don't save entities or blocks in Atmospheric Filtering Unit or It's a Secret to Nobody
        }
        else {
            for (size_t i = 1; i < obj.entities.size(); i++) {
                naiveenemystate e = naiveenemystate();
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
                int8_t effective_statedelay = SDL_max(obj.entities[i].statedelay, 0);
                e.statedelay = effective_statedelay;

                e.tile = obj.entities[i].tile;
                // e.animate = obj.entities[i].animate;
                // Any value less than 0 is equivalent to 0
                int8_t effective_framedelay_e = SDL_max(obj.entities[i].framedelay, 0);
                e.framedelay = effective_framedelay_e;
                e.walkingframe = obj.entities[i].walkingframe;
                e.drawframe = obj.entities[i].drawframe;

                // TODO emplace
                s.entities.push_back(e);
            }

            for (size_t i = 0; i < obj.blocks.size(); i++) {
                naiveblockstate b = naiveblockstate();
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

                // TODO emplace
                s.blocks.push_back(b);
            }
        }

        for (int i = 0; i < 20; i++) {
            s.collect[i] = obj.collect[i];
        }

        // Init everything else with zeroes
        s.f_count = 0;
        s.h = 0.0;
        s.num_l_plus_r = 0;
        s.next_corner = 0;
        s.num_frames_in_room = 0;

        return s;
    }

    void load_naive_state(naivestate s) {
        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            obj.collect[i] = s.collect[i];
        }

        // Load room (this also deletes entities)
        gotoroom(s.game.roomx, s.game.roomy);

        // Load player data
        obj.entities[0].xp = s.player.x;
        obj.entities[0].yp = s.player.y;
        obj.entities[0].vx = s.player.vx;
        obj.entities[0].vy = s.player.vy;
        obj.entities[0].ax = s.player.ax;
        obj.entities[0].ay = s.player.ay;
        obj.entities[0].onground = s.player.onground;
        obj.entities[0].onroof = s.player.onroof;
        obj.entities[0].dir = s.player.dir;

        // obj.entities[0].rule = s.player.rule;
        obj.entities[0].rule = 0;
        obj.entities[0].tile = s.player.tile;
        obj.entities[0].framedelay = s.player.framedelay;
        obj.entities[0].drawframe = s.player.drawframe;
        obj.entities[0].visualonground = s.player.visualonground;
        obj.entities[0].visualonroof = s.player.visualonroof;
        obj.entities[0].walkingframe = s.player.walkingframe;
        obj.entities[0].collisiondrawframe = s.player.collisiondrawframe;
        obj.entities[0].collisionframedelay = s.player.collisionframedelay;
        obj.entities[0].collisionwalkingframe = s.player.collisionwalkingframe;

        obj.entities[0].newxp = s.player.newxp;
        obj.entities[0].newyp = s.player.newyp;

        // TODO: not these Hacky fixes
        obj.entities[0].invis = false;
        obj.entities[0].colour = 0;
        obj.entities[0].type = 0;
        game.hascontrol = true;

        // Load game data
        game.state = s.game.state;
        game.deathseq = s.game.deathseq;
        game.lifeseq = s.game.lifeseq;
        game.gravitycontrol = s.game.gravitycontrol;
        game.press_action = s.game.press_action;
        game.jumpheld = s.game.jumpheld;
        game.jumppressed = s.game.jumppressed;
        game.press_right = s.game.press_right;
        game.press_left = s.game.press_left;
        game.tapright = s.game.tapright;
        game.tapleft = s.game.tapleft;

        // Load entity data
        for (int i = 0; i < s.entities.size(); i++) {
            obj.entities[i + 1].type = s.entities[i].type;
            obj.entities[i + 1].rule = s.entities[i].rule;
            obj.entities[i + 1].xp = s.entities[i].x;
            obj.entities[i + 1].yp = s.entities[i].y;
            obj.entities[i + 1].vx = s.entities[i].vx;
            obj.entities[i + 1].vy = s.entities[i].vy;
            // obj.entities[i + 1].ax = s.entities[i].ax;
            // obj.entities[i + 1].ay = s.entities[i].ay;
            obj.entities[i + 1].behave = s.entities[i].behave;
            // obj.entities[i + 1].para = s.entities[i].para;
            obj.entities[i + 1].state = s.entities[i].state;
            obj.entities[i + 1].onwall = s.entities[i].onwall;
            obj.entities[i + 1].statedelay = s.entities[i].statedelay;

            obj.entities[i + 1].tile = s.entities[i].tile;
            // obj.entities[i + 1].animate = s.entities[i].animate;
            obj.entities[i + 1].framedelay = s.entities[i].framedelay;
            obj.entities[i + 1].walkingframe = s.entities[i].walkingframe;
            obj.entities[i + 1].drawframe = s.entities[i].drawframe;
        }

        // Load block data
        for (int i = 0; i < s.blocks.size(); i++) {
            obj.blocks[i].rect.x = s.blocks[i].rect_x;
            obj.blocks[i].rect.y = s.blocks[i].rect_y;
            obj.blocks[i].rect.w = s.blocks[i].rect_w;
            obj.blocks[i].rect.h = s.blocks[i].rect_h;
            obj.blocks[i].type = s.blocks[i].type;
            obj.blocks[i].trigger = s.blocks[i].trigger;
            obj.blocks[i].xp = s.blocks[i].rect_x;
            obj.blocks[i].yp = s.blocks[i].rect_y;
            // obj.blocks[i].wp = s.blocks[i].wp;
            // obj.blocks[i].hp = s.blocks[i].hp;
            // obj.blocks[i].script = s.blocks[i].script;
            // obj.blocks[i].prompt = s.blocks[i].prompt;
            // obj.blocks[i].r = s.blocks[i].r;
            // obj.blocks[i].g = s.blocks[i].g;
            // obj.blocks[i].b = s.blocks[i].b;
            // obj.blocks[i].activity_y = s.blocks[i].activity_y;
        }
    }

    cachednaivestate create_cached_naivestate() {
        cachednaivestate s = cachednaivestate();

        s.game.roomx = game.roomx;
        s.game.roomy = game.roomy;

        s.player.x = obj.entities[0].xp;
        s.player.y = obj.entities[0].yp;
        // TODO: approximate floats as 1-decimal fixed-point values
        s.player.vx = obj.entities[0].vx;
        s.player.vy = obj.entities[0].vy;
        s.player.ax = obj.entities[0].ax;
        s.player.ay = obj.entities[0].ay;

        // Any value less than 0 is equivalent to 0
        int8_t effective_onground = SDL_max(obj.entities[0].onground, 0);
        int8_t effective_onroof = SDL_max(obj.entities[0].onroof, 0);
        s.player.onground = effective_onground;
        s.player.onroof = effective_onroof;
        s.player.dir = obj.entities[0].dir;

        // s.player.rule = obj.entities[0].rule;
        s.player.tile = obj.entities[0].tile;

        // Any value less than 0 is equivalent to 0
        int8_t effective_framedelay = SDL_max(obj.entities[0].framedelay, 0);
        s.player.framedelay = effective_framedelay;
        s.player.drawframe = obj.entities[0].drawframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_visualonground = SDL_max(obj.entities[0].visualonground, 0);
        int8_t effective_visualonroof = SDL_max(obj.entities[0].visualonroof, 0);
        s.player.visualonground = effective_visualonground;
        s.player.visualonroof = effective_visualonroof;
        s.player.walkingframe = obj.entities[0].walkingframe;

        // Any value less than 0 is equivalent to 0
        int8_t effective_collisionframedelay = SDL_max(obj.entities[0].collisionframedelay, 0);
        s.player.collisiondrawframe = obj.entities[0].collisiondrawframe;
        s.player.collisionframedelay = effective_collisionframedelay;
        s.player.collisionwalkingframe = obj.entities[0].collisionwalkingframe;

        s.player.newxp = obj.entities[0].newxp;
        s.player.newyp = obj.entities[0].newyp;

        s.game.state = game.state;
        s.game.deathseq = game.deathseq;
        s.game.lifeseq = game.lifeseq;
        s.game.gravitycontrol = game.gravitycontrol;
        s.game.press_action = game.press_action;
        s.game.jumpheld = game.jumpheld;
        s.game.jumppressed = game.jumppressed;
        s.game.press_right = game.press_right;
        s.game.press_left = game.press_left;

        // Any value greater than 5 is equivalent to 5
        int8_t effective_tapright = SDL_min(game.tapright, 5);
        int8_t effective_tapleft = SDL_min(game.tapleft, 5);
        s.game.tapright = effective_tapright;
        s.game.tapleft = effective_tapleft;

        if (s.game.roomx == 113 && s.game.roomy == 104) {
            // Hack: don't save entities or blocks in Comms Relay
        }
        else if (s.game.roomx == 114 && (s.game.roomy == 103 || s.game.roomy == 104)) {
            // Hack: don't save entities or blocks in Atmospheric Filtering Unit or It's a Secret to Nobody
        }
        else {
            s.cache_entry = fill_cache_entry();
        }

        for (int i = 0; i < 20; i++) {
            s.collect |= obj.collect[i] << i;
        }

        // Init everything else with zeroes
        s.f_count = 0;
        s.h = 0.0;
        s.num_l_plus_r = 0;
        s.next_corner = 0;
        s.num_frames_in_room = 0;

        return s;
    }

    void load_cached_naivestate(cachednaivestate s) {
        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            obj.collect[i] = (s.collect >> i) & 1;
        }

        // Load room
        if (game.roomx != s.game.roomx || game.roomy != s.game.roomy) {
            // Only load if it's necessary. this might behave weirdly in rooms where sprites are deleted?
            gotoroom(s.game.roomx, s.game.roomy);
        }

        // Load player data
        obj.entities[0].xp = s.player.x;
        obj.entities[0].yp = s.player.y;
        obj.entities[0].vx = s.player.vx;
        obj.entities[0].vy = s.player.vy;
        obj.entities[0].ax = s.player.ax;
        obj.entities[0].ay = s.player.ay;
        obj.entities[0].onground = s.player.onground;
        obj.entities[0].onroof = s.player.onroof;
        obj.entities[0].dir = s.player.dir;

        // obj.entities[0].rule = s.player.rule;
        obj.entities[0].rule = 0;
        obj.entities[0].tile = s.player.tile;
        obj.entities[0].framedelay = s.player.framedelay;
        obj.entities[0].drawframe = s.player.drawframe;
        obj.entities[0].visualonground = s.player.visualonground;
        obj.entities[0].visualonroof = s.player.visualonroof;
        obj.entities[0].walkingframe = s.player.walkingframe;
        obj.entities[0].collisiondrawframe = s.player.collisiondrawframe;
        obj.entities[0].collisionframedelay = s.player.collisionframedelay;
        obj.entities[0].collisionwalkingframe = s.player.collisionwalkingframe;

        obj.entities[0].newxp = s.player.newxp;
        obj.entities[0].newyp = s.player.newyp;

        // TODO: not these Hacky fixes
        obj.entities[0].invis = false;
        obj.entities[0].colour = 0;
        obj.entities[0].type = 0;
        game.hascontrol = true;

        // Load game data
        game.state = s.game.state;
        game.deathseq = s.game.deathseq;
        game.lifeseq = s.game.lifeseq;
        game.gravitycontrol = s.game.gravitycontrol;
        game.press_action = s.game.press_action;
        game.jumpheld = s.game.jumpheld;
        game.jumppressed = s.game.jumppressed;
        game.press_right = s.game.press_right;
        game.press_left = s.game.press_left;
        game.tapright = s.game.tapright;
        game.tapleft = s.game.tapleft;

        // Load cached data
        load_cache_entry(s.cache_entry);
    }

    cacheentry fill_cache_entry() {
        std::vector<std::size_t> entities;
        entities.reserve(obj.entities.size());
        for (size_t i = 1; i < obj.entities.size(); i++) {
            // Skip deleted entities
            // TODO: is this safe? any unexpected side effects?
            if (obj.entities[i].invis && obj.entities[i].size == -1 && obj.entities[i].type == -1 && obj.entities[i].rule == -1 && !obj.entities[i].isplatform) {
                continue;
            }
            naiveenemystate e = naiveenemystate();
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

            entities.push_back(cache_entity(e));
        }

        std::vector<std::size_t> blocks;
        blocks.reserve(obj.blocks.size());
        for (size_t i = 0; i < obj.blocks.size(); i++) {
            // Skip deleted blocks
            // TODO: is this safe? any unexpected side effects?
            if (obj.blocks[i].wp == 0 && obj.blocks[i].hp == 0 && obj.blocks[i].rect.w == 0 && obj.blocks[i].rect.h == 0) {
                continue;
            }

            naiveblockstate b = naiveblockstate();
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

            blocks.push_back(cache_block(b));
        }

        std::size_t entity_set = cache_entity_set(entities);
        std::size_t block_set = cache_block_set(blocks);

        return cacheentry(entity_set, block_set);
    }

    void load_cache_entry(cacheentry cache_entry) {
        if (cache_entry.entity_set != 0) {
            std::vector<std::size_t> cached_entities = get_cached_entity_set(cache_entry.entity_set);
            // Load entity data
            for (int i = 0; i < cached_entities.size(); i++) {
                naiveenemystate e = get_cached_entity(cached_entities[i]);
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
            }
        }
        if (cache_entry.block_set != 0) {
            std::vector<std::size_t> cached_blocks = get_cached_block_set(cache_entry.block_set);
            // Load block data
            for (int i = 0; i < cached_blocks.size(); i++) {
                naiveblockstate b = get_cached_block(cached_blocks[i]);
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

    std::size_t cache_entity(naiveenemystate entity) {
        std::size_t entity_hash = hash_entity(entity);

        if (entitycache.find(entity_hash) == entitycache.end()) {
            entitycache.emplace(entity_hash, entity);
        }

        // TODO can we get away with a smaller id than std::size_t?
        return entity_hash;
    }

    naiveenemystate get_cached_entity(std::size_t hash) {
        if (entitycache.find(hash) == entitycache.end()) {
            VVV_exit(54321);
        }
        return entitycache.at(hash);
    }

    std::size_t cache_entity_set(std::vector<std::size_t> entities) {
        std::size_t combined_entity_hash = combine_hashes(0, entities.size());
        for (std::size_t e_hash : entities) {
            combined_entity_hash = combine_hashes(combined_entity_hash, e_hash);
        }

        if (entity_set_cache.find(combined_entity_hash) == entity_set_cache.end()) {
            entity_set_cache.emplace(combined_entity_hash, entities);
        }

        return combined_entity_hash;
    }

    std::vector<std::size_t> get_cached_entity_set(std::size_t hash) {
        if (entity_set_cache.find(hash) == entity_set_cache.end()) {
            VVV_exit(123348);
        }
        return entity_set_cache.at(hash);
    }

    std::size_t cache_block(naiveblockstate block) {
        std::size_t block_hash = hash_block(block);

        if (blockcache.find(block_hash) == blockcache.end()) {
            blockcache.emplace(block_hash, block);
        }

        // TODO can we get away with a smaller id than std::size_t?
        return block_hash;
    }

    naiveblockstate get_cached_block(std::size_t hash) {
        if (blockcache.find(hash) == blockcache.end()) {
            VVV_exit(54321);
        }
        return blockcache.at(hash);
    }

    std::size_t cache_block_set(std::vector<std::size_t> blocks) {
        std::size_t combined_block_hash = combine_hashes(0, blocks.size());
        for (std::size_t e_hash : blocks) {
            combined_block_hash = combine_hashes(combined_block_hash, e_hash);
        }

        if (block_set_cache.find(combined_block_hash) == block_set_cache.end()) {
            block_set_cache.emplace(combined_block_hash, blocks);
        }

        return combined_block_hash;
    }

    std::vector<std::size_t> get_cached_block_set(std::size_t hash) {
        if (block_set_cache.find(hash) == block_set_cache.end()) {
            VVV_exit(112312231);
        }
        return block_set_cache.at(hash);
    }

    void do_game_step(bool render) {
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
        if (render) {
            graphics.clear();
            graphics.set_render_target(graphics.gameTexture);
            gamerender();
            gameScreen.RenderPresent();
        }
    }

    bool compare_naive_states(naivestate a, naivestate b) {
        if (a.h == b.h) {
            if (a.f_count == b.f_count) {
                // Prioritize low l+r count (because it's ugly)
                return a.num_l_plus_r > b.num_l_plus_r;
                // TODO: maybe prioritize least "input changes", to find the "nicest" solution
            }
            // Prioritize high frame count (continue furthest branch -> reduces runtime, but not asymptotically)
            return a.f_count < b.f_count;
        }
        // Prioritize low heuristic -> fastest solution will be found first if heuristic is admissible
        return a.h > b.h;
    }

    bool compare_cached_naivestates(cachednaivestate a, cachednaivestate b) {
        if (a.h == b.h) {
            if (a.f_count == b.f_count) {
                // Prioritize low l+r count (because it's ugly)
                return a.num_l_plus_r > b.num_l_plus_r;
                // TODO: maybe prioritize least "input changes", to find the "nicest" solution
            }
            // Prioritize high frame count (continue furthest branch -> reduces runtime, but not asymptotically)
            return a.f_count < b.f_count;
        }
        // Prioritize low heuristic -> fastest solution will be found first if heuristic is admissible
        return a.h > b.h;
    }

    bool compare_statehashes(statehash a, statehash b) {
        if (a.heuristic == b.heuristic) {
            // Prioritize high frame count (continue furthest branch -> reduces runtime, but not asymptotically)
            return a.f_count < b.f_count;
        }
        // Prioritize low heuristic -> fastest solution will be found first if heuristic is admissible
        return a.heuristic > b.heuristic;
    }

    void gotoroom(int rx, int ry) {
        /* TODO fix this and make it fast
        game.roomx = rx;
        game.roomy = ry;
        obj.removeallblocks();
        map.loadlevel(game.roomx, game.roomy);
        */
        map.gotoroom(rx, ry);
    }

    // 0: No warps
    // 1: X warps
    // 2: Y warps
    // 3: Both warps
    const int wz_warp_map[] = {
        2,2,3,3,2,1,3,
        0,2,1,2,1,2,1,
        0,3,2,1,3,1,2,
        0,0,0,3,1,2,3,
    };
    int room_warps(int room_x, int room_y) {
        // Super Gravitron: 119, 108
        if (room_x >= 113 && room_x <= 119) {
            if (room_y >= 100 && room_y <= 103) {
                int i = room_x - 113 + (room_y - 100) * 7;
                return wz_warp_map[i];
            }
        }
        return 0;
    }
    bool room_warpx(int room_x, int room_y) {
        return room_warps(room_x, room_y) & 1;
    }
    bool room_warpy(int room_x, int room_y) {
        return room_warps(room_x, room_y) & 2;
    }

    uint16_t get_stupid_heuristic(int next_corner, int room_x, int room_y, int player_x, int player_y) {
        if (next_corner == scenario.corners.size()) {
            return 0;
        }
        int x_min = player_x;
        int x_max = player_x;
        int y_min = player_y;
        int y_max = player_y;
        int rx = room_x;
        int ry = room_y;

        int total_frames = 0;

        if (next_corner > 0 && scenario.corners[next_corner - 1].dir == WARP_TOKEN && passed_next_corner(next_corner - 1, room_x, room_y, player_x, player_y)) {
            // We are still touching the previous warp token and haven't warped yet
            // Advance by one frame
            total_frames++;
            x_min = player_x - X_SPEED;
            x_max = player_x + X_SPEED;
            y_min = player_y - Y_SPEED;
            y_max = player_y + Y_SPEED;
            if (room_x == 116 && room_y == 100) {
                // I Love You
                x_min = 0;
                x_max = 0;
                rx = 114;
                ry = 102;
            }
            else {
                // unsupported warp token
                VVV_exit(1231);
            }
        }

        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            corner c = scenario.corners[c_idx];
            if (c.rx != rx || c.ry != ry) {
                // not in same room, hard to handle warping
                VVV_exit(24);
            }


            int warps = room_warps(c.rx, c.ry);

            int min_x_warp = (warps & 1) ? -1 : 0;
            int max_x_warp = (warps & 1) ? 1 : 0;
            int min_y_warp = (warps & 1) ? -1 : 0;
            int max_y_warp = (warps & 1) ? 1 : 0;

            if (c.rx == 114 && c.ry == 102) {
                //TWIHTKY
                if (x_min > 112) {
                    // we are on right half, only warp up
                    min_x_warp = 0;
                    max_x_warp = 0;
                    min_y_warp = -1;
                    max_y_warp = 0;
                }
                else {
                    // we are on left half, warp left
                    min_x_warp = -1;
                    max_x_warp = -1;
                    min_y_warp = 0;
                    max_y_warp = 0;
                }
            }
            else if (c.rx == 116 && c.ry == 100) {
                // ILY
                // TODO: fix this for later
                min_x_warp = 0;
                max_x_warp = 0;
                min_y_warp = 0;
                max_y_warp = 0;
                if (x_min > 160) {
                    min_x_warp = 1;
                    max_x_warp = 1;
                    min_y_warp = 0;
                    max_y_warp = 0;
                }
            }

            int prev_x_min = x_min;
            int prev_x_max = x_max;
            int prev_y_min = y_min;
            int prev_y_max = y_max;
            int prev_rx = rx;
            int prev_ry = ry;

            int min_frame_count = 9999;
            x_min = 1000;
            x_max = -1000;
            y_min = 1000;
            y_max = -1000;

            for (int x_warp = min_x_warp; x_warp <= max_x_warp; x_warp++) {
                for (int y_warp = min_y_warp; y_warp <= max_y_warp; y_warp++) {
                    int px = prev_x_max - (320 * x_warp);
                    int py = prev_y_max - (232 * y_warp);

                    if (c.x < px) {
                        px = prev_x_min - (320 * x_warp);
                        if (c.x > px) {
                            px = c.x;
                        }
                    }
                    if (c.y < py) {
                        py = prev_y_min - (232 * y_warp);
                        if (c.y > py) {
                            py = c.y;
                        }
                    }

                    int dx = px - c.x;
                    int dy = py - c.y;

                    if (c.dir != TRINKET && c.dir != WARP_TOKEN) {
                        VVV_exit(25);
                    }

                    // Note that we touch the warp token / trinket in the range x [-17, 9], y [-22, 13]
                    if (dx < -17) {
                        dx += 17;
                    }
                    else if (dx > 9) {
                        dx -= 9;
                    }
                    else {
                        dx = 0;
                    }
                    if (dy < -22) {
                        dy += 22;
                    }
                    else if (dy > 13) {
                        dy -= 13;
                    }
                    else {
                        dy = 0;
                    }
                    // At least how many frames will it take to reach the trinket?
                    int x_frames = (SDL_abs(dx) + X_SPEED - 1) / X_SPEED;
                    int y_frames = (SDL_abs(dy) + Y_SPEED - 1) / Y_SPEED;
                    int frame_count = SDL_max(x_frames, y_frames);

                    if (c.dir == WARP_TOKEN) {
                        // What are the min and max positions reachable while still touching the warp token?
                        int next_x_min = SDL_max(px - frame_count * X_SPEED, c.x - 17);
                        int next_x_max = SDL_min(px + frame_count * X_SPEED, c.x + 9);

                        int next_y_min = SDL_max(py - frame_count * Y_SPEED, c.y - 22);
                        int next_y_max = SDL_min(py + frame_count * Y_SPEED, c.y + 13);

                        if (c_idx < scenario.corners.size() - 1) {
                            // Advance one frame
                            frame_count++;
                            next_x_min = next_x_min - X_SPEED;
                            next_x_max = next_x_max + X_SPEED;
                            next_y_min = next_y_min - Y_SPEED;
                            next_y_max = next_y_max + Y_SPEED;

                            // Find position after warp
                            if (prev_rx == 116 && prev_ry == 100) {
                                // I Love You
                                next_x_min = 0;
                                next_x_max = 0;
                                rx = 114;
                                ry = 102;
                            }
                            else {
                                // unsupported warp token
                                VVV_exit(1231);
                            }
                        }

                        // Imprecision here, but i guess it's ok
                        x_min = SDL_min(x_min, next_x_min);
                        x_max = SDL_max(x_max, next_x_max);
                        y_min = SDL_min(y_min, next_y_min);
                        y_max = SDL_max(y_max, next_y_max);
                    }
                    else {
                        VVV_exit(12381723);
                    }

                    min_frame_count = SDL_min(min_frame_count, frame_count);
                    // x_min = SDL_min(x_min, next_x_min);
                }
            }

            total_frames += min_frame_count;
        }
        return total_frames;
    }

    int room_adjusted_x(int rx, int x) {
        return rx * 320 + x;
    }
    int room_adjusted_y(int ry, int y) {
        return ry * 240 + y;
    }

    bool passed_next_corner(int next_corner, int room_x, int room_y, int player_x, int player_y) {
        int px = room_adjusted_x(room_x, player_x);
        int py = room_adjusted_y(room_y, player_y);

        corner c = scenario.corners[next_corner];
        int cx = room_adjusted_x(c.rx, c.x);
        int cy = room_adjusted_y(c.ry, c.y);

        // Subtract corner position from player position
        int x_d = px - cx;
        int y_d = py - cy;

        switch (c.dir) {
        case UP_LEFT:
        case LEFT_UP:
            return x_d <= 0 && y_d <= 0;
        case DOWN_LEFT:
        case LEFT_DOWN:
            return x_d <= 0 && y_d >= 0;
        case UP_RIGHT:
        case RIGHT_UP:
            return x_d >= 0 && y_d <= 0;
        case DOWN_RIGHT:
        case RIGHT_DOWN:
            return x_d >= 0 && y_d >= 0;
        case TRINKET:
            // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
            return x_d >= -17 && x_d <= 9 && y_d >= -22 && y_d <= 13;
        case WARP_TOKEN:
            // Note that we touch the warp token in the range x [-17, 9], y [-22, 13]
            return x_d >= -17 && x_d <= 9 && y_d >= -22 && y_d <= 13;
        }
        return false;
    }

    // TODO: test this somehow
    // TODO: or at least verify signs and stuff, easy to get wrong
    // TODO: refactor eventually
    // TODO: account for velocity (turning around takes time)
    // TODO: account for treadmills and moving platforms (higher max speed)
    // TODO: account for warping rooms (WZ, intermissions, final)
    // TODO: account for warp tokens (overworld, WZ)
    // TODO: account for differing room heights (232 if map.warpy)
    uint16_t get_heuristic(int next_corner, int room_x, int room_y, int player_x, int player_y) {
        int total_frames = 0;

        int min_x = room_adjusted_x(room_x, player_x);
        int max_x = min_x;
        int min_y = room_adjusted_y(room_y, player_y);
        int max_y = min_y;

        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            corner c = scenario.corners[c_idx];
            int cx = room_adjusted_x(c.rx, c.x);
            int cy = room_adjusted_y(c.ry, c.y);

            int px = max_x;
            int py = max_y;
            if (c.dir == UP_LEFT || c.dir == LEFT_UP || c.dir == LEFT_DOWN || c.dir == DOWN_LEFT) {
                // Going left, take smaller x
                px = min_x;
            }
            if (c.dir == UP_LEFT || c.dir == UP_RIGHT || c.dir == LEFT_UP || c.dir == RIGHT_UP) {
                // Going up, take smaller y
                py = min_y;
            }

            // Subtract corner position from player position
            int x_d = px - cx;
            int y_d = py - cy;

            int frame_count = 0;

            // Note that if we are "inside" the corner (e.g. x_d > 0 && y_d < 0 for UP_LEFT),
            //   then we just pretend we can walk through walls. The max corner cut distance constraint
            //   makes sure we don't completely wreck our heuristic
            // Note also that if we are already past the corner (i.e. x_d <= 0 for UP_LEFT),
            //   then we do nothing as we want to preserve min_x, max_x, min_y and max_y for the next corner
            switch (c.dir) {
            case UP_LEFT: // Limiting factor: leftwards movement
                if (x_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (x_d + X_SPEED - 1) / X_SPEED;
                    min_x = px - frame_count * X_SPEED;
                    // Can't cut more than 10 pixels past the corner
                    min_y = SDL_max(py - frame_count * Y_SPEED, cy - Y_SPEED);
                    // In case we want to hug the corner
                    max_x = cx;
                    max_y = SDL_max(py - frame_count * Y_SPEED, cy);
                }
                break;
            case UP_RIGHT: // Limiting factor: rightwards movement
                if (x_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (-x_d + X_SPEED - 1) / X_SPEED;
                    max_x = px + frame_count * X_SPEED;
                    // Can't cut more than 10 pixels past the corner
                    min_y = SDL_max(py - frame_count * Y_SPEED, cy - Y_SPEED);
                    // In case we want to hug the corner
                    min_x = cx;
                    max_y = SDL_max(py - frame_count * Y_SPEED, cy);
                }
                break;
            case LEFT_UP: // Limiting factor: upwards movement
                if (y_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (y_d + Y_SPEED - 1) / Y_SPEED;
                    // Can't cut more than 0 pixels past the corner
                    min_x = SDL_max(px - frame_count * X_SPEED, cx);
                    min_y = py - frame_count * Y_SPEED;
                    // In case we want to hug the corner
                    max_x = SDL_max(px - frame_count * X_SPEED, cx);
                    max_y = cy;
                }
                break;
            case LEFT_DOWN: // Limiting factor: downwards movement
                if (y_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (-y_d + Y_SPEED - 1) / Y_SPEED;
                    // Can't cut more than 0 pixels past the corner
                    min_x = SDL_max(px - frame_count * X_SPEED, cx);
                    max_y = py + frame_count * Y_SPEED;
                    // In case we want to hug the corner
                    max_x = SDL_max(px - frame_count * X_SPEED, cx);
                    min_y = cy;
                }
                break;
            case DOWN_LEFT:
                if (x_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (x_d + X_SPEED - 1) / X_SPEED;
                    min_x = px - frame_count * X_SPEED;
                    // Can't cut more than 10 pixels past the corner
                    max_y = SDL_min(py + frame_count * Y_SPEED, cy + Y_SPEED);
                    // In case we want to hug the corner
                    max_x = cx;
                    min_y = SDL_min(py + frame_count * Y_SPEED, cy);
                }
                break;
            case DOWN_RIGHT:
                if (x_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (-x_d + X_SPEED - 1) / X_SPEED;
                    max_x = px + frame_count * X_SPEED;
                    // Can't cut more than 10 pixels past the corner
                    max_y = SDL_min(py + frame_count * Y_SPEED, cy + Y_SPEED);
                    // In case we want to hug the corner
                    min_x = cx;
                    min_y = SDL_min(py + frame_count * Y_SPEED, cy);
                }
                break;
            case RIGHT_UP:
                if (y_d > 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (y_d + Y_SPEED - 1) / Y_SPEED;
                    // Can't cut more than 0 pixels past the corner
                    max_x = SDL_min(px + frame_count * X_SPEED, cx);
                    min_y = py - frame_count * Y_SPEED;
                    // In case we want to hug the corner
                    min_x = SDL_min(px + frame_count * X_SPEED, cx);
                    max_y = cy;
                }
                break;
            case RIGHT_DOWN:
                if (y_d < 0) {
                    // What distance can we cover until we pass the corner?
                    frame_count = (-y_d + Y_SPEED - 1) / Y_SPEED;
                    // Can't cut more than 0 pixels past the corner
                    max_x = SDL_min(px + frame_count * X_SPEED, cx);
                    max_y = py + frame_count * Y_SPEED;
                    // In case we want to hug the corner
                    min_x = SDL_min(px + frame_count * X_SPEED, cx);
                    min_y = cy;
                }
                break;
            case TRINKET:
                // Could be in any direction
                // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
                // Modify distances accordingly
                if (x_d < -17) {
                    x_d += 17;
                }
                else if (x_d > 9) {
                    x_d -= 9;
                }
                else {
                    x_d = 0;
                }
                if (y_d < -22) {
                    y_d += 22;
                }
                else if (y_d > 13) {
                    y_d -= 13;
                }
                else {
                    y_d = 0;
                }

                // At least how many frames will it take to reach the trinket?
                int x_frames = (SDL_abs(x_d) + X_SPEED - 1) / X_SPEED;
                int y_frames = (SDL_abs(y_d) + Y_SPEED - 1) / Y_SPEED;
                frame_count = SDL_max(x_frames, y_frames);

                // What are the min and max positions reachable while still collecting the trinket?
                min_x = SDL_max(px - frame_count * X_SPEED, cx - 17);
                max_x = SDL_min(px + frame_count * X_SPEED, cx + 9);

                min_y = SDL_max(py - frame_count * Y_SPEED, cy - 22);
                max_y = SDL_min(py + frame_count * Y_SPEED, cy + 13);
            }

            total_frames += frame_count;
        }

        return total_frames;
    }

    std::hash<bool> h_b;
    std::hash<int> h_i;
    std::hash<std::uint64_t> h_u;
    std::hash<float> h_f;

    std::size_t hash_naivestate(naivestate s) {
        std::size_t result = 0;

        result = combine_hashes(result, hash_gamestate(s.game));
        result = combine_hashes(result, hash_playerstate(s.player));
        if (s.game.roomx == 112 && s.game.roomy == 104 && s.player.y < 120) {
            // Hack: if we're in top half of Gantry and Dolly, ignore entities
        }
        else {
            result = combine_hashes(result, hash_entities(s.entities));
        }
        result = combine_hashes(result, hash_blocks(s.blocks));

        for (int i = 0; i < 20; i++) {
            result = combine_hashes(result, h_b(s.collect[i]));
        }

        return result;
    }

    std::size_t hash_cached_naivestate(cachednaivestate s) {
        std::size_t result = 0;

        result = combine_hashes(result, hash_gamestate(s.game));
        result = combine_hashes(result, hash_playerstate(s.player));
        if (s.game.roomx == 112 && s.game.roomy == 104 && s.player.y < 120) {
            // Hack: if we're in top half of Gantry and Dolly, ignore entities
        }
        else {
            // The cache entry is the hash
            result = combine_hashes(result, s.cache_entry.entity_set);
        }
        // Blocks hash
        result = combine_hashes(result, s.cache_entry.block_set);

        // Trinkets hash
        result = combine_hashes(result, h_b(s.collect));

        return result;
    }

    std::size_t hash_playerstate(naiveplayerstate p) {
        std::size_t result = 0;

        result = combine_hashes(result, h_i(p.x));
        result = combine_hashes(result, h_i(p.y));

        // TODO: do we need to account for floating point errors? (precision loss)
        result = combine_hashes(result, h_f(p.vx));
        result = combine_hashes(result, h_f(p.vy));

        // Should always be integers
        result = combine_hashes(result, h_i(p.ax));
        result = combine_hashes(result, h_i(p.ay));

        // Any value less than 0 is equivalent to 0
        int effective_onground = SDL_max(p.onground, 0);
        int effective_onroof = SDL_max(p.onroof, 0);
        result = combine_hashes(result, h_i(effective_onground));
        result = combine_hashes(result, h_i(effective_onroof));

        result = combine_hashes(result, h_b(p.dir));

        // result = combine_hashes(result, h_i(p.rule));
        result = combine_hashes(result, h_i(p.tile));

        // Any value less than 0 is equivalent to 0
        int effective_framedelay = SDL_max(p.framedelay, 0);
        result = combine_hashes(result, h_i(effective_framedelay));
        result = combine_hashes(result, h_i(p.drawframe));

        // Any value less than 0 is equivalent to 0
        int effective_visualonground = SDL_max(p.visualonground, 0);
        int effective_visualonroof = SDL_max(p.visualonroof, 0);
        result = combine_hashes(result, h_i(effective_visualonground));
        result = combine_hashes(result, h_i(effective_visualonroof));

        result = combine_hashes(result, h_i(p.walkingframe));
        result = combine_hashes(result, h_i(p.collisiondrawframe));

        // Any value less than 0 is equivalent to 0
        int effective_collisionframedelay = SDL_max(p.collisionframedelay, 0);
        result = combine_hashes(result, h_i(effective_collisionframedelay));
        result = combine_hashes(result, h_b(p.collisionwalkingframe));

        result = combine_hashes(result, h_f(p.newxp));
        result = combine_hashes(result, h_f(p.newyp));

        return result;
    }

    std::size_t hash_gamestate(naivegamestate g) {
        std::size_t result = 0;

        result = combine_hashes(result, h_i(g.state));
        result = combine_hashes(result, h_i(g.deathseq));
        result = combine_hashes(result, h_i(g.lifeseq));
        result = combine_hashes(result, h_b(g.gravitycontrol));
        result = combine_hashes(result, h_i(g.roomx));
        result = combine_hashes(result, h_i(g.roomy));
        result = combine_hashes(result, h_b(g.press_action));
        result = combine_hashes(result, h_i(g.jumppressed));
        result = combine_hashes(result, h_b(g.jumpheld));
        result = combine_hashes(result, h_b(g.press_right));
        result = combine_hashes(result, h_b(g.press_left));

        // Any value greater than 5 is equivalent to 5
        int effective_tapright = SDL_min(g.tapright, 5);
        int effective_tapleft = SDL_min(g.tapleft, 5);
        result = combine_hashes(result, h_i(effective_tapright));
        result = combine_hashes(result, h_i(effective_tapleft));

        return result;
    }

    std::size_t hash_blocks(std::vector<naiveblockstate> bs) {
        std::size_t result = 0;
        result = combine_hashes(result, h_i(bs.size()));

        for (int i = 0; i < bs.size(); i++) {
            result = combine_hashes(result, hash_block(bs[i]));
        }

        return result;
    }

    std::size_t hash_block(naiveblockstate b) {
        std::size_t b_hash = 0;

        std::uint64_t value = b.rect_x + 100;
        // b_hash = combine_hashes(b_hash, h_i(b.rect_x));
        value = (value << 10) | (b.rect_y + 100);
        // b_hash = combine_hashes(b_hash, h_i(b.rect_y));
        value = (value << 9) | b.rect_w;
        // b_hash = combine_hashes(b_hash, h_i(b.rect_w));
        value = (value << 9) | b.rect_h;
        // b_hash = combine_hashes(b_hash, h_i(b.rect_h));
        value = (value << 3) | b.type;
        // b_hash = combine_hashes(b_hash, h_i(b.type));
        value = (value << 12) | b.trigger;
        // b_hash = combine_hashes(b_hash, h_i(b.trigger));
        b_hash = combine_hashes(b_hash, h_u(value));

        // b_hash = combine_hashes(b_hash, h_i(b.xp));
        // b_hash = combine_hashes(b_hash, h_i(b.yp));
        // b_hash = combine_hashes(b_hash, h_i(b.wp));
        // b_hash = combine_hashes(b_hash, h_i(b.hp));

        // script, prompt

        // b_hash = combine_hashes(b_hash, h_i(b.r));
        // b_hash = combine_hashes(b_hash, h_i(b.g));
        // b_hash = combine_hashes(b_hash, h_i(b.b));
        // b_hash = combine_hashes(b_hash, h_i(b.activity_y));

        return b_hash;
    }

    std::size_t hash_entities(std::vector<naiveenemystate> es) {
        std::size_t result = 0;
        result = combine_hashes(result, h_i(es.size()));
        
        for (int i = 0; i < es.size(); i++) {
            if (es[i].rule == 3 && es[i].type == 13) {
                // Hack: Ignore terminals - TODO do we care about final level terminal probably not
                continue;
            }
            if (es[i].rule == 3 && es[i].type == 8) {
                // Hack: Ignore checkpoints - TODO what about death strats
                continue;
            }
            if (es[i].rule == 3 && es[i].type == 7) {
                // Hack: Ignore trinkets, they should be restored by setting obj.collect
                continue;
            }
            result = combine_hashes(result, hash_entity(es[i]));
        }

        return result;
    }

    std::size_t hash_entity(naiveenemystate e) {
        std::size_t e_hash = 0;
        std::uint64_t value = ((e.type + 1) << 4) | (e.rule + 1);
        // e_hash = combine_hashes(e_hash, h_i(e.type));   // 6.67 bits
        // e_hash = combine_hashes(e_hash, h_i(e.rule));   // 3 bits

        e_hash = combine_hashes(e_hash, h_i(e.x));   // 6.67 bits
        e_hash = combine_hashes(e_hash, h_i(e.y));   // 3 bits
        e_hash = combine_hashes(e_hash, h_f(e.vy));     // ???
        e_hash = combine_hashes(e_hash, h_f(e.vx));     // ???
            // e_hash = combine_hashes(e_hash, h_f(e.ax));     // ???
            // e_hash = combine_hashes(e_hash, h_f(e.ay));     // ???

        value = (value << 5) | (e.behave + 1);
        // e_hash = combine_hashes(e_hash, h_i(e.behave)); // 5 bits
            // e_hash = combine_hashes(e_hash, h_f(e.para));   
        value = (value << 3) | e.state;
        // e_hash = combine_hashes(e_hash, h_i(e.state));  // 3 bits
        value = (value << 2) | e.onwall;
        // e_hash = combine_hashes(e_hash, h_i(e.onwall)); // 2 bits

        // Any value less than 0 is equivalent to 0
        int effective_statedelay = SDL_max(e.statedelay, 0);
        value = (value << 7) | (effective_statedelay + 1);
        // e_hash = combine_hashes(e_hash, h_i(effective_statedelay));

        value = (value << 11) | e.tile;
        // e_hash = combine_hashes(e_hash, h_i(e.tile));
            // e_hash = combine_hashes(e_hash, h_i(e.animate));

        // Any value less than 0 is equivalent to 0
        int effective_framedelay = SDL_max(e.framedelay, 0);
        value = (value << 4) | effective_framedelay;
        // e_hash = combine_hashes(e_hash, h_i(effective_framedelay));
        value = (value << 4) | (e.walkingframe + 5);
        // e_hash = combine_hashes(e_hash, h_i(e.walkingframe));
        value = (value << 10) | (e.drawframe + 10);
        // e_hash = combine_hashes(e_hash, h_i(e.drawframe));

        e_hash = combine_hashes(e_hash, h_u(value));

        return e_hash;
    }

    std::size_t combine_hashes(std::size_t h1, std::size_t h2) {
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
}
#include "solver/Solver.h"

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
                const corner EXIT_4(113, 103, 18, 110, LEFT_DOWN);
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
                const corner PLATFORM(112, 104, 194, 70, UP_RIGHT);
                const corner EXIT_RIGHT(112, 104, 246, 65, RIGHT_UP);
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

            const Scenario START_TO_LINEAR_COLLIDER(113, 105, 200, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_2,
                });
            
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
                CORNERS::LINEAR_COLLIDER::EXIT_3,
                CORNERS::LINEAR_COLLIDER::EXIT_4,
                });

            const Scenario SECURITY_SWEEP(113, 103, -14, 161, 0, 0, {
                CORNERS::SECURITY_SWEEP::ENTRY_1,
                CORNERS::SECURITY_SWEEP::ENTRY_2,
                CORNERS::SECURITY_SWEEP::FLIP_DOWN,
                CORNERS::SECURITY_SWEEP::GO_LEFT,
                CORNERS::SECURITY_SWEEP::EXIT,
                });

            const Scenario LINEAR_COLLIDER_TO_SECURITY_SWEEP(114, 103, -14, 46, 1, 0, {
                CORNERS::LINEAR_COLLIDER::ENTRY_1,
                CORNERS::LINEAR_COLLIDER::ENTRY_2,
                CORNERS::LINEAR_COLLIDER::ENTRY_3,
                CORNERS::LINEAR_COLLIDER::ENTRY_4,
                CORNERS::LINEAR_COLLIDER::EXIT_1,
                CORNERS::LINEAR_COLLIDER::EXIT_2,
                CORNERS::LINEAR_COLLIDER::EXIT_3,
                CORNERS::LINEAR_COLLIDER::EXIT_4,
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
                CORNERS::GANTRY_AND_DOLLY::PLATFORM,
                CORNERS::GANTRY_AND_DOLLY::EXIT_RIGHT,
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

            const Scenario THE_YES_MEN(112, 104, 113, 185, 0, 40, {
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

            const Scenario STOP_AND_REFLECT_TO_VSTITCH(112, 105, 105, 134, 1, 48, {
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

            const corner QUICKSAND_STUPID_HALFWAY_CORNER(116, 106, 184, 95, TRINKET);
            const corner QUICKSAND_STUPID_CORNER(116, 106, 184, 121, TRINKET);
            const Scenario QUICKSAND_STUPID(116, 106, 80, 22, 1, 0, {
                QUICKSAND_STUPID_HALFWAY_CORNER
                });

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
            const Scenario TWIHTKY_STUPID_GLITCHLESS(116, 100, 40, 102, 1, 97, {
                CORNERS::I_LOVE_YOU::WARP_TOKEN,
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
            const Scenario TWIHTKY_STUPID_DEATHWARP(114, 102, 0, 150, 1, 4, {
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
        }
    }

    namespace LAB {
        namespace CORNERS {
            namespace GET_READY_TO_BOUNCE {
                const corner DROP_DOWN(102, 116, 210, 33, DOWN_RIGHT);
            }
            namespace ITS_PERFECTLY_SAFE {
                const corner START_LEFT(102, 117, 210, 22, LEFT_DOWN);
                const corner CONTINUE_LEFT(102, 117, 186, 46, LEFT_DOWN);
            }
            namespace RASCASSE {
                const corner FLIP_DOWN(101, 117, 62, 97, DOWN_LEFT);
                // TODO: maybe remove this phantom
                const corner PHANTOM_TOUCH_GROUND(101, 117, 62, 177, DOWN_LEFT);
                const corner GO_RIGHT(101, 117, 62, 126, RIGHT_DOWN);
                const corner LINE_SKIP_DOWN(101, 117, 206, 97, DOWN_LEFT);
                const corner LINE_SKIP_RIGHT(101, 117, 206, 97, RIGHT_DOWN);
                const corner EXIT(101, 117, 218, 177, DOWN_RIGHT);
            }
            namespace KEEP_GOING {
                const corner ENTRY(101, 118, 218, 14, LEFT_DOWN);
                const corner GO_LEFT(101, 118, 202, 78, LEFT_DOWN);
            }
            namespace SINGLE_SLIT_EXPERIMENT {
                const corner FLIP_UP(100, 118, 278, 89, UP_LEFT);
                const corner EXIT(100, 118, 70, 129, DOWN_RIGHT);
            }
            namespace DONT_FLIP_OUT {
                const corner GO_RIGHT(100, 119, 70, 86, RIGHT_DOWN);
            }
            namespace DOUBLE_SLIT_EXPERIMENT {
                const int rx = 102;
                const int ry = 119;
                const corner ENTRY_SPIKE_CORNER(rx, ry, 66, 70, UP_RIGHT);
                const corner ENTRY_DROP(rx, ry, 66, 121, DOWN_RIGHT);
                const corner TOP_SHAFT_CORNER(rx, ry, 94, 22, RIGHT_DOWN);
                const corner EXIT_CORNER(rx, ry, 238, 110, RIGHT_DOWN);
            }
            namespace YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER {
                const int rx = 102;
                const int ry = 118;
                const corner A(rx, ry, 95, 185, RIGHT_UP);
                const corner B(rx, ry, 250, 174, UP_RIGHT);
                const corner C(rx, ry, 250, 65, LEFT_UP);
                const corner D(rx, ry, 22, 46, UP_LEFT);

                const corner TRINKET(rx, ry, 16, 24, TRINKET);
            }
            namespace YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE {
                const int rx = 102;
                const int ry = 118;
                const corner A(rx, ry, 95, 185, DOWN_LEFT);
                const corner B(rx, ry, 250, 174, LEFT_DOWN);
                const corner C(rx, ry, 250, 65, DOWN_RIGHT);
                const corner D(rx, ry, 22, 46, RIGHT_DOWN);
            }
            namespace THREES_A_CROWD {
                const int rx = 104;
                const int ry = 119;
                const corner ENTRY_DROP(rx, ry, 10, 97, DOWN_RIGHT);
                const corner SPIKE_CUT(rx, ry, 30, 166, RIGHT_DOWN);
                const corner LINE_CLIP_CORNER(rx, ry, 58, 166, UP_RIGHT);
            }
            namespace HITTING_THE_APEX {
                const int rx = 104;
                const int ry = 118;
                const corner ENTRY_CUT(rx, ry, 94, 193, RIGHT_UP);
                const corner TURNAROUND_1(rx, ry, 194, 118, UP_RIGHT);
                const corner TURNAROUND_2(rx, ry, 194, 89, LEFT_UP);
            }
            namespace SQUARE_ROOT {
                const int rx = 103;
                const int ry = 118;
                const corner DROP_DOWN(rx, ry, 238, 89, DOWN_LEFT);
                const corner FALL_UP(rx, ry, 126, 86, UP_LEFT);
            }
            namespace THORNY_EXCHANGE {
                const int rx = 103;
                const int ry = 117;
                const corner GO_RIGHT(rx, ry, 126, 153, RIGHT_UP);
                const corner FALL_UP(rx, ry, 162, 46, UP_RIGHT);
            }
            namespace LETTER_G {
                const int rx = 103;
                const int ry = 116;
                const corner ENTRY_LEFT(rx, ry, 162, 153, LEFT_UP);
                const corner DROP_UP(rx, ry, 54, 110, UP_LEFT);
                const corner GO_RIGHT(rx, ry, 54, 65, RIGHT_UP);
                const corner SKIP_LINE(rx, ry, 278, 65, RIGHT_UP);
            }
            namespace FREE_YOUR_MIND {
                const int rx = 104;
                const int ry = 116;
                const corner DROP(rx, ry, 66, 65, DOWN_RIGHT);
            }
            namespace IN_A_SINGLE_BOUND {
                const int rx = 105;
                const int ry = 116;
                const corner A(rx, ry, 198, 49, RIGHT_UP);
                const corner B(rx, ry, 218, 49, DOWN_RIGHT);
            }
            namespace BARANI_BARANI {
                const int rx = 106;
                const int ry = 116;
                const corner A(rx, ry, 90, 177, DOWN_RIGHT);
                const corner B(rx, ry, 98, 185, DOWN_RIGHT);
            }
            namespace SAFETY_DANCE {
                const int rx = 106;
                const int ry = 117;
                const corner A(rx, ry, 198, 22, RIGHT_DOWN);
                const corner B(rx, ry, 206, 30, RIGHT_DOWN);
                const corner C(rx, ry, 226, 30, UP_RIGHT);
                const corner D(rx, ry, 234, 22, UP_RIGHT);
            }
            namespace ENTANGLEMENT_GENERATOR {
                const int rx = 107;
                const int ry = 115;
                const corner DROP_UP(rx, ry, 234, 174, UP_RIGHT);
                const corner GO_LEFT(rx, ry, 234, 137, LEFT_UP);
                const corner DROP_DOWN(rx, ry, 234, 137, DOWN_RIGHT);
                // Entering Garbage Room
                const corner GARBAGE_ENTER_A(rx, ry, 78, 38, UP_LEFT);
                const corner GARBAGE_ENTER_B(rx, ry, 50, 17, LEFT_UP);
                const corner GARBAGE_ENTER_C(rx, ry, 14, 17, DOWN_LEFT);
                const corner GARBAGE_ENTER_D(rx, ry, 10, 38, LEFT_DOWN);
                // Leaving Garbage Room
                const corner GARBAGE_LEAVE_A(rx, ry, 78, 38, RIGHT_DOWN);
                const corner GARBAGE_LEAVE_B(rx, ry, 50, 17, DOWN_LEFT);
                const corner GARBAGE_LEAVE_C(rx, ry, 14, 17, RIGHT_UP);
                const corner GARBAGE_LEAVE_D(rx, ry, 10, 38, UP_RIGHT);
            }
            namespace GARBAGE_ROOM_ONE_ENTER {
                const int rx = 106;
                const int ry = 115;
                // Entering
                const corner A(rx, ry, 198, 49, DOWN_LEFT);
                const corner B(rx, ry, 190, 57, DOWN_LEFT);
                const corner C(rx, ry, 186, 70, LEFT_DOWN);
                const corner D(rx, ry, 174, 105, DOWN_LEFT);
                const corner E(rx, ry, 170, 182, LEFT_DOWN);
                const corner F(rx, ry, 138, 198, LEFT_DOWN);
                const corner G(rx, ry, 102, 198, UP_LEFT);
                const corner H(rx, ry, 86, 182, UP_LEFT);
                const corner I(rx, ry, 66, 97, LEFT_UP);
                const corner J(rx, ry, 58, 89, LEFT_UP);
                const corner K(rx, ry, 22, 89, DOWN_LEFT);
            }
            namespace GARBAGE_ROOM_ONE_LEAVE {
                const int rx = 106;
                const int ry = 115;
                // Leaving
                const corner A(rx, ry, 198, 49, RIGHT_UP);
                const corner B(rx, ry, 190, 57, RIGHT_UP);
                const corner C(rx, ry, 186, 70, UP_RIGHT);
                const corner D(rx, ry, 174, 105, RIGHT_UP);
                const corner E(rx, ry, 170, 182, UP_RIGHT);
                const corner F(rx, ry, 138, 198, UP_RIGHT);
                const corner G(rx, ry, 102, 198, RIGHT_DOWN);
                const corner H(rx, ry, 86, 182, RIGHT_DOWN);
                const corner I(rx, ry, 66, 97, DOWN_RIGHT);
                const corner J(rx, ry, 58, 89, DOWN_RIGHT);
                const corner K(rx, ry, 22, 89, RIGHT_UP);
            }
            namespace GARBAGE_ROOM_TWO_ENTER {
                const int rx = 105;
                const int ry = 115;
                // Entering
                const corner A(rx, ry, 254, 121, DOWN_LEFT);
                const corner B(rx, ry, 246, 145, DOWN_LEFT);
                const corner C(rx, ry, 242, 190, LEFT_DOWN);
                const corner D(rx, ry, 198, 190, UP_LEFT);
                const corner E(rx, ry, 182, 150, UP_LEFT);
                const corner F(rx, ry, 182, 105, RIGHT_UP);
                const corner G(rx, ry, 202, 94, UP_RIGHT);
                const corner H(rx, ry, 210, 86, UP_RIGHT);
                const corner I(rx, ry, 210, 17, LEFT_UP);
                const corner J(rx, ry, 142, 17, DOWN_LEFT);
                const corner K(rx, ry, 134, 33, DOWN_LEFT);
                const corner L(rx, ry, 130, 118, LEFT_DOWN);
                const corner M(rx, ry, 118, 129, DOWN_LEFT);
                const corner N(rx, ry, 114, 166, LEFT_DOWN);
                const corner O(rx, ry, 110, 177, DOWN_LEFT);
                const corner P(rx, ry, 106, 198, LEFT_DOWN);
                const corner Q(rx, ry, 78, 198, UP_LEFT);
                const corner R(rx, ry, 74, 153, LEFT_UP);
                const corner S(rx, ry, 62, 134, UP_LEFT);
                const corner T(rx, ry, 58, 113, LEFT_UP);
                const corner U(rx, ry, 38, 102, UP_LEFT);
                const corner V(rx, ry, 22, 86, UP_LEFT);
                const corner W(rx, ry, 22, 41, RIGHT_UP);
                
                // Trinket
                const corner TRINKET(rx, ry, 72, 16, TRINKET);
            }
            namespace GARBAGE_ROOM_TWO_LEAVE {
                const int rx = 105;
                const int ry = 115;
                // Leaving
                const corner A(rx, ry, 254, 121, RIGHT_UP);
                const corner B(rx, ry, 246, 145, RIGHT_UP);
                const corner C(rx, ry, 242, 190, UP_RIGHT);
                const corner D(rx, ry, 198, 190, RIGHT_DOWN);
                const corner E(rx, ry, 182, 150, RIGHT_DOWN);
                const corner F(rx, ry, 182, 105, DOWN_LEFT);
                const corner G(rx, ry, 202, 94, LEFT_DOWN);
                const corner H(rx, ry, 210, 86, LEFT_DOWN);
                const corner I(rx, ry, 210, 17, DOWN_RIGHT);
                const corner J(rx, ry, 142, 17, RIGHT_UP);
                const corner K(rx, ry, 134, 33, RIGHT_UP);
                const corner L(rx, ry, 130, 118, UP_RIGHT);
                const corner M(rx, ry, 118, 129, RIGHT_UP);
                const corner N(rx, ry, 114, 166, UP_RIGHT);
                const corner O(rx, ry, 110, 177, RIGHT_UP);
                const corner P(rx, ry, 106, 198, UP_RIGHT);
                const corner Q(rx, ry, 78, 198, RIGHT_DOWN);
                const corner R(rx, ry, 74, 153, DOWN_RIGHT);
                const corner S(rx, ry, 62, 134, RIGHT_DOWN);
                const corner T(rx, ry, 58, 113, DOWN_RIGHT);
                const corner U(rx, ry, 38, 102, RIGHT_DOWN);
                const corner V(rx, ry, 22, 86, RIGHT_DOWN);
                const corner W(rx, ry, 22, 41, DOWN_LEFT);
            }
            namespace HEADY_HEIGHTS {
                const int rx = 107;
                const int ry = 116;
                const corner SHAFT_LEFT_SPIKE(rx, ry, 206, 113, RIGHT_UP);
                const corner SHAFT_ENTRY(rx, ry, 234, 113, DOWN_RIGHT);
            }
            namespace TANTALIZING_TRINKET {
                const int rx = 107;
                const int ry = 118;
                const corner ENTER(rx, ry, 138, 65, LEFT_UP);
                const corner TRINKET(rx, ry, 32, 64, TRINKET);
                const corner LEAVE(rx, ry, 138, 65, DOWN_RIGHT);
            }
            namespace BERNOULLI_PRINCIPLE {
                const int rx = 107;
                const int ry = 119;
                const corner START_RIGHT(rx, ry, 246, 70, RIGHT_DOWN);
                const corner TURNAROUND_1(rx, ry, 274, 89, DOWN_RIGHT);
                const corner TURNAROUND_2(rx, ry, 274, 118, LEFT_DOWN);
                const corner EXIT_DROP(rx, ry, 246, 137, DOWN_LEFT);

                const corner TRINKET_ENTER_A(rx, ry, 138, 201, LEFT_UP);
                const corner TRINKET_ENTER_B(rx, ry, 118, 134, UP_LEFT);
                const corner TRINKET_ENTER_C(rx, ry, 118, 73, RIGHT_UP);
                const corner TRINKET_ENTER_D(rx, ry, 138, 6, UP_RIGHT);

                const corner TRINKET_LEAVE_A(rx, ry, 138, 201, DOWN_RIGHT);
                const corner TRINKET_LEAVE_B(rx, ry, 118, 134, RIGHT_DOWN);
                const corner TRINKET_LEAVE_C(rx, ry, 118, 73, DOWN_LEFT);
                const corner TRINKET_LEAVE_D(rx, ry, 138, 6, LEFT_DOWN);
            }
            namespace STANDING_WAVE {
                const int rx = 107;
                const int ry = 100;
                const corner START_LEFT(rx, ry, 234, 70, LEFT_DOWN);
                const corner FLIP_UP(rx, ry, 198, 70, UP_LEFT);
                const corner SHAFT_LEFT(rx, ry, 138, 70, LEFT_DOWN);
            }
            namespace SPIKE_STRIP_DEPLOYED {
                const int rx = 105;
                const int ry = 100;
                const corner SPIKE_CORNER(rx, ry, 190, 62, UP_LEFT);
                const corner LAST_SPIKE_CORNER(rx, ry, 106, 145, LEFT_UP);
            }
            namespace MERGE {
                const int rx = 103;
                const int ry = 100;
                const corner TOP_RIGHT(rx, ry, 170, 94, LEFT_DOWN);
                const corner BOTTOM_RIGHT(rx, ry, 170, 113, LEFT_UP);
                const corner TOP_LEFT(rx, ry, 134, 94, UP_LEFT);
                const corner BOTTOM_LEFT(rx, ry, 134, 113, DOWN_LEFT);
            }
            namespace IM_SORRY {
                const int rx = 101;
                const int ry = 100;
                const corner CHECKPOINT_DROP(rx, ry, 222, 94, UP_LEFT);
                const corner SPIKE_RIGHT(rx, ry, 106, 57, LEFT_UP);
                const corner SPIKE_LEFT(rx, ry, 86, 57, DOWN_LEFT);

                const corner UNOB_ENTER_A(rx, ry, 158, 153, RIGHT_UP);
                const corner UNOB_ENTER_B(rx, ry, 178, 153, DOWN_RIGHT);
                const corner UNOB_LEAVE_A(rx, ry, 158, 153, DOWN_LEFT);
                const corner UNOB_LEAVE_B(rx, ry, 178, 153, LEFT_UP);
            }
            namespace PLEASE_FORGIVE_ME {
                const int rx = 101;
                const int ry = 101;
                const corner FIRST_SPIKE_LEFT(rx, ry, 86, 102, RIGHT_DOWN);
                const corner FIRST_SPIKE_RIGHT(rx, ry, 106, 102, UP_RIGHT);
                const corner LAST_SPIKE_CORNER(rx, ry, 158, 102, RIGHT_DOWN);
                const corner LAST_SPIKE_RIGHT(rx, ry, 178, 102, UP_RIGHT);
                const corner EXIT_CORNER(rx, ry, 238, 89, RIGHT_UP);
            }
            namespace ANOMALY {
                const int rx = 105;
                const int ry = 101;
                const corner UNOB_ENTER_A(rx, ry, 50, 9, DOWN_RIGHT);
                const corner UNOB_ENTER_B(rx, ry, 234, 17, DOWN_RIGHT);
                const corner UNOB_ENTER_C(rx, ry, 298, 41, DOWN_RIGHT);

                const corner UNOB_LEAVE_A(rx, ry, 50, 9, LEFT_UP);
                const corner UNOB_LEAVE_B(rx, ry, 234, 17, LEFT_UP);
                const corner UNOB_LEAVE_C(rx, ry, 298, 41, LEFT_UP);
            }
            namespace PUREST_UNOBTAINIUM {
                const int rx = 106;
                const int ry = 101;
                const corner TRINKET(rx, ry, 104, 128, TRINKET);
            }
            namespace PLAYING_FOOSBALL {
                const int rx = 102;
                const int ry = 101;
                const corner ENTRY_DROP(rx, ry, 74, 89, DOWN_RIGHT);
            }
            namespace LIVING_DEAD_END {
                const int rx = 104;
                const int ry = 101;
                const corner FIRST_LINE_DROP(rx, ry, 66, 113, DOWN_RIGHT);
                const corner SECOND_LINE_DROP(rx, ry, 130, 161, DOWN_RIGHT);
            }
            namespace DIODE {
                const int rx = 104;
                const int ry = 103;
                const corner LEFT_ENTRY_SPIKE(rx, ry, 130, 54, LEFT_DOWN);
                const corner LEFT_TURNAROUND_1(rx, ry, 86, 81, DOWN_LEFT);
                const corner LEFT_TURNAROUND_2(rx, ry, 86, 126, RIGHT_DOWN);
                const corner LEFT_EXIT_SHAFT(rx, ry, 138, 185, DOWN_RIGHT);
                const corner RIGHT_ENTRY_SPIKE(rx, ry, 174, 54, RIGHT_DOWN);
                const corner RIGHT_TURNAROUND_1(rx, ry, 218, 81, DOWN_RIGHT);
                const corner RIGHT_TURNAROUND_2(rx, ry, 218, 126, LEFT_DOWN);
                const corner RIGHT_EXIT_SHAFT(rx, ry, 166, 185, DOWN_LEFT);
            }
            namespace I_SMELL_OZONE {
                const int rx = 104;
                const int ry = 104;
                const corner CORNER_CUT(rx, ry, 138, 158, LEFT_DOWN);
            }
        }
        namespace SCENARIOS {
            const Scenario IL_START_TO_RASCASSE(102, 116, 191, 33, 0, 0, {
                CORNERS::GET_READY_TO_BOUNCE::DROP_DOWN,
                CORNERS::ITS_PERFECTLY_SAFE::START_LEFT,
                CORNERS::ITS_PERFECTLY_SAFE::CONTINUE_LEFT,
                CORNERS::RASCASSE::FLIP_DOWN,
                CORNERS::RASCASSE::PHANTOM_TOUCH_GROUND,
            });

            const Scenario IL_START_TO_DONT_FLIP_OUT(102, 116, 191, 33, 0, 0, {
                CORNERS::GET_READY_TO_BOUNCE::DROP_DOWN,
                CORNERS::ITS_PERFECTLY_SAFE::START_LEFT,
                CORNERS::ITS_PERFECTLY_SAFE::CONTINUE_LEFT,
                CORNERS::RASCASSE::FLIP_DOWN,
                CORNERS::RASCASSE::GO_RIGHT,
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::EXIT,
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
            });

            const Scenario IL_START_TO_DONT_FLIP_OUT_IGNORE_LINE(102, 116, 191, 33, 0, 0, {
                CORNERS::GET_READY_TO_BOUNCE::DROP_DOWN,
                CORNERS::ITS_PERFECTLY_SAFE::START_LEFT,
                CORNERS::ITS_PERFECTLY_SAFE::CONTINUE_LEFT,
                CORNERS::RASCASSE::LINE_SKIP_DOWN,
                CORNERS::RASCASSE::LINE_SKIP_RIGHT,
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::EXIT,
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
            });

            const Scenario RASCASSE_TO_SINGLE_SLIT(101, 117, 158, 177, 0, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::FLIP_UP,
            });

            const Scenario RASCASSE_TO_DONT_FLIP_OUT(101, 117, 158, 177, 0, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::EXIT,
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
            });

            const Scenario RASCASSE_IGNORE_LINE(101, 117, 310, 46, 1, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
            });

            const Scenario DONT_FLIP_OUT_TO_LINECLIP(100, 119, 61, 21, 0, 0, {
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
                CORNERS::THREES_A_CROWD::ENTRY_DROP,
                CORNERS::THREES_A_CROWD::LINE_CLIP_CORNER,
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
            });

            const Scenario HITTING_THE_APEX_TO_LETTER_G(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::ENTRY_LEFT,
                CORNERS::LETTER_G::DROP_UP,
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
            });

            const Scenario HITTING_THE_APEX_TO_IN_A_SINGLE_BOUND(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::ENTRY_LEFT,
                CORNERS::LETTER_G::DROP_UP,
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                });

            const Scenario LETTER_G_IGNORE_LINE(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::SKIP_LINE,
                CORNERS::FREE_YOUR_MIND::DROP,
            });

            const Scenario LETTER_G_TO_IN_A_SINGLE_BOUND(103, 116, 39, 130, 1, 0, {
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                });

            const Scenario LETTER_G_TO_SAFETY_DANCE(103, 116, 39, 130, 1, 0, {
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                CORNERS::BARANI_BARANI::A,
                CORNERS::BARANI_BARANI::B,
                CORNERS::SAFETY_DANCE::A,
                CORNERS::SAFETY_DANCE::B,
                });

            // TODO: Search space too large, can't confirm this ties TAS
            // TODO: nearest flippable surface check?
            const Scenario LETTER_G_TO_ENTANGLEMENT_GENERATOR(103, 116, 39, 130, 1, 0, {
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                CORNERS::BARANI_BARANI::A,
                CORNERS::BARANI_BARANI::B,
                CORNERS::SAFETY_DANCE::A,
                CORNERS::SAFETY_DANCE::B,
                CORNERS::SAFETY_DANCE::C,
                CORNERS::SAFETY_DANCE::D,
                CORNERS::ENTANGLEMENT_GENERATOR::DROP_UP,
            });

            // TODO: Search space too large, can't confirm this ties TAS
            // TODO: nearest flippable surface check?
            const Scenario LETTER_G_TO_HEADY_HEIGHTS(103, 116, 39, 130, 1, 0, {
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                CORNERS::BARANI_BARANI::A,
                CORNERS::BARANI_BARANI::B,
                CORNERS::SAFETY_DANCE::A,
                CORNERS::SAFETY_DANCE::B,
                CORNERS::SAFETY_DANCE::C,
                CORNERS::SAFETY_DANCE::D,
                CORNERS::HEADY_HEIGHTS::SHAFT_LEFT_SPIKE,
                CORNERS::HEADY_HEIGHTS::SHAFT_ENTRY,
                CORNERS::BERNOULLI_PRINCIPLE::START_RIGHT,
            });

            const Scenario ENTANGLEMENT_GENERATOR_TO_BERNOULLI(107, 115, 233, 174, 1, 0, {
                CORNERS::HEADY_HEIGHTS::SHAFT_ENTRY,
                CORNERS::BERNOULLI_PRINCIPLE::START_RIGHT,
                CORNERS::BERNOULLI_PRINCIPLE::TURNAROUND_1,
                CORNERS::BERNOULLI_PRINCIPLE::TURNAROUND_2,
                CORNERS::BERNOULLI_PRINCIPLE::EXIT_DROP,
                // TODO: fix map/coord wrap-around problems
            });

            const Scenario STANDING_WAVE_TO_MERGE(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::MERGE::BOTTOM_LEFT,
            });

            const Scenario MERGE_TO_IM_SORRY(103, 100, 176, 131, 1, 0, {
                CORNERS::MERGE::TOP_LEFT,
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::SPIKE_RIGHT,
                CORNERS::IM_SORRY::SPIKE_LEFT,
            });

            const Scenario PLEASE_FORGIVE_ME_TO_LIVING_DEAD_END(101, 101, 141, 69, 0, 0, {
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
            });

            const Scenario PLEASE_FORGIVE_ME_TO_LEFT_SIDE_DIODE_TO_END(101, 101, 141, 69, 0, 0, {
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                CORNERS::DIODE::LEFT_ENTRY_SPIKE,
                CORNERS::DIODE::LEFT_TURNAROUND_1,
                CORNERS::DIODE::LEFT_TURNAROUND_2,
                CORNERS::DIODE::LEFT_EXIT_SHAFT,
                CORNERS::I_SMELL_OZONE::CORNER_CUT,
            });

            const Scenario PLEASE_FORGIVE_ME_TO_RIGHT_SIDE_DIODE_TO_END(101, 101, 141, 69, 0, 0, {
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                CORNERS::DIODE::RIGHT_ENTRY_SPIKE,
                CORNERS::DIODE::RIGHT_TURNAROUND_1,
                CORNERS::DIODE::RIGHT_TURNAROUND_2,
                CORNERS::DIODE::RIGHT_EXIT_SHAFT,
                CORNERS::I_SMELL_OZONE::CORNER_CUT,
            });

            const Scenario YOUNG_MAN_ITS_WORTH_THE_CHALLENGE(102, 119, 0, 121, 0, 0, {
                CORNERS::DOUBLE_SLIT_EXPERIMENT::ENTRY_SPIKE_CORNER,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::A,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::C,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::TRINKET,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::C,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::A,
                CORNERS::DOUBLE_SLIT_EXPERIMENT::TOP_SHAFT_CORNER,
                CORNERS::DOUBLE_SLIT_EXPERIMENT::EXIT_CORNER,
                CORNERS::THREES_A_CROWD::ENTRY_DROP,
                CORNERS::THREES_A_CROWD::SPIKE_CUT,
            });

            const Scenario GARBAGE_ROOM_TRINKET(107, 115, 233, 201, 1, 0, {
                CORNERS::ENTANGLEMENT_GENERATOR::DROP_UP,
                CORNERS::ENTANGLEMENT_GENERATOR::GO_LEFT,

                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_ENTER_A,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_ENTER_B,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_ENTER_C,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_ENTER_D,

                CORNERS::GARBAGE_ROOM_ONE_ENTER::A,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::B,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::C,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::D,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::E,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::F,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::G,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::H,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::I,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::J,
                CORNERS::GARBAGE_ROOM_ONE_ENTER::K,

                CORNERS::GARBAGE_ROOM_TWO_ENTER::A,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::B,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::C,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::D,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::E,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::F,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::G,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::H,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::I,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::J,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::K,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::L,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::M,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::N,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::O,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::P,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::Q,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::R,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::S,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::T,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::U,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::V,
                CORNERS::GARBAGE_ROOM_TWO_ENTER::W,

                CORNERS::GARBAGE_ROOM_TWO_ENTER::TRINKET,

                CORNERS::GARBAGE_ROOM_TWO_LEAVE::W,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::V,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::U,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::T,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::S,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::R,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::Q,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::P,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::O,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::N,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::M,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::L,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::K,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::J,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::I,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::H,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::G,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::F,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::E,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::D,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::C,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::B,
                CORNERS::GARBAGE_ROOM_TWO_LEAVE::A,

                CORNERS::GARBAGE_ROOM_ONE_LEAVE::K,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::J,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::I,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::H,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::G,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::F,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::E,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::D,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::C,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::B,
                CORNERS::GARBAGE_ROOM_ONE_LEAVE::A,

                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_LEAVE_D,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_LEAVE_C,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_LEAVE_B,
                CORNERS::ENTANGLEMENT_GENERATOR::GARBAGE_LEAVE_A,
                CORNERS::ENTANGLEMENT_GENERATOR::DROP_DOWN,

                CORNERS::BERNOULLI_PRINCIPLE::START_RIGHT,
            });

            const Scenario TANTALIZING_TRINKET(107, 100, 198, 137, 0, 0, {
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_ENTER_A,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_ENTER_B,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_ENTER_C,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_ENTER_D,

                CORNERS::TANTALIZING_TRINKET::ENTER,
                CORNERS::TANTALIZING_TRINKET::TRINKET,
                CORNERS::TANTALIZING_TRINKET::LEAVE,

                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_D,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_C,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_B,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_A,

                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                CORNERS::SPIKE_STRIP_DEPLOYED::SPIKE_CORNER,
            });

            const Scenario TANTALIZING_TRINKET_IGNORE_LINE(107, 100, 152, 137, 1, 5, {
                CORNERS::TANTALIZING_TRINKET::ENTER,
                CORNERS::TANTALIZING_TRINKET::TRINKET,
                CORNERS::TANTALIZING_TRINKET::LEAVE,
                CORNERS::STANDING_WAVE::SHAFT_LEFT,
            });

            const Scenario TANTALIZING_TRINKET_TO_MERGE(107, 118, 41, 65, 0, 5, {
                CORNERS::TANTALIZING_TRINKET::LEAVE,

                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_D,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_C,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_B,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_A,

                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                CORNERS::MERGE::TOP_RIGHT,
                CORNERS::MERGE::TOP_LEFT,
            });

            const Scenario TANTALIZING_TRINKET_TO_MERGE_IGNORE_LINE(107, 118, 41, 65, 0, 5, {
                CORNERS::TANTALIZING_TRINKET::LEAVE,

                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                CORNERS::MERGE::TOP_RIGHT,
                CORNERS::MERGE::TOP_LEFT,
            });

            const Scenario IM_SORRY_UNOBTAINIUM_LDE(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::SPIKE_RIGHT,
                CORNERS::IM_SORRY::SPIKE_LEFT,
                CORNERS::PLEASE_FORGIVE_ME::FIRST_SPIKE_LEFT,
                CORNERS::PLEASE_FORGIVE_ME::FIRST_SPIKE_RIGHT,
                CORNERS::IM_SORRY::UNOB_ENTER_A,
                CORNERS::IM_SORRY::UNOB_ENTER_B,

                CORNERS::ANOMALY::UNOB_ENTER_A,
                CORNERS::ANOMALY::UNOB_ENTER_B,
                CORNERS::ANOMALY::UNOB_ENTER_C,
                CORNERS::PUREST_UNOBTAINIUM::TRINKET,
                CORNERS::ANOMALY::UNOB_LEAVE_C,
                CORNERS::ANOMALY::UNOB_LEAVE_B,
                CORNERS::ANOMALY::UNOB_LEAVE_A,

                CORNERS::IM_SORRY::UNOB_LEAVE_B,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
            });

            // This is basically the limit of what is possible with the solver currently
            // Took >2h and all 32GB of my RAM to complete
            const Scenario STANDING_WAVE_TO_IM_SORRY(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::IM_SORRY::SPIKE_RIGHT,
                CORNERS::IM_SORRY::SPIKE_LEFT,
            });

            // TODO: run this (it's almost certainly too long though)
            const Scenario STANDING_WAVE_TO_PLEASE_FORGIVE_ME_IGNORE_LINES(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
            });

            const Scenario IM_SORRY_TO_PLEASE_FORGIVE_ME_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
            });

            const Scenario IM_SORRY_TO_LDE_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
            });

            const Scenario IM_SORRY_TO_LEFT_SIDE_DIODE_TO_END_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                CORNERS::DIODE::LEFT_ENTRY_SPIKE,
                CORNERS::DIODE::LEFT_TURNAROUND_1,
                CORNERS::DIODE::LEFT_TURNAROUND_2,
                CORNERS::DIODE::LEFT_EXIT_SHAFT,
                CORNERS::I_SMELL_OZONE::CORNER_CUT,
                });

            const Scenario IM_SORRY_TO_RIGHT_SIDE_DIODE_TO_END_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                CORNERS::DIODE::RIGHT_ENTRY_SPIKE,
                CORNERS::DIODE::RIGHT_TURNAROUND_1,
                CORNERS::DIODE::RIGHT_TURNAROUND_2,
                CORNERS::DIODE::RIGHT_EXIT_SHAFT,
                CORNERS::I_SMELL_OZONE::CORNER_CUT,
            });

            // TODO: run this (it's almost certainly too long though)
            // Didn't finish within 272 frames
            const Scenario STANDING_WAVE_TO_UNOBTAINIUM_IGNORE_LINES(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::PUREST_UNOBTAINIUM::TRINKET,
            });

            // TODO: could do hitting the apex to in a single bound
            // TODO: ideally we would check hitting the apex until entanglement generator, but that needs more optimization / a better heuristic (e.g. nearest flippable surface)
        }
    }

    // Current Benchmarks:
    // The Yes Men: 4630052 states visited, ~1:30 runtime (cached_stateful)
    // It's a Secret to Nobody: 1372903, 19s runtime
    static Scenario scenario = LAB::SCENARIOS::HITTING_THE_APEX_TO_IN_A_SINGLE_BOUND;
    // static Scenario scenario = SS1::SCENARIOS::START;
    // 0: Don't minimize inputs
    // 1: Minimize total number of presses / releases
    // 2: Minimize total number of frames held per button
    static int MIN_INPUT_MODE = 0;
    static bool CHECK_CONSISTENCY = false;
    // This only works for MIN_INPUT_MODE == 2 for now
    static bool SOLVE_MIN_FRAMES = false;

    // TODO: map.loadlevel caching
    // TODO: don't checkblocks for damage blocks
    // TODO: more hashing optimizations - use what we know about the data to make it faster (without introducing collisions)
    // TODO: is there some way of optimizing hitest?
    // TODO: nearest flippable surface check?
    // TODO: reduce room switching as much as possible
    // TODO: parallelize lol

    static std::unordered_map<std::size_t, naiveenemystate, modified_hash> entitycache;
    static std::unordered_map<std::size_t, naiveblockstate, modified_hash> blockcache;
    static std::unordered_map<std::size_t, std::vector<std::size_t>, modified_hash> entity_set_cache;
    static std::unordered_map<std::size_t, std::vector<std::size_t>, modified_hash> block_set_cache;

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
        // playback_ref();

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

            /*
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
            }*/

            /* Screen transition platform clipping proof of concept
            map.resetplayer();
            map.gotoroom(101, 116);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();

            game.gravitycontrol = 0;
            obj.entities[0].xp = 100;
            obj.entities[0].yp = 70;

            for (int i = 0; i < 300; i++) {
                key.clearKeys();
                if (i == 65 || i == 125) {
                    key.setKey(KEYBOARD_v, true);
                }
                obj.entities[0].dir = key.isDown(KEYBOARD_v);
                game.deathcounts = obj.entities[0].yp;
                do_game_step(true);
                SDL_Delay(34);
            }*/

            map.resetplayer();
            map.gotoroom(113, 112);
            map.initmapdata();

            graphics.fademode = FADE_NONE;
            game.jumppressed = false;

            key.clearKeys();

            game.gravitycontrol = 0;
            obj.entities[0].xp = 60;
            obj.entities[0].yp = 140;

            for (int i = 0; i < 60; i++) {
                key.clearKeys();
                if (i == 10) {
                    game.gravitycontrol = 1;
                    obj.entities[0].yp++;
                }
                game.deathcounts = obj.entities[0].yp;
                do_game_step(true);
                SDL_Delay(34);
            }
        }
    }

    void stateful_solver() {
        load_scenario();
        naivestate initial_state = create_naive_state();
        initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y, initial_state.game.gravitycontrol, initial_state.player.vx, initial_state.player.vy, initial_state.game.tapleft, initial_state.game.tapright);

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

                bool can_flip = (!game.jumpheld || game.jumppressed > 0) && (obj.entities[0].onground > 0 && game.gravitycontrol == 0 || obj.entities[0].onroof > 0 && game.gravitycontrol == 1);
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

                    new_state.h = new_state.f_count + get_heuristic(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                    if (new_state.h < s.h) {
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    q.push(new_state);

                    // Remove previous state_map entry if it's worse
                    std::size_t new_state_hash = hash_naivestate(new_state);
                    if (prev_state_map.find(new_state_hash) != prev_state_map.end()) {
                        uint16_t prev_heuristic = prev_state_map.at(new_state_hash).heuristic;
                        if (new_state.h < prev_heuristic) {
                            prev_state_map.erase(new_state_hash);
                        }
                    }
                    // Remember where we came from to reconstruct the solution later
                    if (prev_state_map.find(new_state_hash) == prev_state_map.end()) {
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(new_state_hash),
                            std::forward_as_tuple(s_hash, i, new_state.h));
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
        
        switch (MIN_INPUT_MODE) {
        case 0: 
            initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y, initial_state.game.gravitycontrol, initial_state.player.vx, initial_state.player.vy, initial_state.game.tapleft, initial_state.game.tapright);
            break;
        case 1:
            // TODO
            VVV_exit(2754);
            break;
        case 2:
            initial_state.h = get_input_frames_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y, initial_state.game.gravitycontrol, initial_state.player.vx, initial_state.player.vy, initial_state.game.tapleft, initial_state.game.tapright);
            break;
        }

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
                // TODO: should this use values read from `s` instead of directly reading the game?
                bool can_flip = (!game.jumpheld || game.jumppressed > 0) && (obj.entities[0].onground > 0 && game.gravitycontrol == 0 || obj.entities[0].onroof > 0 && game.gravitycontrol == 1);
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
                    } else {
                        new_state.num_frames_in_room = s.num_frames_in_room + 1;
                    }

                    switch (MIN_INPUT_MODE) {
                    case 0: break;
                    case 1: {
                        int16_t prev_count = s.input_count >> 3;
                        int8_t prev_inputs = int8_t(s.input_count & 0b111);
                        int8_t changed_inputs = prev_inputs ^ i;
                        int16_t num_inputs = (changed_inputs & 1) + ((changed_inputs >> 1) & 1) + ((changed_inputs >> 2) & 1);

                        new_state.input_count = ((prev_count + num_inputs) << 3) | i;
                        break;
                    }
                    case 2: {
                        new_state.input_count = s.input_count;
                        int16_t num_inputs = (int16_t)left + (int16_t)right + (int16_t)flip;
                        new_state.input_count += num_inputs;
                        break;
                    }
                    }

                    switch (MIN_INPUT_MODE) {
                    case 0:
                        new_state.h = new_state.f_count + get_heuristic(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                        break;
                    case 1:
                        // TODO
                        VVV_exit(2033);
                        break;
                    case 2:
                        new_state.h = new_state.input_count + get_input_frames_heuristic(new_state.next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                        break;
                    }

                    if (new_state.h < s.h) {
                        naivestate b = create_naive_state();
                        load_cached_naivestate(s);
                        naivestate a = create_naive_state();

                        int num_states = hash_set.size();

                        // Clear q by assigning a new empty queue
                        q = std::priority_queue<cachednaivestate, std::vector<cachednaivestate>, std::function<bool(cachednaivestate, cachednaivestate)>>(compare_cached_naivestates);
                        hash_set.clear();
                        prev_state_map.clear();

                        playback_inadmissibility(a, b);
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    // Add state to queue
                    q.push(new_state);

                    // Remove previous state_map entry if it's worse
                    std::size_t new_state_hash = hash_cached_naivestate(new_state);
                    if (prev_state_map.find(new_state_hash) != prev_state_map.end()) {
                        stateinfo& prev_info = prev_state_map.at(new_state_hash);
                        if (prev_info.heuristic > new_state.input_count) {
                            prev_info.heuristic = new_state.input_count;
                            prev_info.prev_hash = s_hash;
                        }
                    } else {
                        // Remember where we came from to reconstruct the solution later
                        // TODO: how can we guarantee this reconstructs the right solution later? if we find a worse path to the solution first
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(new_state_hash),
                            std::forward_as_tuple(s_hash, i, new_state.input_count));
                    }

                    // Consistency check
                    if (CHECK_CONSISTENCY) {
                        int num_dir_inputs = 0;
                        int num_flip_inputs = 0;
                        std::size_t hash = hash_cached_naivestate(new_state);
                        while (hash != initial_hash) {
                            if (prev_state_map.find(hash) == prev_state_map.end()) {
                                VVV_exit(69); // Shouldn't happen
                            }

                            stateinfo i = prev_state_map.at(hash);
                            int left = i.input & 1;
                            int right = (i.input >> 1) & 1;
                            int flip = (i.input >> 2) & 1;
                            num_dir_inputs += left + right;
                            num_flip_inputs += flip;
                            hash = i.prev_hash;
                        }

                        if (num_dir_inputs + num_flip_inputs > new_state.input_count) {
                            VVV_exit(27625);
                        }
                    }
                }
            }
            if (q.empty()) {
                VVV_exit(42069);
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

        // Now play back the run in a loop
        int t = 0;
        bool different_speeds = true;
        while (true) {
            int16_t total_inputs = 0;
            load_naive_state(restore_point);
            game.hours = 0;
            game.deathcounts = 0;
            int8_t prev_i = 0;
            for (int idx = 0; idx < inputs.size(); idx++) {
                int8_t i = inputs[idx];

                bool left = i & 1;
                bool right = i & 2;
                bool flip = i & 4;
                switch (MIN_INPUT_MODE) {
                case 0:
                    game.deathcounts = num_states;
                    break;
                case 1: {
                    int8_t changed_inputs = prev_i ^ i;
                    int16_t num_inputs = (changed_inputs & 1) + ((changed_inputs >> 1) & 1) + ((changed_inputs >> 2) & 1);
                    total_inputs += num_inputs;
                    game.deathcounts = total_inputs;
                    break;
                }
                case 2: {
                    int16_t num_inputs = (int16_t)left + (int16_t)right + (int16_t)flip;
                    total_inputs += num_inputs;
                    game.deathcounts = total_inputs;
                    break;
                }
                }
                prev_i = i;

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
            if (different_speeds) {
                t = 2 - t;
            }
        }
    }

    void playback_inadmissibility(naivestate& a, naivestate& b) {
        while (true) {
            load_naive_state(a);
            game.hours = a.h;
            do_game_render();
            uint16_t a_h = get_heuristic(a.next_corner, a.game.roomx, a.game.roomy, a.player.x, a.player.y, a.game.gravitycontrol, a.player.vx, a.player.vy, a.game.tapleft, a.game.tapright);
            a.h = a_h;
            SDL_Delay(170);

            load_naive_state(b);
            game.hours = b.h;
            do_game_render();
            uint16_t b_h = 1 + get_heuristic(b.next_corner, b.game.roomx, b.game.roomy, b.player.x, b.player.y, b.game.gravitycontrol, b.player.vx, b.player.vy, b.game.tapleft, b.game.tapright);
            b.h = b_h;
            SDL_Delay(170);
        }
    }

    void playback_ref() {
        load_scenario();
        cachednaivestate initial_state = create_cached_naivestate();
        // Example solution
        std::vector<int> inputs_ref;
        for (int c = 0; c < 2; c++) {
            inputs_ref.push_back(1); // L
            inputs_ref.push_back(1); // L
            inputs_ref.push_back(1); // L
            inputs_ref.push_back(1); // L
            inputs_ref.push_back(1); // L
            inputs_ref.push_back(0);
        }
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(2); // R
        inputs_ref.push_back(6); // R
        for (int c = 0; c < 3; c++) {
            inputs_ref.push_back(2); // R
            inputs_ref.push_back(2); // R
            inputs_ref.push_back(2); // R
            inputs_ref.push_back(2); // R
            inputs_ref.push_back(2); // R
            inputs_ref.push_back(0);
            inputs_ref.push_back(0);
            inputs_ref.push_back(0);
        }
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);
        inputs_ref.push_back(0);

        // Clear the entity and block caches
        load_cached_naivestate(initial_state);
        naivestate restore_point = create_naive_state();
        blockcache.clear();
        entitycache.clear();

        int ref_dir_input_count = 0;
        int ref_flip_input_count = 0;
        for (int idx = 0; idx < inputs_ref.size(); idx++) {
            int8_t i = inputs_ref[idx];

            bool left = i & 1;
            bool right = i & 2;
            bool flip = i & 4;

            ref_dir_input_count += (int16_t)left + (int16_t)right;
            ref_flip_input_count += flip ? 1 : 0;
        }

        // Now play back the run in a loop
        int t = 0;
        bool different_speeds = true;
        while (true) {
            int16_t total_dir_inputs = 0;
            int16_t total_flip_inputs = 0;
            load_naive_state(restore_point);
            game.hours = 0;
            game.deathcounts = 0;
            int8_t prev_i = 0;
            uint8_t next_corner = 0;
            for (int idx = 0; idx < inputs_ref.size(); idx++) {
                int8_t i = inputs_ref[idx];

                bool left = i & 1;
                bool right = i & 2;
                bool flip = i & 4;
                switch (MIN_INPUT_MODE) {
                case 0: break;
                case 1: {
                    int8_t changed_inputs = prev_i ^ i;
                    total_dir_inputs += (changed_inputs & 1) + ((changed_inputs >> 1) & 1);
                    total_flip_inputs += ((changed_inputs >> 2) & 1);
                    game.deathcounts = total_dir_inputs;
                    break;
                }
                case 2: {
                    // Check heuristic admissibility
                    cachednaivestate new_state = create_cached_naivestate();

                    if (passed_next_corner(next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y)) {
                        next_corner++;
                    }


                    int tmp_next_corner = next_corner;
                    if (!before_next_corner(next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y)) {
                        if (passed_next_corner(next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y)) {
                            // Sanity check, shouldn't happen
                            VVV_exit(29067);
                        }
                        // We can just solve the next corner directly without sacrificing accuracy
                        if (next_corner + 1 < scenario.corners.size() && scenario.corners[next_corner].dir != TRINKET) {
                            tmp_next_corner++;
                        }
                    }

                    uint16_t input_dir_h = total_dir_inputs + get_input_frames_heuristic_x(tmp_next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                    uint16_t input_flip_h = total_flip_inputs + get_input_frames_heuristic_y(tmp_next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);

                    if (input_dir_h > ref_dir_input_count) {
                        uint16_t input_h = total_dir_inputs + get_input_frames_heuristic_x(tmp_next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                        VVV_exit(12307);
                    }
                    if (input_flip_h > ref_flip_input_count) {
                        uint16_t input_h = total_flip_inputs + get_input_frames_heuristic_y(tmp_next_corner, new_state.game.roomx, new_state.game.roomy, new_state.player.x, new_state.player.y, new_state.game.gravitycontrol, new_state.player.vx, new_state.player.vy, new_state.game.tapleft, new_state.game.tapright);
                        VVV_exit(12308);
                    }

                    total_dir_inputs += (int16_t)left + (int16_t)right;
                    total_flip_inputs += (int16_t)flip;
                    game.deathcounts = input_dir_h + input_flip_h;
                    break;
                }
                }
                prev_i = i;

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
            if (different_speeds) {
                t = 2 - t;
            }
        }
    }
    
    void stateless_solver(bool debug_checks) {
        load_scenario();

        naivestate initial_state = create_naive_state();
        initial_state.h = get_heuristic(initial_state.next_corner, initial_state.game.roomx, initial_state.game.roomy, initial_state.player.x, initial_state.player.y, initial_state.game.gravitycontrol, initial_state.player.vx, initial_state.player.vy, initial_state.game.tapleft, initial_state.game.tapright);

        std::size_t initial_hash = hash_naivestate(initial_state);

        std::unordered_set<std::size_t, modified_hash> hash_set;
        std::unordered_map<std::size_t, stateinfo, modified_hash> prev_state_map;

        statehash s = statehash(initial_state.h, initial_hash, 0, 0);
        std::vector<int8_t> inputs;
        { // start q scope
            std::priority_queue<statehash, std::vector<statehash>, std::function<bool(statehash, statehash)>> q(compare_statehashes);

            q.push(s);
            // Uncomment this to skip

            std::size_t tmp_hash;
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

                tmp_hash = s.hash;
                // What inputs got us to this state
                while (tmp_hash != initial_hash) {
                    if (prev_state_map.find(tmp_hash) == prev_state_map.end()) {
                        VVV_exit(69000);
                    }

                    stateinfo i = prev_state_map.at(tmp_hash);
                    inputs.push_back(i.input);
                    tmp_hash = i.prev_hash;
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
                bool can_flip = (!restore_point.game.jumpheld || restore_point.game.jumppressed > 0) && (restore_point.player.onground > 0 && restore_point.game.gravitycontrol == 0 || restore_point.player.onroof > 0 && restore_point.game.gravitycontrol == 1);
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

                    tmp_state.h = tmp_state.f_count + get_heuristic(tmp_state.next_corner, tmp_state.game.roomx, tmp_state.game.roomy, tmp_state.player.x, tmp_state.player.y, tmp_state.game.gravitycontrol, tmp_state.player.vx, tmp_state.player.vy, tmp_state.game.tapleft, tmp_state.game.tapright);
                    if (tmp_state.h < s.heuristic) {
                        VVV_exit(69420); // Should hopefully not happen, means heuristic might be inadmissible
                    }

                    tmp_hash = hash_naivestate(tmp_state);
                    q.emplace(tmp_state.h, tmp_hash, tmp_state.f_count, tmp_state.next_corner);

                    // Remove previous state_map entry if it's worse
                    if (prev_state_map.find(tmp_hash) != prev_state_map.end()) {
                        uint16_t prev_heuristic = prev_state_map.at(tmp_hash).heuristic;
                        if (tmp_state.h < prev_heuristic) {
                            prev_state_map.erase(tmp_hash);
                        }
                    }
                    // Remember where we came from to reconstruct the solution later
                    if (prev_state_map.find(tmp_hash) == prev_state_map.end()) {
                        prev_state_map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(tmp_hash),
                            std::forward_as_tuple(s.hash, i, tmp_state.h));
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
            game.hours = int(obj.entities[0].xp);
            game.deathcounts = int(obj.entities[0].yp);
            do_game_step(true);
            SDL_Delay(34);
            /*
            i++;
            if (i == 30) {
                tmp = create_naive_state();
            }
            else if (i == 60) {
                load_naive_state(tmp);
                i = 0;
            }*/
        }
    }

    void debug_test_cached() {
        key.actually_poll = true;
        load_scenario();
        cachednaivestate tmp;
        int i = 0;
        while (true) {
            game.hours = int(block_set_cache.size());
            game.deathcounts = int(game.deathseq);
            game.hours = i;
            do_game_step(true);
            SDL_Delay(34);
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

                e.invis = obj.entities[i].invis;
                e.life = obj.entities[i].life;
                e.onentity = obj.entities[i].onentity;

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

    void load_naive_state(naivestate& s) {
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

            obj.entities[i + 1].invis = s.entities[i].invis;
            obj.entities[i + 1].life = s.entities[i].life;
            obj.entities[i + 1].onentity = s.entities[i].onentity;
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

    // TODO: what if we created the struct in-place, then only filled it here using ptrs? benchmark
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
        // else if (s.game.roomx == 114 && (s.game.roomy == 103 || s.game.roomy == 104)) {
            // Hack: don't save entities or blocks in Atmospheric Filtering Unit or It's a Secret to Nobody
        // }
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
        s.input_count = 0;

        return s;
    }

    void load_cached_naivestate(cachednaivestate& s) {
        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            // hacky fix pt1
            obj.collect[i] = false;
        }

        // Load room
        if (game.roomx != s.game.roomx || game.roomy != s.game.roomy) {
            // Only load if it's necessary. this might behave weirdly in rooms where sprites are deleted?
            gotoroom(s.game.roomx, s.game.roomy);
        }

        // Restore trinkets first, because they affect room load
        for (int i = 0; i < 20; i++) {
            // hacky fix pt2
            obj.collect[i] = (s.collect >> i) & 1;
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
            // -> Yes, this breaks trinket collecting
            /* if (obj.entities[i].invis && obj.entities[i].size == -1 && obj.entities[i].type == -1 && obj.entities[i].rule == -1 && !obj.entities[i].isplatform) {
                continue;
            } */
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

            e.invis = obj.entities[i].invis;
            e.life = obj.entities[i].life;
            e.onentity = obj.entities[i].onentity;

            entities.push_back(cache_entity(e));
        }

        std::vector<std::size_t> blocks;
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

                obj.entities[i + 1].invis = e.invis;
                obj.entities[i + 1].life = e.life;
                obj.entities[i + 1].onentity = e.onentity;
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
        for (std::size_t b_hash : blocks) {
            combined_block_hash = combine_hashes(combined_block_hash, b_hash);
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
            do_game_render();
        }
    }

    void do_game_render() {
        graphics.clear();
        graphics.set_render_target(graphics.gameTexture);
        gamerender();
        gameScreen.RenderPresent();
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
            if (SOLVE_MIN_FRAMES && MIN_INPUT_MODE != 0 && a.f_count != b.f_count) {
                // Prioritize low frame count for faster solutions
                return a.f_count > b.f_count;
            }

            int a_depth, b_depth;
            switch (MIN_INPUT_MODE) {
            case 0:
                a_depth = a.f_count;
                b_depth = b.f_count;
                break;
            case 2:
                a_depth = a.input_count;
                b_depth = b.input_count;
                break;
            case 1:
            default:
                // TODO
                a_depth = 0;
                b_depth = 0;
                VVV_exit(923);
                break;
            }
            if (a_depth == b_depth) {
                if (!SOLVE_MIN_FRAMES && MIN_INPUT_MODE != 0 && a.f_count != b.f_count) {
                    // Prioritize low frame count for faster solutions
                    return a.f_count > b.f_count;
                }
                // Prioritize low l+r count (because it's ugly)
                return a.num_l_plus_r > b.num_l_plus_r;
                // TODO: maybe prioritize least "input changes", to find the "nicest" solution
            }
            // Prioritize high depth (continue furthest branch -> reduces runtime, but not asymptotically)
            return a_depth < b_depth;
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
                    else if (c.dir == TRINKET) {
                        // What are the min and max positions reachable while still touching trinket?
                        int next_x_min = SDL_max(px - frame_count * X_SPEED, c.x - 17);
                        int next_x_max = SDL_min(px + frame_count * X_SPEED, c.x + 9);

                        int next_y_min = SDL_max(py - frame_count * Y_SPEED, c.y - 22);
                        int next_y_max = SDL_min(py + frame_count * Y_SPEED, c.y + 13);
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

    bool before_next_corner(int next_corner, int room_x, int room_y, int player_x, int player_y) {
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
        case DOWN_LEFT:
            return x_d > 0;
        case LEFT_UP:
        case RIGHT_UP:
            return y_d > 0;
        case LEFT_DOWN:
        case RIGHT_DOWN:
            return y_d < 0;
        case UP_RIGHT:
        case DOWN_RIGHT:
            return x_d < 0;
        case TRINKET:
            // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
            return !(x_d >= -17 && x_d <= 9 && y_d >= -22 && y_d <= 13);
        case WARP_TOKEN:
            // Note that we touch the warp token in the range x [-17, 9], y [-22, 13]
            return !(x_d >= -17 && x_d <= 9 && y_d >= -22 && y_d <= 13);
        }
        return true;
    }

    int get_coasted_dist(float vx, int rx, int px) {
        int sim_rx = rx;
        int sim_px = px;
        float sim_vx = vx;
        while (sim_vx != 0.0f) {
            if (sim_vx > 0.0f) {
                sim_vx -= 1.1f;
            }
            else if (sim_vx < 0.0f) {
                sim_vx += 1.1f;
            }
            if (SDL_fabsf(sim_vx) < 1.1f) {
                sim_vx = 0.0f;
            }
            sim_px += sim_vx;
            if (sim_px >= 308) {
                sim_px -= 320;
                sim_rx++;
            }
            else if (sim_px < -14) {
                sim_px += 320;
                sim_rx--;
            }
        }
        return room_adjusted_x(sim_rx, sim_px) - room_adjusted_x(rx, px);
    }

    uint16_t get_input_frames_heuristic(int next_corner_orig, int room_x, int room_y, int player_x, int player_y, int gravity, float vx, float vy, int tapleft, int tapright) {
        int total_inputs = 0;

        int next_corner = next_corner_orig;
        while (!before_next_corner(next_corner, room_x, room_y, player_x, player_y)) {
            if (passed_next_corner(next_corner, room_x, room_y, player_x, player_y)) {
                // Sanity check, shouldn't happen
                VVV_exit(29067);
            }
            // We can just solve the next corner directly without sacrificing accuracy
            if (next_corner + 1 < scenario.corners.size() && scenario.corners[next_corner].dir != TRINKET) {
                next_corner++;
            }
            else {
                break;
            }
        }

        total_inputs += get_input_frames_heuristic_x(next_corner, room_x, room_y, player_x, player_y, gravity, vx, vy, tapleft, tapright);
        total_inputs += get_input_frames_heuristic_y(next_corner, room_x, room_y, player_x, player_y, gravity, vx, vy, tapleft, tapright);
        return total_inputs;
    }

    uint16_t get_input_frames_heuristic_y(int next_corner, int room_x, int room_y, int player_x, int player_y, int gravity, float vx, float vy, int tapleft, int tapright) {
        // Solve y dimension
        int flip_inputs = 0;
        /*
        int dist = 0;
        int last_py = room_adjusted_y(room_y, player_y);
        int last_ry = room_y;
        float last_vy = vy;
        int last_cy = room_adjusted_y(room_y, player_y);
        int last_cry = room_y;
        bool last_gravity = gravity;
        bool isFirstSegment = true;
        for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            corner c = scenario.corners[c_idx];
            int cy = room_adjusted_y(c.ry, c.y);
            int c_ry = c.ry;

            int c_dist = cy - last_cy;
            if (c.dir == TRINKET) {
                // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
                if (c_dist < -13) {
                    cy += 13;
                    c_dist += 13;
                }
                else if (c_dist > 22) {
                    cy -= 22;
                    c_dist -= 22;
                }
                else {
                    cy -= c_dist;
                    c_dist = 0;
                }
                // TODO: we should technically check for room transitions here
            }


            if (dist == 0 && c_dist == 0) {
                if (c_idx > next_corner && c_idx + 1 < scenario.corners.size()) {
                    // Not quite sure what to do here yet, probably just continue
                    // VVV_exit(79023);
                }
                if ((c.dir == UP_LEFT || c.dir == UP_RIGHT) && last_gravity == 0) {
                    flip_inputs++;
                    last_gravity = !last_gravity;
                }
                else if ((c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT) && last_gravity == 1) {
                    flip_inputs++;
                    last_gravity = !last_gravity;
                }
                continue;
            }
            else if (dist == 0 || c_dist != 0 && ((dist < 0) == (c_dist < 0))) {
                // New segment or same Y direction
                dist += c_dist;
                last_cy = cy;
                last_cry = c_ry;
                if (c_idx + 1 < scenario.corners.size()) {
                    continue;
                }
                else {
                    // Pretend there's another corner after the last one
                    c_dist = 0;
                }
            }

            // Segment is finished, solve it
            if (last_cy - last_py != dist || dist == 0 || c_dist != 0) {
                // This can only happen if the corner sequence is ill-formed (i.e. dir change without intermediate 0 segment)
                // Usually, this happens if we pass a corner, then go back (or around a different corner)
                // Thus, it should only really happen on the first segment, or for trinkets
                corner c_prev = scenario.corners[c_idx - 1];
                if ((!isFirstSegment || c_idx <= next_corner) && c_prev.dir != TRINKET) {
                    VVV_exit(46728);
                }
                // We just want to run the previous corner a second time, let's pretend there were two of them
                cy = last_cy;
                c_ry = last_cry;
                c_idx--;
                c_dist = 0;
            }

            bool goingDown = dist > 0;
            int delta_y = SDL_abs(dist);
            float vyInDir = goingDown ? last_vy : -last_vy;
            bool oppGrav = last_gravity == goingDown;
            int goalGrav = -1;
            if (c.dir == UP_LEFT || c.dir == UP_RIGHT) {
                goalGrav = 1;
            }
            else if (c.dir == DOWN_LEFT || c.dir == DOWN_RIGHT) {
                goalGrav = 0;
            }

            if (isFirstSegment && oppGrav && vyInDir > 0.0) {
                // Can we reach the corner despite opposite gravity?
                // TODO
                VVV_exit(9382);
            }
            else if (oppGrav) {
                flip_inputs++;
                last_gravity = !last_gravity;
            }
            // We need to change gravity to pass the corner
            if (goalGrav != -1 && last_gravity != goalGrav) {
                flip_inputs++;
                last_gravity = !last_gravity;
            }

            last_py = cy;
            last_vy = 0.0;
            last_ry = c_ry;
            last_cy = cy;
            last_cry = c_ry;
            dist = 0;
            isFirstSegment = false;
        }*/
        return flip_inputs;
    }


    uint16_t get_input_frames_heuristic_x(int next_corner, int room_x, int room_y, int player_x, int player_y, int gravity, float vx, float vy, int tapleft, int tapright) {
        // Solve x dimension
        int dir_inputs = 0;
        int dist = 0;
        int last_px = room_adjusted_x(room_x, player_x);
        int last_rx = room_x;
        float last_vx = vx;
        int last_cx = room_adjusted_x(room_x, player_x);
        int last_crx = room_x;
        bool isFirstSegment = true;
        /*for (int c_idx = next_corner; c_idx < scenario.corners.size(); c_idx++) {
            corner c = scenario.corners[c_idx];
            int cx = room_adjusted_x(c.rx, c.x);
            int c_rx = c.rx;

            int c_dist = cx - last_cx;
            if (c.dir == TRINKET) {
                // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
                if (c_dist < -9) {
                    cx += 9;
                    c_dist += 9;
                }
                else if (c_dist > 17) {
                    cx -= 17;
                    c_dist -= 17;
                }
                else {
                    cx -= c_dist;
                    c_dist = 0;
                }
                // TODO: we should technically check for room transitions here, although you wouldn't be able to collect the trinket from another room
            }

            if (dist == 0 && c_dist == 0) {
                if (c_idx > next_corner && c_idx + 1 < scenario.corners.size()) {
                    // Not quite sure what to do here yet, probably just continue
                    // VVV_exit(79023);
                }
                continue;
            } else if (dist == 0 || c_dist != 0 && ((dist < 0) == (c_dist < 0))) {
                // New segment or same X direction
                dist += c_dist;
                last_cx = cx;
                last_crx = c_rx;
                if (c_idx + 1 < scenario.corners.size()) {
                    continue;
                }
                else {
                    // Pretend there's another corner after the last one
                    c_dist = 0;
                }
            }

            // Segment is finished, solve it
            if (last_cx - last_px != dist || dist == 0 || c_dist != 0) {
                // This can only happen if the corner sequence is ill-formed (i.e. dir change without intermediate 0 segment)
                // Usually, this happens if we pass a corner, then go back (or around a different corner)
                // Thus, it should only really happen on the first segment
                corner c_prev = scenario.corners[c_idx - 1];
                if ((!isFirstSegment || c_idx <= next_corner) && c_prev.dir != TRINKET) {
                    VVV_exit(46729);
                }
                // We just want to run the previous corner a second time, let's pretend there were two of them
                cx = last_cx;
                c_rx = last_crx;
                c_idx--;
                c_dist = 0;
            }
            bool goingRight = dist > 0;
            int delta_x = SDL_abs(dist);
            float vxInDir = goingRight ? last_vx : -last_vx;
            int tapInDir = goingRight ? tapright : tapleft;

            if (isFirstSegment && vxInDir > 0.0f && !(tapInDir > 0 && tapInDir < 5)) {
                // Check if we can just coast to the finish line
                int coast_dist = get_coasted_dist(last_vx, last_rx, (last_px - 320 * last_rx));
                if (SDL_abs(coast_dist) >= delta_x) {
                    // No inputs needed
                    last_px = cx;
                    last_vx = 0.0;
                    last_rx = c_rx;
                    last_cx = cx;
                    last_crx = c_rx;
                    dist = 0;
                    isFirstSegment = false;
                    continue;
                }
            }

            float minSpeedForMaxCoast = 5.5f;
            std::vector<SimState> possible_starts;
            {
                SimState state;
                state.input_count = 0;
                state.px = last_px - (last_rx * 320);
                state.rx = last_rx;
                state.vx = (vxInDir > 0.0) ? last_vx : 0.0;
                state.tap = 0;
                state.abs_dist = 0;
                if (isFirstSegment) {
                    // We may have nonzero vxInDir and tapInDir
                    state.tap = tapInDir;

                    // Try to proceed to a standard situation
                    while (state.tap > 0 && (state.tap < 5 || SDL_fabsf(state.vx) < minSpeedForMaxCoast) && state.abs_dist < delta_x) {
                        bool pressDir;
                        if (state.tap < 5) {
                            // It's always best to preserve tapInDir to profit off of coasting
                            pressDir = true;
                        }
                        else if (state.vx < 2.2f && state.tap < 5) {
                            // If we release our speed will be set to 0, so keep holding to preserve it
                            pressDir = true;
                        }
                        else {
                            // tapInDir >= 5 && vxInDir < 5.5f
                            // Whether or not we want to keep accelerating depends on the exact distance to the goal
                            pressDir = false;
                        }

                        if (!pressDir) {
                            // Next decision is ambiguous, add release case to possible starts
                            SimState new_state;
                            new_state.input_count = state.input_count;
                            new_state.tap = 0;
                            new_state.vx = state.vx + ((state.vx == 0.0) ? 0.0 : (goingRight ? (-1.1f) : 1.1f));
                            if (SDL_fabsf(new_state.vx) < 1.1f) {
                                new_state.vx = 0.0f;
                            }
                            new_state.px = state.px + new_state.vx;
                            new_state.abs_dist = state.abs_dist + SDL_abs(new_state.px - state.px);
                            new_state.rx = state.rx;
                            if (new_state.px >= 308) {
                                new_state.px -= 320;
                                new_state.rx++;
                            }
                            else if (new_state.px < -14) {
                                new_state.px += 320;
                                new_state.rx--;
                            }
                            possible_starts.push_back(new_state);
                        }

                        // Keeping dir pressed is guaranteed to be optimal
                        state.input_count++;
                        state.tap++;
                        if (goingRight) {
                            state.vx = SDL_min(6.0f, (state.vx + 3.0f) - 1.1f);
                        }
                        else {
                            state.vx = SDL_max(-6.0f, (state.vx - 3.0f) + 1.1f);
                        }
                        int prev_px = state.px;
                        state.px += state.vx;
                        state.abs_dist += SDL_abs(state.px - prev_px);
                        if (state.px >= 308) {
                            state.px -= 320;
                            state.rx++;
                        }
                        else if (state.px < -14) {
                            state.px += 320;
                            state.rx--;
                        }
                    }
                }
                possible_starts.push_back(state);
            }

            int best_result = INT_MAX;
            for (int start_idx = 0; start_idx < possible_starts.size(); start_idx++) {
                SimState state = possible_starts[start_idx];

                // Can we reach the goal with no inputs?
                int coast_dist = 0;
                if (!(state.tap > 0 && state.tap < 5)) {
                    coast_dist = SDL_abs(get_coasted_dist(state.vx, state.rx, state.px));
                }
                if (state.abs_dist + coast_dist >= delta_x) {
                    best_result = SDL_min(best_result, state.input_count);
                    continue;
                }
                // Sanity check, applicable if state not already past the goal
                if (state.tap > 0 && (state.tap < 5 || SDL_fabsf(state.vx) < minSpeedForMaxCoast)) {
                    VVV_exit(82373);
                }
                if (state.tap >= 5 && state.abs_dist + coast_dist + 4 * X_SPEED >= delta_x) {
                    // We can trivially reach the goal with 5 or less inputs
                    int num_inputs = (delta_x - state.abs_dist - coast_dist + X_SPEED - 1) / X_SPEED;
                    best_result = SDL_min(best_result, state.input_count + num_inputs);
                    continue;
                }
                
                if (state.tap == 0) {
                    // We know we can't coast to the finish line
                    // It turns out the optimal cycle start timing is the same as for starting final inputs if we do a partial cycle
                    // So just progress cycle starting inputs until we reach the goal or a standard cycle
                    while (state.abs_dist < delta_x && (state.tap < 5 || SDL_fabsf(state.vx) < minSpeedForMaxCoast)) {
                        bool pressDir = false;
                        if (state.tap >= 5) {
                            // This shouldn't be possible without reaching max speed
                            VVV_exit(17052);
                        }
                        else if (SDL_fabsf(state.vx) < 2.2f || state.tap > 0 && state.tap < 5) {
                            // Our speed will be 0 next frame unless we press the button
                            pressDir = true;
                        }
                        else if (SDL_fabsf(state.vx) >= 3.3f) {
                            // If we press now, the distance we travel in the frame after next will not improve by the maximum of 6 pixels
                            // This is because we will have speed 1.1 if we don't press now, so we travel at least a pixel regardless of direction
                            // But it's always possible to get 6 extra pixels on the second frame after pressing the directional input
                            pressDir = false;
                        }
                        else {
                            // This is the speed regime where rounding matters
                            // There are two possibilities - either we wait one frame, or we start pressing now
                            // We have to simulate two frames of pressed input to know which one is better

                            // Case 1: Release, hold, hold
                            int wait_px = state.px;
                            int wait_rx = state.rx;
                            float wait_vx = state.vx + (goingRight ? (-1.1f) : 1.1f);

                            wait_px += wait_vx;
                            if (wait_px >= 308) {
                                wait_px -= 320;
                                wait_rx++;
                            }
                            else if (wait_px < -14) {
                                wait_px += 320;
                                wait_rx--;
                            }
                            for (int i = 0; i < 2; i++) {
                                wait_vx = wait_vx + (goingRight ? 1.9f : -1.9f);
                                if (SDL_fabsf(wait_vx) >= 6.0f) {
                                    wait_vx = goingRight ? 6.0f : -6.0f;
                                }
                                wait_px += wait_vx;
                                if (wait_px >= 308) {
                                    wait_px -= 320;
                                    wait_rx++;
                                }
                                else if (wait_px < -14) {
                                    wait_px += 320;
                                    wait_rx--;
                                }
                            }
                            
                            // Case 2: hold, hold
                            int hold_px = state.px;
                            int hold_rx = state.rx;
                            float hold_vx = state.vx + (goingRight ? 1.9f : -1.9f);
                            // Note: hold_vx can be at most 5.19f here, so we don't need to cap it
                            // Sanity check:
                            if (SDL_fabsf(hold_vx) >= 6.0f) {
                                VVV_exit(56209);
                            }
                            hold_px += hold_vx;
                            if (hold_px >= 308) {
                                hold_px -= 320;
                                hold_rx++;
                            }
                            else if (hold_px < -14) {
                                hold_px += 320;
                                hold_rx--;
                            }
                            // Bonus check: does holding right reach the goal right away?
                            int hold_1_dist = SDL_abs(room_adjusted_x(hold_rx, hold_px) - room_adjusted_x(state.rx, state.px));

                            hold_vx = hold_vx + (goingRight ? 1.9f : -1.9f);
                            if (SDL_fabsf(hold_vx) >= 6.0f) {
                                hold_vx = goingRight ? 6.0f : -6.0f;
                            }
                            hold_px += hold_vx;
                            if (hold_px >= 308) {
                                hold_px -= 320;
                                hold_rx++;
                            }
                            else if (hold_px < -14) {
                                hold_px += 320;
                                hold_rx--;
                            }

                            int wait_abs_dist = SDL_abs(room_adjusted_x(wait_rx, wait_px) - room_adjusted_x(state.rx, state.px));
                            int hold_abs_dist = SDL_abs(room_adjusted_x(hold_rx, hold_px) - room_adjusted_x(state.rx, state.px));

                            if (hold_1_dist + state.abs_dist >= delta_x) {
                                pressDir = true;
                                // I think this actually shouldn't happen, or at least not without wait + hold also reaching the goal
                                // Because moving extra distance can only make rounding unfavorable, never the inverse
                                // So wait + hold will always be equal or better since the first hold input just adds 3px of distance without changing rounding
                            }
                            else {
                                // Hold if not worse since it uses fewer frames
                                pressDir = hold_abs_dist >= wait_abs_dist;
                            }
                        }

                        if (pressDir) {
                            state.vx += goingRight ? 3.0f : -3.0f;
                            state.input_count++;
                            state.tap++;
                        }
                        state.vx += goingRight ? -1.1f : 1.1f;
                        if (SDL_fabsf(state.vx) > 6.0f) {
                            state.vx = goingRight ? 6.0f : -6.0f;
                        }
                        else if (SDL_fabsf(state.vx) < 1.1f) {
                            state.vx = 0.0f;
                            // Sanity check, this shouldn't happen
                            // VVV_exit(198264);
                        }
                        int prev_px = state.px;
                        state.px += state.vx;
                        state.abs_dist += SDL_abs(state.px - prev_px);
                        if (state.px >= 308) {
                            state.px -= 320;
                            state.rx++;
                        }
                        else if (state.px < -14) {
                            state.px += 320;
                            state.rx--;
                        }
                    }
                }

                if (state.abs_dist >= delta_x) {
                    // We can reach the goal without further inputs
                    best_result = SDL_min(best_result, state.input_count);
                    continue;
                }
                else if (state.tap < 5 || SDL_fabsf(state.vx) < minSpeedForMaxCoast) {
                    // Sanity check
                    VVV_exit(17583);
                }
                // We are in a standard cycle start pos, i.e. tap >= 5 and vxInDir >= 5.5
                int delta_rx = SDL_abs(last_crx - state.rx);
                // Max amount of "unexpected" favorable rounding
                int max_bonus_pixels = goingRight ? 3 * delta_rx : 0;
                if (goingRight && state.px <= -5) {
                    if (state.px <= -14) {
                        max_bonus_pixels += 4;
                    }
                    else if (state.px <= -12) {
                        max_bonus_pixels += 3;
                    }
                    else if (state.px <= -9) {
                        max_bonus_pixels += 2;
                    }
                    else {
                        max_bonus_pixels += 1;
                    }
                }
                int cycleLen = goingRight ? 37 : 42;
                int cycleInputs = 5;
                // We ignore rounding for coasting distance because it's already captured by bonus pixels
                int coastDist = goingRight ? 10 : 14;

                int inputsSoFar = state.input_count;
                int remaining_distance = delta_x - state.abs_dist - max_bonus_pixels - coastDist;
                
                if (remaining_distance > 5 * X_SPEED) {
                    // Adding more 5-frame hold cycles is optimal
                    int num_cycles = remaining_distance / cycleLen;
                    remaining_distance %= cycleLen;
                    if (remaining_distance > 5 * X_SPEED) {
                        // Remaining distance can't be covered in 5 frames naively
                        // Add another cycle
                        num_cycles++;
                        remaining_distance -= cycleLen;
                    }
                    inputsSoFar += num_cycles * cycleInputs;
                }

                if (remaining_distance > 0) {
                    // Simply add some extra inputs right now to reach the goal by coasting
                    inputsSoFar += (remaining_distance + X_SPEED - 1) / X_SPEED;
                }

                // We can reach the goal without further inputs
                best_result = SDL_min(best_result, inputsSoFar);
            }

            dir_inputs += best_result;
            last_px = cx;
            last_vx = 0.0;
            last_rx = c_rx;
            last_cx = cx;
            last_crx = c_rx;
            dist = 0;
            isFirstSegment = false;
        }*/
        return dir_inputs;
    }

    // TODO: test this somehow
    // TODO: or at least verify signs and stuff, easy to get wrong
    // TODO: refactor eventually
    // TODO: account for velocity (turning around takes time)
    // TODO: account for treadmills and moving platforms (higher max speed)
    // TODO: account for warping rooms (WZ, intermissions, final)
    // TODO: account for warp tokens (overworld, WZ)
    // TODO: account for differing room heights (232 if map.warpy)
    // TODO: account for nearest flippable surface
    // TODO: maybe we can cache results starting from next_corner
    uint16_t get_heuristic(int next_corner_orig, int room_x, int room_y, int player_x, int player_y, int gravity, float vx, float vy, int tapleft, int tapright) {
        int total_frames = 0;
        int next_corner = next_corner_orig;
        bool can_flip = (!game.jumpheld || game.jumppressed > 0) && (obj.entities[0].onground > 0 && game.gravitycontrol == 0 || obj.entities[0].onroof > 0 && game.gravitycontrol == 1);
        bool can_double_flip = (!game.jumpheld || game.jumppressed > 0) && (obj.entities[0].onground > 0 && game.gravitycontrol == 0 && obj.entities[0].onroof > 0);

        // Extract first corner from the loop below to handle initial acceleration
        SimState state_ul = SimState(player_x, player_y, room_x, room_y, vx, vy, gravity, tapleft, tapright);
        SimState state_dr = SimState(player_x, player_y, room_x, room_y, vx, vy, gravity, tapleft, tapright);

        while (next_corner < scenario.corners.size()) {
            corner c = scenario.corners[next_corner];
            corner_dir c_dir = c.dir;
            int cx = room_adjusted_x(c.rx, c.x);
            int cy = room_adjusted_y(c.ry, c.y);

            int px_min = room_adjusted_x(state_ul.rx, state_ul.px);
            int px_max = room_adjusted_x(state_dr.rx, state_dr.px);
            int py_min = room_adjusted_y(state_ul.ry, state_ul.py);
            int py_max = room_adjusted_y(state_dr.ry, state_dr.py);

            bool trinket_or_warp = c_dir == TRINKET || c_dir == WARP_TOKEN;

            int cx_min = cx;
            int cx_max = cx;
            int cy_min = cy;
            int cy_max = cy;
            if (trinket_or_warp) {
                // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
                cx_min = cx - 17;
                cx_max = cx + 9;
                cy_min = cy - 22;
                cy_max = cy + 13;
            }

            // Subtract corner position from player position
            int x_d = px_min > cx_max ? (px_min - cx_max) : (px_max < cx_min ? (px_max - cx_min) : 0);
            int y_d = py_min > cy_max ? (py_min - cy_max) : (py_max < cy_min ? (py_max - cy_min) : 0);

            // Skip this corner if we may no longer be before it
            {
                bool is_before = true;
                bool is_inside = false;
                corner_dir inside_fix_dir = c_dir;
                switch (c_dir) {
                case UP_LEFT:
                    if (x_d <= 0) {
                        // Restrict x range to be not before the corner
                        px_max = SDL_max(px_max, cx);
                        is_before = false;
                    }
                    else if (y_d < 0) {
                        is_inside = true;
                        inside_fix_dir = LEFT_DOWN;
                    }
                    break;
                case DOWN_LEFT:
                    if (x_d <= 0) {
                        // Restrict x range to be not before the corner
                        px_max = SDL_max(px_max, cx);
                        is_before = false;
                    }
                    else if (y_d > 0) {
                        is_inside = true;
                        inside_fix_dir = LEFT_UP;
                    }
                    break;
                case LEFT_UP:
                    if (y_d <= 0) {
                        // Restrict y range to be not before the corner
                        py_max = cy;
                        is_before = false;
                    }
                    else if (x_d < 0) {
                        is_inside = true;
                        inside_fix_dir = UP_RIGHT;
                    }
                    break;
                case RIGHT_UP:
                    if (y_d <= 0) {
                        // Restrict y range to be not before the corner
                        py_max = cy;
                        is_before = false;
                    }
                    else if (x_d > 0) {
                        is_inside = true;
                        inside_fix_dir = UP_LEFT;
                    }
                    break;
                case LEFT_DOWN:
                    if (y_d >= 0) {
                        // Restrict y range to be not before the corner
                        py_min = cy;
                        is_before = false;
                    }
                    else if (x_d < 0) {
                        is_inside = true;
                        inside_fix_dir = DOWN_RIGHT;
                    }
                    break;
                case RIGHT_DOWN:
                    if (y_d >= 0) {
                        // Restrict y range to be not before the corner
                        py_min = cy;
                        is_before = false;
                    }
                    else if (x_d > 0) {
                        is_inside = true;
                        inside_fix_dir = DOWN_LEFT;
                    }
                    break;
                case UP_RIGHT:
                    if (x_d >= 0) {
                        // Restrict x range to be not before the corner
                        px_min = cx;
                        is_before = false;
                    }
                    else if (y_d < 0) {
                        is_inside = true;
                        inside_fix_dir = RIGHT_DOWN;
                    }
                    break;
                case DOWN_RIGHT:
                    if (x_d >= 0) {
                        // Restrict x range to be not before the corner
                        px_min = cx;
                        is_before = false;
                    }
                    else if (y_d > 0) {
                        is_inside = true;
                        inside_fix_dir = RIGHT_UP;
                    }
                    break;
                case TRINKET:
                case WARP_TOKEN:
                    if (x_d == 0 && y_d == 0) {
                        // Restrict x and y ranges to the trinket hitbox
                        px_min = cx_min;
                        px_max = cx_max;
                        py_min = cy_min;
                        py_max = cy_max;
                        is_before = false;
                    }
                    break;
                }
                if (!is_before) {
                    state_ul.bound_x(px_min, px_min);
                    state_dr.bound_x(px_max, px_max);
                    state_ul.bound_y(py_min, py_min);
                    state_dr.bound_y(py_max, py_max);
                    next_corner++;
                    continue;
                }
                else if (is_inside) {
                    if (next_corner != next_corner_orig || trinket_or_warp) {
                        // This shouldn't really ever happen
                        VVV_exit(782976);
                    }
                    // We "add" another corner so as not to be inside the next one
                    next_corner--;
                    c_dir = inside_fix_dir;
                }
            }
            
            // Sanity check: at least one of x_d or y_d is non-zero
            if (x_d == 0 && y_d == 0) {
                VVV_exit(120971);
            }

            bool is_vertical_cut = c_dir == UP_LEFT || c_dir == UP_RIGHT || c_dir == DOWN_LEFT || c_dir == DOWN_RIGHT;
            bool is_horizontal_cut = c_dir == LEFT_UP || c_dir == LEFT_DOWN || c_dir == RIGHT_UP || c_dir == RIGHT_DOWN;
            bool is_limited_by_x_d = is_vertical_cut || trinket_or_warp;
            bool is_limited_by_y_d = is_horizontal_cut || trinket_or_warp;

            bool going_up    = y_d >= 0;
            bool going_down  = y_d <= 0;
            bool going_left  = x_d >= 0;
            bool going_right = x_d <= 0;

            bool reached_corner_x = !(is_limited_by_x_d && x_d != 0);
            int x_frame_reached = (x_d == 0) ? state_ul.frame_count : INT_MAX;
            bool reached_corner_y = !(is_limited_by_y_d && y_d != 0);
            int y_frame_reached = (y_d == 0) ? state_ul.frame_count : INT_MAX;

            bool ul_accel = (going_left && -state_ul.vx < MAX_X_SPEED) || (going_up && -state_ul.vy < MAX_Y_SPEED);
            bool dr_accel = (going_right && state_dr.vx < MAX_X_SPEED) || (going_down && state_dr.vy < MAX_Y_SPEED);
            while (!reached_corner_x || !reached_corner_y || state_ul.frame_count != state_dr.frame_count) {
                // Advance UL state until it reaches top speed in both dimensions
                if (ul_accel || dr_accel) {
                    if (state_ul.frame_count > 0) {
                        do_sim_step(state_ul, true, false, !state_ul.gravity && can_flip);
                    } else {
                        int prev_px = state_ul.px;
                        int prev_rx = state_ul.rx;
                        int prev_py = state_ul.py;
                        int prev_ry = state_ul.ry;
                        do_sim_step(state_ul, true, false, !state_ul.gravity && can_flip);
                        if (state_ul.vx > 0.0f) {
                            // If vx is in wrong direction, we have to account for hitting a wall to turn around faster
                            // Note that we can check vx *after* the update because that's the speed that was applied 
                            state_ul.vx = 0.0f;
                            // We have to assume the wall is right was right in front of us from the beginning
                            state_ul.px = prev_px;
                            state_ul.rx = prev_rx;
                            state_ul.x_dist = 0;
                        }
                        if (state_ul.vy > 0.0f) {
                            // As above, we have to account for touching a floor to turn around faster
                            state_ul.vy = 0.0f;
                            state_ul.py = prev_py;
                            state_ul.ry = prev_ry;
                            state_ul.y_dist = 0;
                        }
                    }
                }
                // Advance DR state until it reaches top speed in both dimensions
                if (ul_accel || dr_accel) {
                    // Unfortunately we have to deal with double flips
                    if (state_dr.vy < 4.0f && can_double_flip) {
                        // I think this is sufficient?
                        state_dr.gravity = true;
                        if (!can_flip) {
                            // Sanity check
                            VVV_exit(209348);
                        }
                    }

                    if (state_dr.frame_count > 0) {
                        do_sim_step(state_dr, false, true, state_dr.gravity && can_flip);
                    } else {
                        int prev_px = state_dr.px;
                        int prev_rx = state_dr.rx;
                        int prev_py = state_dr.py;
                        int prev_ry = state_dr.ry;
                        do_sim_step(state_dr, false, true, state_dr.gravity && can_flip);
                        if (state_dr.vx < 0.0f) {
                            // If vx is in wrong direction, we have to account for hitting a wall to turn around faster
                            // Note that we can check vx *after* the update because that's the speed that was applied 
                            state_dr.vx = 0.0f;
                            // We have to assume the wall is right was right in front of us from the beginning
                            state_dr.px = prev_px;
                            state_dr.rx = prev_rx;
                            state_dr.x_dist = 0;
                        }
                        if (state_dr.vy < 0.0f) {
                            // As above, we have to account for touching a floor to turn around faster
                            state_dr.vy = 0.0f;
                            state_dr.py = prev_py;
                            state_dr.ry = prev_ry;
                            state_dr.y_dist = 0;
                        }
                    }
                }
                // We have to assume we will be able to flip next frame
                can_flip = true;
                can_double_flip = true;
                if (!ul_accel && !dr_accel) {
                    state_ul.vx = -MAX_X_SPEED;
                    state_dr.vx = MAX_X_SPEED;
                    state_ul.vy = -MAX_Y_SPEED;
                    state_dr.vy = MAX_Y_SPEED;
                    // We've reached max speed in all relevant directions, so we don't have to worry about rounding anymore
                    int l_dist = SDL_max(0, px_min + state_ul.x_dist - cx_max);
                    int u_dist = SDL_max(0, py_min + state_ul.y_dist - cy_max);
                    int r_dist = SDL_max(0, cx_min - (px_max + state_dr.x_dist));
                    int d_dist = SDL_max(0, cy_min - (py_max + state_dr.y_dist));

                    int l_frames = (l_dist + MAX_X_SPEED - 1) / MAX_X_SPEED;
                    int u_frames = (u_dist + MAX_Y_SPEED - 1) / MAX_Y_SPEED;
                    int r_frames = (r_dist + MAX_X_SPEED - 1) / MAX_X_SPEED;
                    int d_frames = (d_dist + MAX_Y_SPEED - 1) / MAX_Y_SPEED;

                    // On which frame do we reach the corner in the respective dimension?
                    x_frame_reached = SDL_min(x_frame_reached, SDL_max(state_ul.frame_count + l_frames, state_dr.frame_count + r_frames));
                    y_frame_reached = SDL_min(y_frame_reached, SDL_max(state_ul.frame_count + u_frames, state_dr.frame_count + d_frames));

                    // This is unfortunately necessary to keep things consistent
                    if (!is_limited_by_x_d) {
                        l_frames = 0;
                        r_frames = 0;
                    }
                    if (!is_limited_by_y_d) {
                        u_frames = 0;
                        d_frames = 0;
                    }

                    int ul_frames = SDL_max(l_frames, u_frames);
                    int dr_frames = SDL_max(r_frames, d_frames);
                    int total_frames = SDL_max(ul_frames + state_ul.frame_count, dr_frames + state_dr.frame_count);
                    int extra_ul_frames = total_frames - state_ul.frame_count;
                    int extra_dr_frames = total_frames - state_dr.frame_count;

                    // Sanity check, at least one extra frame somewhere (even if it's just to catch up to the other state)
                    if (SDL_max(extra_ul_frames, extra_dr_frames) <= 0) VVV_exit(203974);

                    state_ul.frame_count += extra_ul_frames;
                    state_dr.frame_count += extra_dr_frames;
                    // Travel max distance in all directions so we have accurate min and max positions
                    state_ul.x_dist -= extra_ul_frames * MAX_X_SPEED;
                    state_ul.y_dist -= extra_ul_frames * MAX_Y_SPEED;
                    state_ul.tapleft += extra_ul_frames;
                    state_ul.tapright = 0;
                    state_ul.gravity = true;

                    state_dr.x_dist += extra_dr_frames * MAX_X_SPEED;
                    state_dr.y_dist += extra_dr_frames * MAX_Y_SPEED;
                    state_dr.tapright += extra_dr_frames;
                    state_dr.tapleft = 0;
                    state_dr.gravity = false;
                    // We intentionally forgo updating px, rx and input_count here
                    break;
                }
                
                // We might reach the corner while still accelerating
                if (x_frame_reached == INT_MAX && px_min + state_ul.x_dist <= cx_max && px_max + state_dr.x_dist >= cx_min) {
                    reached_corner_x = true;
                    x_frame_reached = SDL_max(state_ul.frame_count, state_dr.frame_count);
                }
                if (y_frame_reached == INT_MAX && py_min + state_ul.y_dist <= cy_max && py_max + state_dr.y_dist >= cy_min) {
                    reached_corner_y = true;
                    y_frame_reached = SDL_max(state_ul.frame_count, state_dr.frame_count);
                }
                ul_accel = (going_left && -state_ul.vx < MAX_X_SPEED) || (going_up  && -state_ul.vy < MAX_Y_SPEED);
                dr_accel = (going_right && state_dr.vx < MAX_X_SPEED) || (going_down && state_dr.vy < MAX_Y_SPEED);
            }

            if (!reached_corner_x || !reached_corner_y || state_ul.frame_count != state_dr.frame_count) {
                if (px_min + state_ul.x_dist <= cx_max && px_max + state_dr.x_dist >= cx_min) {
                    reached_corner_x = true;
                }
                if (py_min + state_ul.y_dist <= cy_max && py_max + state_dr.y_dist >= cy_min) {
                    reached_corner_y = true;
                }
                if (!reached_corner_x || !reached_corner_y || state_ul.frame_count != state_dr.frame_count) {
                    // Sanity check
                    VVV_exit(23705);
                }
            }

            px_min += state_ul.x_dist;
            px_max += state_dr.x_dist;
            py_min += state_ul.y_dist;
            py_max += state_dr.y_dist;

            // Note that if we are "inside" the corner (e.g. x_d > 0 && y_d < 0 for UP_LEFT),
            //   then we just pretend we can walk through walls. The max corner cut distance constraint
            //   makes sure we don't completely wreck our heuristic
            // Note also that if we are already past the corner (i.e. x_d <= 0 for UP_LEFT),
            //   then we do nothing as we want to preserve min_x, max_x, min_y and max_y for the next corner
            switch (c_dir) {
            case UP_LEFT: // Limiting factor: leftwards movement
                if (x_d <= 0 || px_min > cx || cx > px_max)
                    VVV_exit(57031);
                // Can't cut more than 10 pixels past the corner vertically
                px_min = px_min;
                py_min = SDL_max(py_min, cy - Y_SPEED);
                // In case we want to hug the corner
                px_max = cx;
                py_max = SDL_max(py_max, cy);
                break;
            case UP_RIGHT: // Limiting factor: rightwards movement
                if (x_d >= 0 || px_min > cx || cx > px_max)
                    VVV_exit(57032);
                // Can't cut more than 10 pixels past the corner vertically
                px_max = px_max;
                py_min = SDL_max(py_min, cy - Y_SPEED);
                // In case we want to hug the corner
                px_min = cx;
                py_max = SDL_max(py_max, cy);
                break;
            case LEFT_UP: // Limiting factor: upwards movement
                if (y_d <= 0 || py_min > cy || cy > py_max)
                    VVV_exit(57033);
                // Can't cut more than 0 pixels past the corner horizontally
                px_min = SDL_max(px_min, cx);
                py_min = py_min;
                // In case we want to hug the corner
                px_max = SDL_max(px_max, cx);
                py_max = cy;
                break;
            case LEFT_DOWN: // Limiting factor: downwards movement
                if (y_d >= 0 || py_min > cy || cy > py_max)
                    VVV_exit(57034);
                // Can't cut more than 0 pixels past the corner horizontally
                px_min = SDL_max(px_min, cx);
                py_max = py_max;
                // In case we want to hug the corner
                px_max = SDL_max(px_max, cx);
                py_min = cy;
                break;
            case DOWN_LEFT: // Limiting factor: leftwards movement
                if (x_d <= 0 || px_min > cx || cx > px_max)
                    VVV_exit(57035);
                // Can't cut more than 10 pixels past the corner vertically
                px_min = px_min;
                py_max = SDL_min(py_max, cy + Y_SPEED);
                // In case we want to hug the corner
                px_max = cx;
                py_min = SDL_min(py_min, cy);
                break;
            case DOWN_RIGHT: // Limiting factor: rightwards movement
                if (x_d >= 0 || px_min > cx || cx > px_max)
                    VVV_exit(57036);
                // TODO: if py_min > cy, we might be forced to land on the corner
                //       does that mean we can reduce the max corner cut distance?

                // Can't cut more than 10 pixels past the corner vertically
                px_max = px_max;
                py_max = SDL_min(py_max, cy + Y_SPEED);
                // In case we want to hug the corner
                px_min = cx;
                py_min = SDL_min(py_min, cy);
                break;
            case RIGHT_UP: // Limiting factor: upwards movement
                if (y_d <= 0 || py_min > cy || cy > py_max)
                    VVV_exit(57037);
                // Can't cut more than 0 pixels past the corner horizontally
                px_max = SDL_min(px_max, cx);
                py_min = py_min;
                // In case we want to hug the corner
                px_min = SDL_min(px_min, cx);
                py_max = cy;
                break;
            case RIGHT_DOWN: // Limiting factor: downwards movement
                if (y_d >= 0 || py_min > cy || cy > py_max)
                    VVV_exit(57038);
                // Can't cut more than 0 pixels past the corner horizontally
                px_max = SDL_min(px_max, cx);
                py_max = py_max;
                // In case we want to hug the corner
                px_min = SDL_min(px_min, cx);
                py_min = cy;
                break;
            case TRINKET:
            case WARP_TOKEN:
                // Could be in any direction
                // What are the min and max positions reachable while still collecting the trinket?
                // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
                px_min = SDL_max(px_min, cx - 17);
                px_max = SDL_min(px_max, cx + 9);
                py_min = SDL_max(py_min, cy - 22);
                py_max = SDL_min(py_max, cy + 13);
                break;
            }

            // TODO: we could also bound vx and vy here based on corner type
            state_ul.bound_x(px_min, px_min);
            state_dr.bound_x(px_max, px_max);
            state_ul.bound_y(py_min, py_min);
            state_dr.bound_y(py_max, py_max);

            int spare_x_frames = SDL_max(0, y_frame_reached - x_frame_reached);
            int spare_y_frames = SDL_max(0, x_frame_reached - y_frame_reached);

            if (spare_x_frames == 0 || !is_limited_by_y_d) {
                if (x_d > 0) {
                    state_ul.bound_vx(-MAX_X_SPEED, -X_RATE);
                    state_dr.bound_vx(-MAX_X_SPEED, -X_RATE);
                    if (px_min == px_max && state_ul.vx == -MAX_X_SPEED) {
                        // We *exactly* reached the corner while going full speed
                        // So vx must be minimal
                        state_dr.vx = state_ul.vx;
                        state_dr.tapright = SDL_min(state_dr.tapright, state_ul.tapleft);
                        state_dr.tapleft = state_ul.tapleft;
                    }
                    else {
                        // Assume the best case scenario, we can cancel leftward speed
                        state_dr.tapleft = 1;
                    }
                } else if (x_d < 0) {
                    state_ul.bound_vx(X_RATE, MAX_X_SPEED);
                    state_dr.bound_vx(X_RATE, MAX_X_SPEED);
                    // tapleft can't be more than 1 without having vx <= 0.0
                    state_ul.tapleft = SDL_max(state_ul.tapleft, 1);
                    if (px_min == px_max && state_dr.vx == MAX_X_SPEED) {
                        // We *exactly* reached the corner while going full speed
                        // So vx must be maximal
                        state_ul.vx = state_dr.vx;
                        state_ul.tapright = state_dr.tapright;
                        state_ul.tapleft = state_dr.tapleft;
                    }
                    else {
                        // Assume the best case scenario, we can cancel rightward speed
                        state_ul.tapright = 1;
                    }
                }
            }
            else if (spare_x_frames <= 3) {
                float accel_per_frame = 3.0f - X_RATE;
                if (x_d > 0) {
                    state_ul.bound_vx(-MAX_X_SPEED, spare_x_frames * accel_per_frame);
                    state_dr.bound_vx(-MAX_X_SPEED, spare_x_frames * accel_per_frame);
                    // Allow cancelling speed if we want to
                    state_dr.tapleft = 1;
                }
                else if (x_d < 0) {
                    state_ul.bound_vx(-spare_x_frames * accel_per_frame, MAX_X_SPEED);
                    state_dr.bound_vx(-spare_x_frames * accel_per_frame, MAX_X_SPEED);
                    // Allow cancelling speed if we want to
                    state_ul.tapright = 1;
                }
            }


            if (spare_y_frames == 0 || !is_limited_by_x_d) {
                if (y_d > 0) {
                    state_ul.bound_vy(-MAX_Y_SPEED, -Y_RATE);
                    state_dr.bound_vy(-MAX_Y_SPEED, -Y_RATE);
                    state_dr.gravity = state_ul.gravity;
                }
                else if (y_d < 0) {
                    state_ul.bound_vy(Y_RATE, MAX_Y_SPEED);
                    state_dr.bound_vy(Y_RATE, MAX_Y_SPEED);
                    state_ul.gravity = state_dr.gravity;
                }
            }
            
            total_frames += state_ul.frame_count;

            state_ul.reset_counters();
            state_dr.reset_counters();

            next_corner++;
        }

        // We are now guaranteed not to be before the final corner
        // But we want to know how long it takes to get past it
        corner c = scenario.corners[next_corner - 1];
        corner_dir c_dir = c.dir;
        int cx = room_adjusted_x(c.rx, c.x);
        int cy = room_adjusted_y(c.ry, c.y);

        int px_min = room_adjusted_x(state_ul.rx, state_ul.px);
        int px_max = room_adjusted_x(state_dr.rx, state_dr.px);
        int py_min = room_adjusted_y(state_ul.ry, state_ul.py);
        int py_max = room_adjusted_y(state_dr.ry, state_dr.py);

        bool trinket_or_warp = c_dir == TRINKET || c_dir == WARP_TOKEN;

        int cx_min = cx;
        int cx_max = cx;
        int cy_min = cy;
        int cy_max = cy;
        if (trinket_or_warp) {
            // Note that we can collect the trinket in the range x [-17, 9], y [-22, 13]
            cx_min = cx - 17;
            cx_max = cx + 9;
            cy_min = cy - 22;
            cy_max = cy + 13;
        }

        // Subtract corner position from player position
        int x_d = px_min > cx_max ? (px_min - cx_max) : (px_max < cx_min ? (px_max - cx_min) : 0);
        int y_d = py_min > cy_max ? (py_min - cy_max) : (py_max < cy_min ? (py_max - cy_min) : 0);

        // Skip this corner if we may no longer be before it
        {
            bool is_after = false;
            switch (c_dir) {
            case UP_LEFT:
            case UP_RIGHT:
                if (y_d <= 0) {
                    is_after = true;
                }
                break;
            case LEFT_UP:
            case LEFT_DOWN:
                if (x_d <= 0) {
                    is_after = true;
                }
                break;
            case DOWN_LEFT:
            case DOWN_RIGHT:
                if (y_d >= 0) {
                    is_after = true;
                }
                break;
            case RIGHT_UP:
            case RIGHT_DOWN:
                if (x_d >= 0) {
                    is_after = true;
                }
                break;
            case TRINKET:
            case WARP_TOKEN:
                if (x_d == 0 && y_d == 0) {
                    is_after = true;
                }
                else {
                    // This shouldn't happen
                    VVV_exit(295493);
                }
                break;
            }
            if (!is_after) {
                if (x_d != 0 && y_d != 0) {
                    // This shouldn't happen
                    VVV_exit(295494);
                }

                bool reached_corner_x = x_d == 0;
                bool reached_corner_y = y_d == 0;
                bool ul_accel = (x_d > 0 && -state_ul.vx < MAX_X_SPEED) || (y_d > 0 && -state_ul.vy < MAX_Y_SPEED);
                bool dr_accel = (x_d < 0 && state_dr.vx < MAX_X_SPEED) || (y_d < 0 && state_dr.vy < MAX_Y_SPEED);
                while ((!reached_corner_x || !reached_corner_y) && (ul_accel || dr_accel)) {
                    if (ul_accel || dr_accel) {
                        if (state_ul.frame_count > 0) {
                            do_sim_step(state_ul, true, false, !state_ul.gravity && can_flip);
                        }
                        else {
                            int prev_px = state_ul.px;
                            int prev_rx = state_ul.rx;
                            int prev_py = state_ul.py;
                            int prev_ry = state_ul.ry;
                            do_sim_step(state_ul, true, false, !state_ul.gravity && can_flip);
                            if (state_ul.vx > 0.0f) {
                                // If vx is in wrong direction, we have to account for hitting a wall to turn around faster
                                // Note that we can check vx *after* the update because that's the speed that was applied 
                                state_ul.vx = 0.0f;
                                // We have to assume the wall is right was right in front of us from the beginning
                                state_ul.px = prev_px;
                                state_ul.rx = prev_rx;
                                state_ul.x_dist = 0;
                            }
                            if (state_ul.vy > 0.0f) {
                                // As above, we have to account for touching a floor to turn around faster
                                state_ul.vy = 0.0f;
                                state_ul.py = prev_py;
                                state_ul.ry = prev_ry;
                                state_ul.y_dist = 0;
                            }
                        }
                        if (state_dr.frame_count > 0) {
                            do_sim_step(state_dr, false, true, state_dr.gravity && can_flip);
                        }
                        else {
                            int prev_px = state_dr.px;
                            int prev_rx = state_dr.rx;
                            int prev_py = state_dr.py;
                            int prev_ry = state_dr.ry;
                            do_sim_step(state_dr, false, true, state_dr.gravity && can_flip);
                            if (state_dr.vx < 0.0f) {
                                // If vx is in wrong direction, we have to account for hitting a wall to turn around faster
                                // Note that we can check vx *after* the update because that's the speed that was applied 
                                state_dr.vx = 0.0f;
                                // We have to assume the wall is right was right in front of us from the beginning
                                state_dr.px = prev_px;
                                state_dr.rx = prev_rx;
                                state_dr.x_dist = 0;
                            }
                            if (state_dr.vy < 0.0f) {
                                // As above, we have to account for touching a floor to turn around faster
                                state_dr.vy = 0.0f;
                                state_dr.py = prev_py;
                                state_dr.ry = prev_ry;
                                state_dr.y_dist = 0;
                            }
                        }
                    }

                    can_flip = true;
                    can_double_flip = true;
                    if (!reached_corner_x && px_min + state_ul.x_dist <= cx_max && px_max + state_dr.x_dist >= cx_min) {
                        reached_corner_x = true;
                    }
                    if (!reached_corner_y && py_min + state_ul.y_dist <= cy_max && py_max + state_dr.y_dist >= cy_min) {
                        reached_corner_y = true;
                    }
                    ul_accel = (x_d > 0 && -state_ul.vx < MAX_X_SPEED) || (y_d > 0 && -state_ul.vy < MAX_Y_SPEED);
                    dr_accel = (x_d < 0 && state_dr.vx < MAX_X_SPEED) || (y_d < 0 && state_dr.vy < MAX_Y_SPEED);
                }

                px_min += state_ul.x_dist;
                px_max += state_dr.x_dist;
                py_min += state_ul.y_dist;
                py_max += state_dr.y_dist;
                total_frames += state_ul.frame_count;

                x_d = px_min > cx_max ? (px_min - cx_max) : (px_max < cx_min ? (px_max - cx_min) : 0);
                y_d = py_min > cy_max ? (py_min - cy_max) : (py_max < cy_min ? (py_max - cy_min) : 0);

                if (x_d != 0 || y_d != 0) {
                    int x_frames = (SDL_abs(x_d) + MAX_X_SPEED - 1) / MAX_X_SPEED;
                    int y_frames = (SDL_abs(y_d) + MAX_Y_SPEED - 1) / MAX_Y_SPEED;
                    int extra_frames = SDL_max(x_frames, y_frames);
                    total_frames += extra_frames;
                }
            }
        }

        return total_frames;
    }

    void do_sim_step(SimState& state, bool press_left, bool press_right, bool flip) {
        state.frame_count++;

        // Update acceleration and tap state
        float ax = 0;
        if (press_right) {
            state.tapright++;
            state.input_count++;
            ax = 3;
        }
        else {
            if (0 < state.tapright && state.tapright < 5 && state.vx > 0.0f) {
                state.vx = 0;
            }
            state.tapright = 0;
        }
        // Left must come after right so that it overrides acceleration if both are pressed
        if (press_left) {
            state.tapleft++;
            state.input_count++;
            ax = -3;
        }
        else {
            if (0 < state.tapleft && state.tapleft < 5 && state.vx < 0.0f) {
                state.vx = 0;
            }
            state.tapleft = 0;
        }
        // Process flip input
        if (flip) {
            state.input_count++;
            state.gravity = !state.gravity;
            state.vy = state.gravity ? -4 : 4;
        }

        // Y accel is just gravity
        float ay = state.gravity ? -3 : 3;

        // Update X velocity
        state.vx += ax;
        if (state.vx >  0.00f) state.vx -= X_RATE;
        if (state.vx <  0.00f) state.vx += X_RATE;
        if (state.vx >  MAX_X_SPEED) state.vx =  MAX_X_SPEED;
        if (state.vx < -MAX_X_SPEED) state.vx = -MAX_X_SPEED;
        if (SDL_fabsf(state.vx) < X_RATE) state.vx = 0.0f;

        // Update Y velocity
        state.vy += ay;
        if (state.vy > 0.00f) state.vy -= Y_RATE;
        if (state.vy < 0.00f) state.vy += Y_RATE;
        if (state.vy > 10.00f) state.vy = 10.0f;
        if (state.vy < -10.00f) state.vy = -10.0f;
        if (SDL_fabsf(state.vy) < Y_RATE) state.vy = 0.0f;

        // Update X position
        int delta_x = -state.px;
        state.px += state.vx;
        delta_x += state.px;
        state.x_dist += delta_x;

        // Update Y position
        int delta_y = -state.py;
        state.py += state.vy;
        delta_y += state.py;
        state.y_dist += delta_y;

        // TODO: Warping rooms are not taken into account here!
        // Note that Y room transition happens first!
        if (state.py >= 238) {
            state.py -= 240;
            state.ry++;
        }
        else if (state.py < -2) {
            state.py += 240;
            state.ry--;
        }
        // X room transition second
        if (state.px >= 308) {
            state.px -= 320;
            state.rx++;
        }
        else if (state.px < -14) {
            state.px += 320;
            state.rx--;
        }
    }

    void SimState::reset_counters() {
        this->frame_count = 0;
        this->input_count = 0;
        this->x_dist = 0;
        this->y_dist = 0;
    }
    void SimState::bound_x(int min_x, int max_x) {
        int self_x = room_adjusted_x(this->rx, this->px);
        int new_x = SDL_clamp(self_x, min_x, max_x);
        int x_diff = new_x - self_x;
        int rx_diff = x_diff / 320;
        x_diff = x_diff - (rx_diff * 320);
        this->px += x_diff;
        this->rx += rx_diff;
        if (this->px >= 308) {
            this->px -= 320;
            this->rx++;
        }
        else if (this->px < -14) {
            this->px += 320;
            this->rx--;
        }
        if (this->px >= 308 || this->px < -14) {
            // Sanity check
            VVV_exit(275902);
        }
    }
    void SimState::bound_y(int min_y, int max_y) {
        int self_y = room_adjusted_y(this->ry, this->py);
        int new_y = SDL_clamp(self_y, min_y, max_y);
        int y_diff = new_y - self_y;
        int ry_diff = y_diff / 240;
        y_diff = y_diff - (ry_diff * 240);
        this->py += y_diff;
        this->ry += ry_diff;
        if (this->py >= 238) {
            this->py -= 240;
            this->ry++;
        }
        else if (this->py < -2) {
            this->py += 240;
            this->ry--;
        }
        if (this->py >= 238 || this->py < -2) {
            // Sanity check
            VVV_exit(275903);
        }
    }
    void SimState::bound_vx(float min_vx, float max_vx) {
        this->vx = SDL_clamp(this->vx, min_vx, max_vx);
    }
    void SimState::bound_vy(float min_vy, float max_vy) {
        this->vy = SDL_clamp(this->vy, min_vy, max_vy);
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
        result = combine_hashes(result, h_i(s.collect));

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
            if (bs[i].type == BLOCK || bs[i].type == TRIGGER || bs[i].type == ACTIVITY) {
                result = combine_hashes(result, hash_block(bs[i]));
            }
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
        std::uint64_t value = 0;
        value = (value << 7) | (e.type + 1);
        // e_hash = combine_hashes(e_hash, h_i(e.type));   // 6.67 bits
        value = (value << 4) | (e.rule + 1);
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

        value = (value << 1) | e.invis;
        // e_hash = combine_hashes(e_hash, h_b(e.invis));
        value = (value << 4) | e.life;
        // e_hash = combine_hashes(e_hash, h_i(e.life));
        value = (value << 2) | e.invis;
        // e_hash = combine_hashes(e_hash, h_i(e.onentity));

        e_hash = combine_hashes(e_hash, h_u(value));

        return e_hash;
    }

    std::size_t combine_hashes(std::size_t h1, std::size_t h2) {
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
}
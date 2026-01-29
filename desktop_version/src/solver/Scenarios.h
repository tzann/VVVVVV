#ifndef SCENARIOS_H
#define SCENARIOS_H

namespace Scenarios {
    enum CornerDir {
        // Add 4 to invert first dir (means can't be sequential)
        UP_LEFT,
        UP_RIGHT,
        LEFT_UP,
        LEFT_DOWN,
        DOWN_LEFT,
        DOWN_RIGHT,
        RIGHT_UP,
        RIGHT_DOWN,
        TRINKET,
        WARP_TOKEN,
        // To get "canonical" previous corner direction
        // 0 -> 3 -> 5 -> 6
        // 7 -> 4 -> 2 -> 1
    };

    struct RawCorner {
        int rx;
        int ry;
        int x;
        int y;
        CornerDir dir;

        RawCorner(int a, int b, int c, int d, CornerDir e) : rx(a), ry(b), x(c), y(d), dir(e) {}
    };

    struct RawScenario {
        int init_rx;
        int init_ry;
        int init_x;
        int init_y;
        int init_gravity;
        int frames_to_advance;
        std::vector<RawCorner> corners;

        RawScenario(int a, int b, int c, int d, int e, int f, std::vector<RawCorner> cs) : init_rx(a), init_ry(b), init_x(c), init_y(d), init_gravity(e), frames_to_advance(f), corners(cs) {}
    };

    namespace SS1 {
        namespace CORNERS {
            // Solitude
            const RawCorner SOLITUDE(115, 105, 114, 126, UP_RIGHT);

            // Traffic Jam
            namespace TRAFFIC_JAM {
                const RawCorner ENTRY_1(115, 103, 250, 182, UP_RIGHT);
                const RawCorner ENTRY_2(115, 103, 250, 97, LEFT_UP);
            }

            // Atmospheric Filtering Unit (part 1)
            namespace ATMOSPHERIC_FILTERING_UNIT {
                const RawCorner ENTRY_1(114, 103, 258, 73, LEFT_UP);
                const RawCorner ENTRY_2(114, 103, 222, 73, DOWN_LEFT);
            }

            // It's a Secret to Nobody
            namespace ITS_A_SECRET_TO_NOBODY {
                const RawCorner ENTRY(114, 104, 202, 78, LEFT_DOWN);
                const RawCorner TRINKET(114, 104, 16, 136, CornerDir::TRINKET);
                const RawCorner EXIT(114, 104, 202, 78, UP_RIGHT);
            }

            // Atmospheric Filtering Unit (part 2)
            namespace ATMOSPHERIC_FILTERING_UNIT {
                const RawCorner EXIT_1(114, 103, 74, 142, LEFT_DOWN);
                const RawCorner EXIT_2(114, 103, 38, 142, UP_LEFT);
                const RawCorner EXIT_3(114, 103, 18, 97, LEFT_UP);
            }

            // Linear collider
            namespace LINEAR_COLLIDER {
                const RawCorner ENTRY_1(113, 103, 278, 97, DOWN_LEFT);
                const RawCorner ENTRY_2(113, 103, 264, 182, LEFT_DOWN);
                const RawCorner ENTRY_3(113, 103, 230, 182, UP_LEFT);
                const RawCorner ENTRY_4(113, 103, 218, 129, LEFT_UP);
                const RawCorner EXIT_1(113, 103, 78, 78, UP_LEFT);
                const RawCorner EXIT_2(113, 103, 66, 25, LEFT_UP);
                const RawCorner EXIT_3(113, 103, 30, 25, DOWN_LEFT);
                const RawCorner EXIT_4(113, 103, 18, 110, LEFT_DOWN);
            }

            // Security Sweep
            namespace SECURITY_SWEEP {
                const RawCorner ENTRY_1(112, 103, 294, 110, UP_LEFT);
                const RawCorner ENTRY_2(112, 103, 258, 33, LEFT_UP);
                const RawCorner FLIP_DOWN(112, 103, 206, 33, DOWN_LEFT);
                const RawCorner GO_LEFT(112, 103, 186, 142, LEFT_DOWN);
                const RawCorner EXIT(112, 103, 78, 161, DOWN_LEFT);
            }

            // Gantry and Dolly (part 1)
            namespace GANTRY_AND_DOLLY {
                const RawCorner ENTRY_1(112, 104, 78, 46, RIGHT_DOWN);
                const RawCorner ENTRY_2(112, 104, 86, 70, RIGHT_DOWN);
                const RawCorner PLATFORM(112, 104, 194, 70, UP_RIGHT);
                const RawCorner EXIT_RIGHT(112, 104, 246, 65, RIGHT_UP);
            }

            // Comms Relay
            namespace COMMS_RELAY {
                const RawCorner STEPS(113, 104, 10, 65, DOWN_RIGHT);
                const RawCorner LEDGE(113, 104, 242, 97, DOWN_RIGHT);
                const RawCorner SNAKE_1(113, 104, 242, 190, LEFT_DOWN);
                const RawCorner SNAKE_2(113, 104, 206, 190, UP_LEFT);
                const RawCorner SNAKE_3(113, 104, 194, 161, LEFT_UP);
                const RawCorner SNAKE_4(113, 104, 134, 161, DOWN_LEFT);
                const RawCorner SNAKE_5(113, 104, 122, 190, LEFT_DOWN);
                const RawCorner SNAKE_6(113, 104, 86, 190, UP_LEFT);
                const RawCorner SNAKE_7(113, 104, 74, 161, LEFT_UP);
                const RawCorner SNAKE_8(113, 104, 38, 161, DOWN_LEFT);
            }

            // Gantry and Dolly (part 2)
            namespace GANTRY_AND_DOLLY {
                const RawCorner EXIT(112, 104, 78, 185, DOWN_LEFT);
            }

            // The Yes Men
            namespace THE_YES_MEN {
                const RawCorner ENTRY(112, 105, 78, 38, RIGHT_DOWN);
                const RawCorner DROP(112, 105, 258, 73, DOWN_RIGHT);
                const RawCorner LEFT(112, 105, 258, 134, LEFT_DOWN);
                const RawCorner EXIT(112, 105, 78, 169, DOWN_LEFT);
            }

            // Stop and Reflect (part 1)
            namespace STOP_AND_REFLECT {
                const RawCorner ENTRY(112, 106, 78, 46, RIGHT_DOWN);
                const RawCorner LEDGE(112, 106, 114, 81, DOWN_RIGHT);
            }

            // Trench Warfare
            const RawCorner TRENCH_WARFARE(113, 106, 272, 152, TRINKET);

            // Stop and Reflect (part 2)
            namespace STOP_AND_REFLECT {
                const RawCorner EXIT_LEDGE(112, 106, 114, 126, LEFT_DOWN);
                const RawCorner EXIT_SPIKE(112, 106, 78, 161, DOWN_LEFT);
            }

            // V Stitch
            const RawCorner V_STITCH(112, 107, 78, 22, RIGHT_DOWN);

            // Quicksand
            namespace QUICKSAND {
                const RawCorner ENTRY(116, 106, 86, 41, RIGHT_UP);
                const RawCorner LEDGE(116, 106, 122, 41, DOWN_RIGHT);
                const RawCorner SPIKE(116, 106, 154, 121, DOWN_RIGHT); // Spike corner, not sure if useful
            }

            // The Tomb of Mad Carew
            namespace THE_TOMB_OF_MAD_CAREW {
                const RawCorner ENTRY_1(116, 107, 158, 22, RIGHT_DOWN);
                const RawCorner ENTRY_2(116, 107, 174, 38, RIGHT_DOWN);
            }

            // Brass Sent Us Under The Top
            const RawCorner BRASS_SENT_US_UNDER_THE_TOP(117, 107, 250, 89, DOWN_RIGHT);

            // A Wrinkle in Time
            namespace A_WRINKLE_IN_TIME {
                const RawCorner IL_END(118, 107, 55, 121, DOWN_RIGHT); // IL ending box
                const RawCorner STEPS(118, 107, 98, 121, DOWN_RIGHT); // first teleporter step
            }
        }
        namespace SCENARIOS {
            const RawScenario START_TO_FIRST_TRINKET(113, 105, 200, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ITS_A_SECRET_TO_NOBODY::ENTRY,
                CORNERS::ITS_A_SECRET_TO_NOBODY::TRINKET,
                CORNERS::ITS_A_SECRET_TO_NOBODY::EXIT,
                });

            const RawScenario START(113, 105, 200, 161, 0, 0, { CORNERS::SOLITUDE });

            const RawScenario START_TO_LINEAR_COLLIDER(113, 105, 200, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_2,
                });

            const RawScenario TRAFFIC_JAM_TO_SECRET(115, 105, 57, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                });

            const RawScenario ITS_A_SECRET_TO_NOBODY(114, 103, 301, 46, 1, 0, {
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ITS_A_SECRET_TO_NOBODY::ENTRY,
                CORNERS::ITS_A_SECRET_TO_NOBODY::TRINKET,
                CORNERS::ITS_A_SECRET_TO_NOBODY::EXIT,
                });

            const RawScenario TRAFFIC_JAM_TO_LINEAR_COLLIDER(115, 105, 57, 161, 0, 0, {
                CORNERS::SOLITUDE,
                CORNERS::TRAFFIC_JAM::ENTRY_1,
                CORNERS::TRAFFIC_JAM::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::ENTRY_2,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_1,
                CORNERS::ATMOSPHERIC_FILTERING_UNIT::EXIT_2,
                });

            const RawScenario LINEAR_COLLIDER(114, 103, -14, 46, 1, 0, {
                CORNERS::LINEAR_COLLIDER::ENTRY_1,
                CORNERS::LINEAR_COLLIDER::ENTRY_2,
                CORNERS::LINEAR_COLLIDER::ENTRY_3,
                CORNERS::LINEAR_COLLIDER::ENTRY_4,
                CORNERS::LINEAR_COLLIDER::EXIT_1,
                CORNERS::LINEAR_COLLIDER::EXIT_2,
                CORNERS::LINEAR_COLLIDER::EXIT_3,
                CORNERS::LINEAR_COLLIDER::EXIT_4,
                });

            const RawScenario SECURITY_SWEEP(113, 103, -14, 161, 0, 0, {
                CORNERS::SECURITY_SWEEP::ENTRY_1,
                CORNERS::SECURITY_SWEEP::ENTRY_2,
                CORNERS::SECURITY_SWEEP::FLIP_DOWN,
                CORNERS::SECURITY_SWEEP::GO_LEFT,
                CORNERS::SECURITY_SWEEP::EXIT,
                });

            const RawScenario LINEAR_COLLIDER_TO_SECURITY_SWEEP(114, 103, -14, 46, 1, 0, {
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

            const RawScenario GANTRY_AND_DOLLY_TO_COMMS_RELAY(112, 103, 150, 161, 0, 0, {
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

            const RawScenario THE_YES_MEN(112, 104, 113, 185, 0, 40, {
                CORNERS::GANTRY_AND_DOLLY::EXIT,
                CORNERS::THE_YES_MEN::ENTRY,
                CORNERS::THE_YES_MEN::DROP,
                CORNERS::THE_YES_MEN::LEFT,
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                });

            const RawScenario THE_YES_MEN_TO_VSTITCH(112, 104, 113, 185, 0, 0, {
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

            const RawScenario THE_YES_MEN_TO_TRENCH_WARFARE(112, 104, 113, 185, 0, 0, {
                CORNERS::GANTRY_AND_DOLLY::EXIT,
                CORNERS::THE_YES_MEN::ENTRY,
                CORNERS::THE_YES_MEN::DROP,
                CORNERS::THE_YES_MEN::LEFT,
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::TRENCH_WARFARE,
                });

            const RawScenario STOP_AND_REFLECT_TO_TRENCH_WARFARE(112, 105, 235, 134, 1, 48, {
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::TRENCH_WARFARE,
                });

            const RawScenario STOP_AND_REFLECT_TO_VSTITCH(112, 105, 105, 134, 1, 48, {
                CORNERS::THE_YES_MEN::EXIT,
                CORNERS::STOP_AND_REFLECT::ENTRY,
                CORNERS::STOP_AND_REFLECT::LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_SPIKE,
                CORNERS::V_STITCH,
                });

            const RawScenario TRENCH_WARFARE_GRAB(112, 106, 297, 134, 0, 0, {
                CORNERS::TRENCH_WARFARE,
                });

            const RawScenario TRENCH_WARFARE_BACK_TO_VSTITCH(113, 106, 255, 152, 0, 40, {
                CORNERS::STOP_AND_REFLECT::EXIT_LEDGE,
                CORNERS::STOP_AND_REFLECT::EXIT_SPIKE,
                CORNERS::V_STITCH,
                });

            // TODO: V Stitch to Quicksand

            const RawCorner QUICKSAND_STUPID_HALFWAY_CORNER(116, 106, 184, 95, TRINKET);
            const RawCorner QUICKSAND_STUPID_CORNER(116, 106, 184, 121, TRINKET);
            const RawScenario QUICKSAND_STUPID(116, 106, 80, 22, 1, 0, {
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
                const RawCorner FLIP_UP(114, 101, 122, 46, UP_RIGHT);
                const RawCorner GO_RIGHT(114, 101, 174, 161, RIGHT_UP);
                // const corner SCREEN_EDGE(114, 101, 308, ??, X_ONLY);
            }

            namespace BISECTED_SPIRAL {
                const RawCorner FLIP_UP(115, 101, 222, 158, UP_LEFT);
            }

            // TODO more corners

            namespace I_LOVE_YOU {
                // TODO more corners
                const RawCorner WARP_TOKEN(116, 100, 152, 112, WARP_TOKEN);
            }
            namespace THATS_WHY_I_HAVE_TO_KILL_YOU {
                // TODO more corners
                const RawCorner WARP_TOKEN(114, 102, 152, 112, WARP_TOKEN);
            }
        }
        namespace SCENARIOS {
            const RawScenario TWIHTKY_STUPID_GLITCHLESS(116, 100, 40, 102, 1, 97, {
                CORNERS::I_LOVE_YOU::WARP_TOKEN,
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
            const RawScenario TWIHTKY_STUPID_DEATHWARP(114, 102, 0, 150, 1, 4, {
                CORNERS::THATS_WHY_I_HAVE_TO_KILL_YOU::WARP_TOKEN,
                });
        }
    }

    namespace LAB {
        namespace CORNERS {
            namespace GET_READY_TO_BOUNCE {
                const RawCorner DROP_DOWN(102, 116, 210, 33, DOWN_RIGHT);
            }
            namespace ITS_PERFECTLY_SAFE {
                const RawCorner START_LEFT(102, 117, 210, 22, LEFT_DOWN);
                const RawCorner CONTINUE_LEFT(102, 117, 186, 46, LEFT_DOWN);
            }
            namespace RASCASSE {
                const RawCorner FLIP_DOWN(101, 117, 62, 97, DOWN_LEFT);
                // TODO: maybe remove this phantom
                const RawCorner PHANTOM_TOUCH_GROUND(101, 117, 62, 177, DOWN_LEFT);
                const RawCorner GO_RIGHT(101, 117, 62, 126, RIGHT_DOWN);
                const RawCorner LINE_SKIP_DOWN(101, 117, 206, 97, DOWN_LEFT);
                const RawCorner LINE_SKIP_RIGHT(101, 117, 206, 97, RIGHT_DOWN);
                const RawCorner EXIT(101, 117, 218, 177, DOWN_RIGHT);
            }
            namespace KEEP_GOING {
                const RawCorner ENTRY(101, 118, 218, 14, LEFT_DOWN);
                const RawCorner GO_LEFT(101, 118, 202, 78, LEFT_DOWN);
            }
            namespace SINGLE_SLIT_EXPERIMENT {
                const RawCorner FLIP_UP(100, 118, 278, 89, UP_LEFT);
                const RawCorner EXIT(100, 118, 70, 129, DOWN_RIGHT);
            }
            namespace DONT_FLIP_OUT {
                const RawCorner GO_RIGHT(100, 119, 70, 86, RIGHT_DOWN);
            }
            namespace DOUBLE_SLIT_EXPERIMENT {
                const int rx = 102;
                const int ry = 119;
                const RawCorner ENTRY_SPIKE_CORNER(rx, ry, 66, 70, UP_RIGHT);
                const RawCorner ENTRY_DROP(rx, ry, 66, 121, DOWN_RIGHT);
                const RawCorner TOP_SHAFT_CORNER(rx, ry, 94, 22, RIGHT_DOWN);
                const RawCorner EXIT_CORNER(rx, ry, 238, 110, RIGHT_DOWN);
            }
            namespace YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER {
                const int rx = 102;
                const int ry = 118;
                const RawCorner A(rx, ry, 94, 185, RIGHT_UP);
                const RawCorner B(rx, ry, 250, 174, UP_RIGHT);
                const RawCorner C(rx, ry, 250, 65, LEFT_UP);

                const RawCorner E(rx, ry, 50, 46, LEFT_DOWN);
                const RawCorner F(rx, ry, 22, 46, UP_LEFT);

                const RawCorner TRINKET(rx, ry, 16, 24, TRINKET);
            }
            namespace YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE {
                const int rx = 102;
                const int ry = 118;

                const RawCorner A(rx, ry, 94, 185, DOWN_LEFT);
                const RawCorner B(rx, ry, 250, 174, LEFT_DOWN);
                const RawCorner C(rx, ry, 250, 65, DOWN_RIGHT);
                const RawCorner D(rx, ry, 190, 65, RIGHT_UP);

                const RawCorner F(rx, ry, 22, 46, RIGHT_DOWN);
            }
            namespace THEY_CALL_HIM_FLIPPER {
                const int rx = 103;
                const int ry = 119;
                const RawCorner ENTRY_SPIKE_CORNER(rx, ry, 130, 110, UP_RIGHT);
                const RawCorner EXIT_SPIKE_CORNER(rx, ry, 166, 97, RIGHT_UP);
            }
            namespace THREES_A_CROWD {
                const int rx = 104;
                const int ry = 119;
                const RawCorner ENTRY_DROP(rx, ry, 10, 97, DOWN_RIGHT);
                const RawCorner SPIKE_CUT(rx, ry, 30, 166, RIGHT_DOWN);
                const RawCorner LINE_CLIP_CORNER(rx, ry, 58, 166, UP_RIGHT);
            }
            namespace HITTING_THE_APEX {
                const int rx = 104;
                const int ry = 118;
                const RawCorner ENTRY_CUT(rx, ry, 94, 193, RIGHT_UP);
                const RawCorner TURNAROUND_1(rx, ry, 194, 118, UP_RIGHT);
                const RawCorner TURNAROUND_2(rx, ry, 194, 89, LEFT_UP);
            }
            namespace SQUARE_ROOT {
                const int rx = 103;
                const int ry = 118;
                const RawCorner DROP_DOWN(rx, ry, 238, 89, DOWN_LEFT);
                const RawCorner SPIKE_DOWN(rx, ry, 154, 102, LEFT_DOWN);
                const RawCorner FALL_UP_1(rx, ry, 142, 102, UP_LEFT);
                const RawCorner FALL_UP_2(rx, ry, 126, 86, UP_LEFT);
            }
            namespace THORNY_EXCHANGE {
                const int rx = 103;
                const int ry = 117;
                const RawCorner GO_RIGHT(rx, ry, 126, 153, RIGHT_UP);
                const RawCorner FALL_UP(rx, ry, 162, 46, UP_RIGHT);
            }
            namespace LETTER_G {
                const int rx = 103;
                const int ry = 116;
                const RawCorner ENTRY_LEFT(rx, ry, 162, 153, LEFT_UP);
                const RawCorner DROP_UP(rx, ry, 54, 110, UP_LEFT);
                const RawCorner GO_RIGHT(rx, ry, 54, 65, RIGHT_UP);
                const RawCorner SKIP_LINE(rx, ry, 278, 65, RIGHT_UP);
            }
            namespace FREE_YOUR_MIND {
                const int rx = 104;
                const int ry = 116;
                const RawCorner DROP(rx, ry, 66, 65, DOWN_RIGHT);
            }
            namespace IN_A_SINGLE_BOUND {
                const int rx = 105;
                const int ry = 116;
                const RawCorner A(rx, ry, 198, 49, RIGHT_UP);
                const RawCorner B(rx, ry, 218, 49, DOWN_RIGHT);
            }
            namespace BARANI_BARANI {
                const int rx = 106;
                const int ry = 116;
                const RawCorner A(rx, ry, 90, 177, DOWN_RIGHT);
                const RawCorner B(rx, ry, 98, 185, DOWN_RIGHT);
            }
            namespace SAFETY_DANCE {
                const int rx = 106;
                const int ry = 117;
                const RawCorner A(rx, ry, 198, 22, RIGHT_DOWN);
                const RawCorner B(rx, ry, 206, 30, RIGHT_DOWN);
                const RawCorner C(rx, ry, 226, 30, UP_RIGHT);
                const RawCorner D(rx, ry, 234, 22, UP_RIGHT);
            }
            namespace ENTANGLEMENT_GENERATOR {
                const int rx = 107;
                const int ry = 115;
                const RawCorner DROP_UP(rx, ry, 234, 174, UP_RIGHT);
                const RawCorner GO_LEFT(rx, ry, 234, 137, LEFT_UP);
                const RawCorner DROP_DOWN(rx, ry, 234, 137, DOWN_RIGHT);
                // Entering Garbage Room
                const RawCorner GARBAGE_ENTER_A(rx, ry, 78, 38, UP_LEFT);
                const RawCorner GARBAGE_ENTER_B(rx, ry, 50, 17, LEFT_UP);
                const RawCorner GARBAGE_ENTER_C(rx, ry, 14, 17, DOWN_LEFT);
                const RawCorner GARBAGE_ENTER_D(rx, ry, 10, 38, LEFT_DOWN);
                // Leaving Garbage Room
                const RawCorner GARBAGE_LEAVE_A(rx, ry, 78, 38, RIGHT_DOWN);
                const RawCorner GARBAGE_LEAVE_B(rx, ry, 50, 17, DOWN_LEFT);
                const RawCorner GARBAGE_LEAVE_C(rx, ry, 14, 17, RIGHT_UP);
                const RawCorner GARBAGE_LEAVE_D(rx, ry, 10, 38, UP_RIGHT);
            }
            namespace GARBAGE_ROOM_ONE_ENTER {
                const int rx = 106;
                const int ry = 115;
                // Entering
                const RawCorner A(rx, ry, 198, 49, DOWN_LEFT);
                const RawCorner B(rx, ry, 190, 57, DOWN_LEFT);
                const RawCorner C(rx, ry, 186, 70, LEFT_DOWN);
                const RawCorner D(rx, ry, 174, 105, DOWN_LEFT);
                const RawCorner E(rx, ry, 170, 182, LEFT_DOWN);
                const RawCorner F(rx, ry, 138, 198, LEFT_DOWN);
                const RawCorner G(rx, ry, 102, 198, UP_LEFT);
                const RawCorner H(rx, ry, 86, 182, UP_LEFT);
                const RawCorner I(rx, ry, 66, 97, LEFT_UP);
                const RawCorner J(rx, ry, 58, 89, LEFT_UP);
                const RawCorner K(rx, ry, 22, 89, DOWN_LEFT);
            }
            namespace GARBAGE_ROOM_ONE_LEAVE {
                const int rx = 106;
                const int ry = 115;
                // Leaving
                const RawCorner A(rx, ry, 198, 49, RIGHT_UP);
                const RawCorner B(rx, ry, 190, 57, RIGHT_UP);
                const RawCorner C(rx, ry, 186, 70, UP_RIGHT);
                const RawCorner D(rx, ry, 174, 105, RIGHT_UP);
                const RawCorner E(rx, ry, 170, 182, UP_RIGHT);
                const RawCorner F(rx, ry, 138, 198, UP_RIGHT);
                const RawCorner G(rx, ry, 102, 198, RIGHT_DOWN);
                const RawCorner H(rx, ry, 86, 182, RIGHT_DOWN);
                const RawCorner I(rx, ry, 66, 97, DOWN_RIGHT);
                const RawCorner J(rx, ry, 58, 89, DOWN_RIGHT);
                const RawCorner K(rx, ry, 22, 89, RIGHT_UP);
            }
            namespace GARBAGE_ROOM_TWO_ENTER {
                const int rx = 105;
                const int ry = 115;
                // Entering
                const RawCorner A(rx, ry, 254, 121, DOWN_LEFT);
                const RawCorner B(rx, ry, 246, 145, DOWN_LEFT);
                const RawCorner C(rx, ry, 242, 190, LEFT_DOWN);
                const RawCorner D(rx, ry, 198, 190, UP_LEFT);
                const RawCorner E(rx, ry, 182, 150, UP_LEFT);
                const RawCorner F(rx, ry, 182, 105, RIGHT_UP);
                const RawCorner G(rx, ry, 202, 94, UP_RIGHT);
                const RawCorner H(rx, ry, 210, 86, UP_RIGHT);
                const RawCorner I(rx, ry, 210, 17, LEFT_UP);
                const RawCorner J(rx, ry, 142, 17, DOWN_LEFT);
                const RawCorner K(rx, ry, 134, 33, DOWN_LEFT);
                const RawCorner L(rx, ry, 130, 118, LEFT_DOWN);
                const RawCorner M(rx, ry, 118, 129, DOWN_LEFT);
                const RawCorner N(rx, ry, 114, 166, LEFT_DOWN);
                const RawCorner O(rx, ry, 110, 177, DOWN_LEFT);
                const RawCorner P(rx, ry, 106, 198, LEFT_DOWN);
                const RawCorner Q(rx, ry, 78, 198, UP_LEFT);
                const RawCorner R(rx, ry, 74, 153, LEFT_UP);
                const RawCorner S(rx, ry, 62, 134, UP_LEFT);
                const RawCorner T(rx, ry, 58, 113, LEFT_UP);
                const RawCorner U(rx, ry, 38, 102, UP_LEFT);
                const RawCorner V(rx, ry, 22, 86, UP_LEFT);
                const RawCorner W(rx, ry, 22, 41, RIGHT_UP);

                // Trinket
                const RawCorner TRINKET(rx, ry, 72, 16, TRINKET);
            }
            namespace GARBAGE_ROOM_TWO_LEAVE {
                const int rx = 105;
                const int ry = 115;
                // Leaving
                const RawCorner A(rx, ry, 254, 121, RIGHT_UP);
                const RawCorner B(rx, ry, 246, 145, RIGHT_UP);
                const RawCorner C(rx, ry, 242, 190, UP_RIGHT);
                const RawCorner D(rx, ry, 198, 190, RIGHT_DOWN);
                const RawCorner E(rx, ry, 182, 150, RIGHT_DOWN);
                const RawCorner F(rx, ry, 182, 105, DOWN_LEFT);
                const RawCorner G(rx, ry, 202, 94, LEFT_DOWN);
                const RawCorner H(rx, ry, 210, 86, LEFT_DOWN);
                const RawCorner I(rx, ry, 210, 17, DOWN_RIGHT);
                const RawCorner J(rx, ry, 142, 17, RIGHT_UP);
                const RawCorner K(rx, ry, 134, 33, RIGHT_UP);
                const RawCorner L(rx, ry, 130, 118, UP_RIGHT);
                const RawCorner M(rx, ry, 118, 129, RIGHT_UP);
                const RawCorner N(rx, ry, 114, 166, UP_RIGHT);
                const RawCorner O(rx, ry, 110, 177, RIGHT_UP);
                const RawCorner P(rx, ry, 106, 198, UP_RIGHT);
                const RawCorner Q(rx, ry, 78, 198, RIGHT_DOWN);
                const RawCorner R(rx, ry, 74, 153, DOWN_RIGHT);
                const RawCorner S(rx, ry, 62, 134, RIGHT_DOWN);
                const RawCorner T(rx, ry, 58, 113, DOWN_RIGHT);
                const RawCorner U(rx, ry, 38, 102, RIGHT_DOWN);
                const RawCorner V(rx, ry, 22, 86, RIGHT_DOWN);
                const RawCorner W(rx, ry, 22, 41, DOWN_LEFT);
            }
            namespace HEADY_HEIGHTS {
                const int rx = 107;
                const int ry = 116;
                const RawCorner SHAFT_LEFT_SPIKE(rx, ry, 206, 113, RIGHT_UP);
                const RawCorner SHAFT_ENTRY(rx, ry, 234, 113, DOWN_RIGHT);
            }
            namespace TANTALIZING_TRINKET {
                const int rx = 107;
                const int ry = 118;
                const RawCorner ENTER(rx, ry, 138, 65, LEFT_UP);
                const RawCorner TRINKET(rx, ry, 32, 64, TRINKET);
                const RawCorner LEAVE(rx, ry, 138, 65, DOWN_RIGHT);
            }
            namespace BERNOULLI_PRINCIPLE {
                const int rx = 107;
                const int ry = 119;
                const RawCorner START_RIGHT(rx, ry, 246, 70, RIGHT_DOWN);
                const RawCorner TURNAROUND_1(rx, ry, 274, 89, DOWN_RIGHT);
                const RawCorner TURNAROUND_2(rx, ry, 274, 118, LEFT_DOWN);
                const RawCorner EXIT_DROP(rx, ry, 246, 137, DOWN_LEFT);

                const RawCorner TRINKET_ENTER_A(rx, ry, 138, 201, LEFT_UP);
                const RawCorner TRINKET_ENTER_B(rx, ry, 118, 134, UP_LEFT);
                const RawCorner TRINKET_ENTER_C(rx, ry, 118, 73, RIGHT_UP);
                const RawCorner TRINKET_ENTER_D(rx, ry, 138, 6, UP_RIGHT);

                const RawCorner TRINKET_LEAVE_A(rx, ry, 138, 201, DOWN_RIGHT);
                const RawCorner TRINKET_LEAVE_B(rx, ry, 118, 134, RIGHT_DOWN);
                const RawCorner TRINKET_LEAVE_C(rx, ry, 118, 73, DOWN_LEFT);
                const RawCorner TRINKET_LEAVE_D(rx, ry, 138, 6, LEFT_DOWN);
            }
            namespace STANDING_WAVE {
                const int rx = 107;
                const int ry = 100;
                const RawCorner START_LEFT(rx, ry, 234, 70, LEFT_DOWN);
                const RawCorner FLIP_UP(rx, ry, 198, 70, UP_LEFT);
                const RawCorner SHAFT_LEFT(rx, ry, 138, 70, LEFT_DOWN);
            }
            namespace SPIKE_STRIP_DEPLOYED {
                const int rx = 105;
                const int ry = 100;
                const RawCorner SPIKE_CORNER(rx, ry, 190, 62, UP_LEFT);
                const RawCorner LAST_SPIKE_CORNER(rx, ry, 106, 145, LEFT_UP);
            }
            namespace MERGE {
                const int rx = 103;
                const int ry = 100;
                const RawCorner TOP_RIGHT(rx, ry, 170, 94, LEFT_DOWN);
                const RawCorner BOTTOM_RIGHT(rx, ry, 170, 113, LEFT_UP);
                const RawCorner TOP_LEFT(rx, ry, 134, 94, UP_LEFT);
                const RawCorner BOTTOM_LEFT(rx, ry, 134, 113, DOWN_LEFT);
            }
            namespace IM_SORRY {
                const int rx = 101;
                const int ry = 100;
                const RawCorner CHECKPOINT_DROP(rx, ry, 222, 94, UP_LEFT);
                const RawCorner SPIKE_RIGHT(rx, ry, 106, 57, LEFT_UP);
                const RawCorner SPIKE_LEFT(rx, ry, 86, 57, DOWN_LEFT);

                const RawCorner UNOB_ENTER_A(rx, ry, 158, 153, RIGHT_UP);
                const RawCorner UNOB_ENTER_B(rx, ry, 178, 153, DOWN_RIGHT);
                const RawCorner UNOB_LEAVE_A(rx, ry, 158, 153, DOWN_LEFT);
                const RawCorner UNOB_LEAVE_B(rx, ry, 178, 153, LEFT_UP);
            }
            namespace PLEASE_FORGIVE_ME {
                const int rx = 101;
                const int ry = 101;
                const RawCorner FIRST_SPIKE_LEFT(rx, ry, 86, 102, RIGHT_DOWN);
                const RawCorner FIRST_SPIKE_RIGHT(rx, ry, 106, 102, UP_RIGHT);
                const RawCorner LAST_SPIKE_CORNER(rx, ry, 158, 102, RIGHT_DOWN);
                const RawCorner LAST_SPIKE_RIGHT(rx, ry, 178, 102, UP_RIGHT);
                const RawCorner EXIT_CORNER(rx, ry, 238, 89, RIGHT_UP);
            }
            namespace ANOMALY {
                const int rx = 105;
                const int ry = 101;
                const RawCorner UNOB_ENTER_A(rx, ry, 50, 9, DOWN_RIGHT);
                const RawCorner UNOB_ENTER_B(rx, ry, 234, 17, DOWN_RIGHT);
                const RawCorner UNOB_ENTER_C(rx, ry, 298, 41, DOWN_RIGHT);

                const RawCorner UNOB_LEAVE_A(rx, ry, 50, 9, LEFT_UP);
                const RawCorner UNOB_LEAVE_B(rx, ry, 234, 17, LEFT_UP);
                const RawCorner UNOB_LEAVE_C(rx, ry, 298, 41, LEFT_UP);
            }
            namespace PUREST_UNOBTAINIUM {
                const int rx = 106;
                const int ry = 101;
                const RawCorner TRINKET(rx, ry, 104, 128, TRINKET);
            }
            namespace PLAYING_FOOSBALL {
                const int rx = 102;
                const int ry = 101;
                const RawCorner ENTRY_DROP(rx, ry, 74, 89, DOWN_RIGHT);
            }
            namespace LIVING_DEAD_END {
                const int rx = 104;
                const int ry = 101;
                const RawCorner FIRST_LINE_DROP(rx, ry, 66, 113, DOWN_RIGHT);
                const RawCorner SECOND_LINE_DROP(rx, ry, 130, 161, DOWN_RIGHT);
            }
            namespace DIODE {
                const int rx = 104;
                const int ry = 103;
                const RawCorner LEFT_ENTRY_SPIKE(rx, ry, 130, 54, LEFT_DOWN);
                const RawCorner LEFT_TURNAROUND_1(rx, ry, 86, 81, DOWN_LEFT);
                const RawCorner LEFT_TURNAROUND_2(rx, ry, 86, 126, RIGHT_DOWN);
                const RawCorner LEFT_EXIT_SHAFT(rx, ry, 138, 185, DOWN_RIGHT);
                const RawCorner RIGHT_ENTRY_SPIKE(rx, ry, 174, 54, RIGHT_DOWN);
                const RawCorner RIGHT_TURNAROUND_1(rx, ry, 218, 81, DOWN_RIGHT);
                const RawCorner RIGHT_TURNAROUND_2(rx, ry, 218, 126, LEFT_DOWN);
                const RawCorner RIGHT_EXIT_SHAFT(rx, ry, 166, 185, DOWN_LEFT);
            }
            namespace I_SMELL_OZONE {
                const int rx = 104;
                const int ry = 104;
                const RawCorner CORNER_CUT(rx, ry, 138, 158, LEFT_DOWN);
            }
        }
        namespace SCENARIOS {
            const RawScenario IL_START_TO_RASCASSE(102, 116, 191, 33, 0, 0, {
                CORNERS::GET_READY_TO_BOUNCE::DROP_DOWN,
                CORNERS::ITS_PERFECTLY_SAFE::START_LEFT,
                CORNERS::ITS_PERFECTLY_SAFE::CONTINUE_LEFT,
                CORNERS::RASCASSE::FLIP_DOWN,
                CORNERS::RASCASSE::PHANTOM_TOUCH_GROUND,
                });

            const RawScenario IL_START_TO_DONT_FLIP_OUT(102, 116, 191, 33, 0, 0, {
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

            const RawScenario IL_START_TO_DONT_FLIP_OUT_IGNORE_LINE(102, 116, 191, 33, 0, 0, {
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

            const RawScenario RASCASSE_TO_SINGLE_SLIT(101, 117, 158, 177, 0, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::FLIP_UP,
                });

            const RawScenario RASCASSE_TO_DONT_FLIP_OUT(101, 117, 158, 177, 0, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                CORNERS::SINGLE_SLIT_EXPERIMENT::EXIT,
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
                });

            const RawScenario RASCASSE_IGNORE_LINE(101, 117, 310, 46, 1, 0, {
                CORNERS::RASCASSE::EXIT,
                CORNERS::KEEP_GOING::ENTRY,
                CORNERS::KEEP_GOING::GO_LEFT,
                });

            const RawScenario DONT_FLIP_OUT_TO_LINECLIP(100, 119, 61, 21, 0, 0, {
                CORNERS::DONT_FLIP_OUT::GO_RIGHT,
                CORNERS::THEY_CALL_HIM_FLIPPER::ENTRY_SPIKE_CORNER,
                CORNERS::THEY_CALL_HIM_FLIPPER::EXIT_SPIKE_CORNER,
                CORNERS::THREES_A_CROWD::ENTRY_DROP,
                CORNERS::THREES_A_CROWD::LINE_CLIP_CORNER,
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                });

            const RawScenario HITTING_THE_APEX_TO_LETTER_G(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::SPIKE_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP_1,
                CORNERS::SQUARE_ROOT::FALL_UP_2,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::ENTRY_LEFT,
                CORNERS::LETTER_G::DROP_UP,
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                });

            const RawScenario HITTING_THE_APEX_TO_IN_A_SINGLE_BOUND(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::SPIKE_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP_1,
                CORNERS::SQUARE_ROOT::FALL_UP_2,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::ENTRY_LEFT,
                CORNERS::LETTER_G::DROP_UP,
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                });

            const RawScenario LETTER_G_IGNORE_LINE(104, 119, 85, 41, 1, 0, {
                CORNERS::HITTING_THE_APEX::ENTRY_CUT,
                CORNERS::HITTING_THE_APEX::TURNAROUND_1,
                CORNERS::HITTING_THE_APEX::TURNAROUND_2,
                CORNERS::SQUARE_ROOT::DROP_DOWN,
                CORNERS::SQUARE_ROOT::SPIKE_DOWN,
                CORNERS::SQUARE_ROOT::FALL_UP_1,
                CORNERS::SQUARE_ROOT::FALL_UP_2,
                CORNERS::THORNY_EXCHANGE::GO_RIGHT,
                CORNERS::THORNY_EXCHANGE::FALL_UP,
                CORNERS::LETTER_G::SKIP_LINE,
                CORNERS::FREE_YOUR_MIND::DROP,
                });

            const RawScenario LETTER_G_TO_IN_A_SINGLE_BOUND(103, 116, 39, 130, 1, 0, {
                CORNERS::LETTER_G::GO_RIGHT,
                CORNERS::FREE_YOUR_MIND::DROP,
                CORNERS::IN_A_SINGLE_BOUND::A,
                CORNERS::IN_A_SINGLE_BOUND::B,
                });

            const RawScenario LETTER_G_TO_SAFETY_DANCE(103, 116, 39, 130, 1, 0, {
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
            const RawScenario LETTER_G_TO_ENTANGLEMENT_GENERATOR(103, 116, 39, 130, 1, 0, {
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
            const RawScenario LETTER_G_TO_HEADY_HEIGHTS(103, 116, 39, 130, 1, 0, {
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

            const RawScenario ENTANGLEMENT_GENERATOR_TO_BERNOULLI(107, 115, 233, 174, 1, 0, {
                CORNERS::HEADY_HEIGHTS::SHAFT_ENTRY,
                CORNERS::BERNOULLI_PRINCIPLE::START_RIGHT,
                CORNERS::BERNOULLI_PRINCIPLE::TURNAROUND_1,
                CORNERS::BERNOULLI_PRINCIPLE::TURNAROUND_2,
                CORNERS::BERNOULLI_PRINCIPLE::EXIT_DROP,
                // TODO: fix map/coord wrap-around problems
                });

            const RawScenario STANDING_WAVE_TO_MERGE(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::MERGE::BOTTOM_LEFT,
                });

            const RawScenario MERGE_TO_IM_SORRY(103, 100, 176, 131, 1, 0, {
                CORNERS::MERGE::TOP_LEFT,
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::SPIKE_RIGHT,
                CORNERS::IM_SORRY::SPIKE_LEFT,
                });

            const RawScenario PLEASE_FORGIVE_ME_TO_LIVING_DEAD_END(101, 101, 141, 69, 0, 0, {
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                });

            const RawScenario PLEASE_FORGIVE_ME_TO_LEFT_SIDE_DIODE_TO_END(101, 101, 141, 69, 0, 0, {
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

            const RawScenario PLEASE_FORGIVE_ME_TO_RIGHT_SIDE_DIODE_TO_END(101, 101, 141, 69, 0, 0, {
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

            const RawScenario YOUNG_MAN_ITS_WORTH_THE_CHALLENGE(102, 119, 0, 121, 0, 0, {
                CORNERS::DOUBLE_SLIT_EXPERIMENT::ENTRY_SPIKE_CORNER,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::A,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::C,
                // CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::E,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::F,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::TRINKET,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::F,
                // CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::E,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::C,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::A,
                CORNERS::DOUBLE_SLIT_EXPERIMENT::TOP_SHAFT_CORNER,
                });

            const RawScenario YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_TO_LINECLIP(102, 118, 50, 26, 0, 4, {
                CORNERS::DOUBLE_SLIT_EXPERIMENT::ENTRY_SPIKE_CORNER,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::A,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::C,
                // CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::E,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::F,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_ENTER::TRINKET,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::F,
                // CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::E,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::D,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::C,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::B,
                CORNERS::YOUNG_MAN_ITS_WORTH_THE_CHALLENGE_LEAVE::A,
                CORNERS::DOUBLE_SLIT_EXPERIMENT::TOP_SHAFT_CORNER,
                CORNERS::DOUBLE_SLIT_EXPERIMENT::EXIT_CORNER,
                CORNERS::THEY_CALL_HIM_FLIPPER::ENTRY_SPIKE_CORNER,
                CORNERS::THEY_CALL_HIM_FLIPPER::EXIT_SPIKE_CORNER,
                CORNERS::THREES_A_CROWD::ENTRY_DROP,
                CORNERS::THREES_A_CROWD::SPIKE_CUT,
                });

            const RawScenario GARBAGE_ROOM_TRINKET(107, 115, 233, 201, 1, 0, {
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

            const RawScenario TANTALIZING_TRINKET(107, 100, 198, 137, 0, 0, {
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

            const RawScenario TANTALIZING_TRINKET_IGNORE_LINE(107, 100, 152, 137, 1, 5, {
                CORNERS::TANTALIZING_TRINKET::ENTER,
                CORNERS::TANTALIZING_TRINKET::TRINKET,
                CORNERS::TANTALIZING_TRINKET::LEAVE,
                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                });

            const RawScenario TANTALIZING_TRINKET_TO_MERGE(107, 118, 41, 65, 0, 5, {
                CORNERS::TANTALIZING_TRINKET::LEAVE,

                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_D,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_C,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_B,
                CORNERS::BERNOULLI_PRINCIPLE::TRINKET_LEAVE_A,

                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                CORNERS::MERGE::TOP_RIGHT,
                CORNERS::MERGE::TOP_LEFT,
                });

            const RawScenario TANTALIZING_TRINKET_TO_MERGE_IGNORE_LINE(107, 118, 41, 65, 0, 5, {
                CORNERS::TANTALIZING_TRINKET::LEAVE,

                CORNERS::STANDING_WAVE::SHAFT_LEFT,
                CORNERS::MERGE::TOP_RIGHT,
                CORNERS::MERGE::TOP_LEFT,
                });

            const RawScenario IM_SORRY_UNOBTAINIUM_LDE(101, 100, 223, 94, 1, 0, {
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
            const RawScenario STANDING_WAVE_TO_IM_SORRY(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::IM_SORRY::SPIKE_RIGHT,
                CORNERS::IM_SORRY::SPIKE_LEFT,
                });

            // TODO: run this (it's almost certainly too long though)
            const RawScenario STANDING_WAVE_TO_PLEASE_FORGIVE_ME_IGNORE_LINES(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                });

            const RawScenario IM_SORRY_TO_PLEASE_FORGIVE_ME_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                });

            const RawScenario IM_SORRY_TO_LDE_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
                CORNERS::IM_SORRY::CHECKPOINT_DROP,
                CORNERS::IM_SORRY::UNOB_LEAVE_A,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_CORNER,
                CORNERS::PLEASE_FORGIVE_ME::LAST_SPIKE_RIGHT,
                CORNERS::PLEASE_FORGIVE_ME::EXIT_CORNER,
                CORNERS::PLAYING_FOOSBALL::ENTRY_DROP,
                CORNERS::LIVING_DEAD_END::FIRST_LINE_DROP,
                CORNERS::LIVING_DEAD_END::SECOND_LINE_DROP,
                });

            const RawScenario IM_SORRY_TO_LEFT_SIDE_DIODE_TO_END_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
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

            const RawScenario IM_SORRY_TO_RIGHT_SIDE_DIODE_TO_END_IGNORE_LINES(101, 100, 223, 94, 1, 0, {
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
            const RawScenario STANDING_WAVE_TO_UNOBTAINIUM_IGNORE_LINES(107, 100, 240, 19, 0, 0, {
                CORNERS::STANDING_WAVE::START_LEFT,
                CORNERS::PUREST_UNOBTAINIUM::TRINKET,
                });

            // TODO: could do hitting the apex to in a single bound
            // TODO: ideally we would check hitting the apex until entanglement generator, but that needs more optimization / a better heuristic (e.g. nearest flippable surface)
        }
    }
}

#endif /* SCENARIOS_H */
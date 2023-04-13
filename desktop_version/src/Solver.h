#ifndef SOLVER_H
#define SOLVER_H

#include <cstddef>
#include <vector>

namespace Solver {

    enum corner_dir {
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
    };

    struct corner {
        int rx;
        int ry;
        int x;
        int y;
        corner_dir dir;

        corner(int a, int b, int c, int d, corner_dir e) : rx(a), ry(b), x(c), y(d), dir(e) {}
    };

    struct Scenario {
        int init_rx;
        int init_ry;
        int init_x;
        int init_y;
        int init_gravity;
        int frames_to_advance;
        std::vector<corner> corners;

        Scenario(int a, int b, int c, int d, int e, int f, std::vector<corner> cs) : init_rx(a), init_ry(b), init_x(c), init_y(d), init_gravity(e), frames_to_advance(f), corners(cs) {}
    };

    struct statehash {
        float heuristic;
        std::size_t hash;
        int16_t f_count;
        int8_t next_corner;

        statehash(float h, std::size_t hsh, int16_t frames, int8_t corner)
        {
            heuristic = h;
            hash = hsh;
            f_count = frames;
            next_corner = corner;
        }
    };

    struct stateinfo {
        std::size_t prev_hash;
        int8_t input;

        stateinfo(std::size_t h, int8_t i) : prev_hash(h), input(i) {}
    };

    // Reduced from 44 to 14 bytes
    struct naivegamestate {
        int16_t state;          // 0 <= state <= 4099
        int8_t deathseq;        // -1 <= deathseq <= 30
        int8_t lifeseq;         // 0 <= lifeseq <= 10

        bool gravitycontrol;    // 0 <= gravitycontrol <= 1
        int8_t roomx;           // 0 <= roomx <= 120
        int8_t roomy;           // 0 <= roomy <= 120

        bool press_action;
        int8_t jumppressed;     // 0 <= jumppressed <= 5
        bool jumpheld;

        bool press_right;
        bool press_left;
        int8_t tapright;        // 0 <= tapright <= ??? (effective max is 5)
        int8_t tapleft;         // 0 <= tapleft <= ??? (effective max is 5)
    };

    // Reduced from 84 to ?? bytes
    struct naiveplayerstate {
        int16_t x;      // -20 <= x <= 320
        int16_t y;      // -20 <= x <= 320

        // TODO: these floats can be packed better
        float vx;       // -6 <= vx <= 6
        float vy;       // -10 <= vy <= 10
        float ax;       // ax in { -6, -3, 0, 3, 6 }
        float ay;       // ay in { -6, -3, 0, 3 }

        int8_t onground;   // ?? <= onground <= 2 (effective min is 0)
        int8_t onroof;     // ?? <= onroof <= 2 (effective min is 0)
        bool dir;        // 0 <= dir <= 1

        // int rule;           // -1 <= rule <= 7
        int16_t tile;           // 0 <= tile <= 711
        int8_t framedelay;     // 0 <= framedelay <= 10
        int16_t drawframe;      // -10 <= drawframe <= 721
        int8_t walkingframe;   // -5 <= walkingframe <= 5

        int8_t visualonground;     // ?? <= visualonground <= 2 (effective min is 0)
        int8_t visualonroof;       // ?? <= visualonroof <= 2 (effective min is 0)
        
        int16_t collisiondrawframe;     // 0 <= drawframe <= 720
        int8_t collisionframedelay;    // ?? <= framedelay <= 4  (effective min is 1)
        bool collisionwalkingframe;  // 0 <= walkingframe <= 1

        float newxp;    // newxp in { 0, 152, {x}, {x+vx} }
        float newyp;    // newyp in { 0, {y}, {y+vy} }
    };

    struct naiveenemystate {
        int8_t type;            // -1 <= type <= 100
        int8_t rule;            // -1 <= rule <= 7
        int16_t x;              // -20 <= x <= 320
        int16_t y;              // -20 <= y <= 240

        // TODO: these can be packed better
        float vx;
        float vy;
        float ax;               // ax in { -6, -3, 0, 3, 6 }
        float ay;               // ay in { -6, -3, 0, 3 }

        // TODO: don't we need onroof, onground, onxwall, onywall? for moving platforms
        int8_t behave;          // -1 <= behave <= 13
        float para;
        int8_t state;           // 0 <= state <= 4
        int8_t onwall;          // 0 <= onwall <= 3
        int8_t statedelay;      // 0 <= statedelay <= 120

        int16_t tile;           // 0 <= tile <= 711
        int8_t animate;         // 0 <= animate <= 100
        int8_t framedelay;      // 0 <= framedelay <= 10
        int8_t walkingframe;    // -5 <= walkingframe <= 5
        int16_t drawframe;      // -10 <= drawframe <= 721
    };

    // TODO better packing
    struct naiveblockstate {
        int rect_x;
        int rect_y;
        int rect_w;
        int rect_h;
        int type;
        int trigger;
        int xp, yp, wp, hp;
        // std::string script, prompt;
        int r, g, b;
        int activity_y;
    };

    struct naivestate {
        std::vector<naiveenemystate> entities;
        std::vector<naiveblockstate> blocks;
        naivegamestate game;
        naiveplayerstate player;

        // TODO: bitmap int32_t collect;
        bool collect[20];

        int i;

        int num_frames_in_room;
        int num_l_plus_r;
        int next_corner;

        double h;
        int f_count;
    };

    struct cacheentry {
        std::vector<std::size_t> cached_entities;
        std::vector<std::size_t> cached_blocks;
    };

    struct cachednaivestate {
        cacheentry cache_entry;
        naivegamestate game;
        naiveplayerstate player;
        bool collect[20];

        int i;

        int num_frames_in_room;
        int num_l_plus_r;
        int next_corner;

        double h;
        int f_count;
    };

	void entrypoint();

    void squish_test();

    void stateful_solver(Scenario scenario);

    void cached_stateful_solver(Scenario scenario);

    void stateless_solver(Scenario scenario, bool debug_checks);

    void debug_test(Scenario scenario);
    void debug_test_cached(Scenario scenario);

	void load_scenario(Scenario scenario);

    naivestate create_naive_state();
	void load_naive_state(naivestate s);

    cachednaivestate create_cached_naivestate();
    void load_cached_naivestate(cachednaivestate s);

    cacheentry fill_cache_entry();
    void load_cache_entry(cacheentry cache_entry);

    std::size_t cache_entity(naiveenemystate entity);
    naiveenemystate get_cached_entity(std::size_t hash);
    std::size_t cache_block(naiveblockstate block);
    naiveblockstate get_cached_block(std::size_t hash);

	void do_game_step(bool render);

    bool compare_naive_states(naivestate a, naivestate b);
    bool compare_cached_naivestates(cachednaivestate a, cachednaivestate b);
    bool compare_statehashes(statehash a, statehash b);

    void gotoroom(int rx, int ry);

    int room_warps(int room_x, int room_y);
    bool room_warpx(int room_x, int room_y);
    bool room_warpy(int room_x, int room_y);
    double get_stupid_heuristic(Scenario scenario, int next_corner, int room_x, int room_y, int player_x, int player_y);

    int room_adjusted_x(int rx, int x);
    int room_adjusted_y(int ry, int y);
    bool passed_next_corner(Scenario scenario, int next_corner, int room_x, int room_y, int player_x, int player_y);
    double get_heuristic(Scenario scenario, int next_corner, int room_x, int room_y, int player_x, int player_y);

    std::size_t hash_naivestate(naivestate s);
    std::size_t hash_cached_naivestate(cachednaivestate s);
    std::size_t hash_playerstate(naiveplayerstate s);
    std::size_t hash_gamestate(naivegamestate g);
    std::size_t hash_blocks(std::vector<naiveblockstate> bs);
    std::size_t hash_block(naiveblockstate b);
    std::size_t hash_entities(std::vector<naiveenemystate> es);
    std::size_t hash_entity(naiveenemystate e);

    std::size_t combine_hashes(std::size_t h1, std::size_t h2);
}

#endif /* SOLVER_H */

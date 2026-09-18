#include "ScenarioProbes.h"

#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Entity.h"
#include "../Game.h"
#include "../GlitchrunnerMode.h"
#include "../Graphics.h"
#include "../KeyPoll.h"
#include "../Map.h"
#include "../Music.h"
#include "../Script.h"
#include "../UtilityClass.h"
#include "../Xoshiro.h"
#include "ScenarioRand.h"

namespace scenario
{

const char* probe_type_name(const ProbeType type)
{
    switch (type)
    {
    case PT_I32: return "i32";
    case PT_U32: return "u32";
    case PT_BOOL: return "bool";
    case PT_F32: return "f32";
    case PT_STR: return "str";
    }
    return "?";
}

namespace
{

/* ------------------------------------------------------------------------
 * Probe tables
 *
 * Probe names are the C++ expressions they read, so `#expr` gives the name.
 * ------------------------------------------------------------------------ */

struct ScalarDef
{
    const char* name;
    const char* group;
    char kind; /* i: int, b: bool, f: float, s: std::string, c: const char* (read-only) */
    void* ptr;
};

#define S_I(g, expr) {#expr, g, 'i', (void*) &(expr)}
#define S_B(g, expr) {#expr, g, 'b', (void*) &(expr)}
#define S_F(g, expr) {#expr, g, 'f', (void*) &(expr)}
#define S_S(g, expr) {#expr, g, 's', (void*) &(expr)}
#define S_C(g, expr) {#expr, g, 'c', (void*) &(expr)}

struct ArrayDef
{
    const char* name;
    const char* group;
    char kind; /* i: int, b: bool */
    void* ptr;
    int count;
};

#define A_I(g, expr) {#expr, g, 'i', (void*) &(expr)[0], (int) (sizeof(expr) / sizeof((expr)[0]))}
#define A_B(g, expr) {#expr, g, 'b', (void*) &(expr)[0], (int) (sizeof(expr) / sizeof((expr)[0]))}

template <class T>
struct MemberDef
{
    const char* name;
    char kind;
    int T::* pi;
    bool T::* pb;
    float T::* pf;
    std::string T::* ps;
};

#define M_I(T, f) {#f, 'i', &T::f, NULL, NULL, NULL}
#define M_B(T, f) {#f, 'b', NULL, &T::f, NULL, NULL}
#define M_F(T, f) {#f, 'f', NULL, NULL, &T::f, NULL}
#define M_S(T, f) {#f, 's', NULL, NULL, NULL, &T::f}

/* --- game ------------------------------------------------------------- */

static const ScalarDef game_scalars[] = {
    S_I("game", game.roomx), S_I("game", game.roomy),
    S_I("game", game.prevroomx), S_I("game", game.prevroomy),
    S_I("game", game.savex), S_I("game", game.savey),
    S_I("game", game.saverx), S_I("game", game.savery),
    S_I("game", game.savegc), S_I("game", game.savedir), S_I("game", game.savecolour),
    S_I("game", game.edsavex), S_I("game", game.edsavey),
    S_I("game", game.edsaverx), S_I("game", game.edsavery),
    S_I("game", game.edsavegc), S_I("game", game.edsavedir),
    S_I("game", game.state), S_I("game", game.statedelay),
    S_B("game", game.glitchrunkludge),
    S_B("game", game.hascontrol), S_B("game", game.jumpheld),
    S_I("game", game.jumppressed),
    S_I("game", game.gravitycontrol),
    S_B("game", game.muted), S_I("game", game.mutebutton),
    S_B("game", game.musicmuted), S_I("game", game.musicmutebutton),
    S_I("game", game.tapleft), S_I("game", game.tapright),
    S_B("game", game.mapheld),
    S_I("game", game.menupage),
    S_I("game", game.lastsaved),
    S_I("game", game.deathcounts),
    S_I("game", game.framecounter),
    S_B("game", game.seed_use_sdl_getticks),
    S_B("game", game.editor_disabled),
    S_I("game", game.frames), S_I("game", game.seconds),
    S_I("game", game.minutes), S_I("game", game.hours),
    S_B("game", game.gamesaved), S_B("game", game.gamesavefailed),
    S_S("game", game.savetime),
    S_I("game", game.saveframes), S_I("game", game.saveseconds),
    S_I("game", game.savetrinkets),
    S_B("game", game.startscript), S_S("game", game.newscript),
    S_B("game", game.menustart),
    S_B("game", game.teleport_to_new_area),
    S_I("game", game.teleport_to_x), S_I("game", game.teleport_to_y),
    S_S("game", game.teleportscript),
    S_B("game", game.useteleporter),
    S_I("game", game.teleport_to_teleporter),
    S_I("game", game.currentmenuoption),
    S_B("game", game.menutestmode),
    S_I("game", game.current_credits_list_index),
    S_I("game", game.translator_credits_pagenum),
    S_I("game", game.menuxoff), S_I("game", game.menuyoff), S_I("game", game.menuspacing),
    S_I("game", game.menucountdown),
    S_I("game", game.creditposx), S_I("game", game.creditposy),
    S_I("game", game.creditposdelay), S_I("game", game.oldcreditposx),
    S_B("game", game.gpmenu_confirming), S_B("game", game.gpmenu_showremove),
    S_B("game", game.silence_settings_error),
    S_B("game", game.swnmode),
    S_I("game", game.swnstate), S_I("game", game.swnstate2),
    S_I("game", game.swnstate3), S_I("game", game.swnstate4),
    S_I("game", game.swndelay), S_I("game", game.swndeaths),
    S_I("game", game.swntimer), S_I("game", game.swncolstate), S_I("game", game.swncoldelay),
    S_I("game", game.swnrecord), S_I("game", game.swnbestrank),
    S_I("game", game.swnrank), S_I("game", game.swnmessage),
    S_B("game", game.supercrewmate), S_B("game", game.scmhurt),
    S_I("game", game.scmprogress),
    S_B("game", game.colourblindmode), S_B("game", game.noflashingmode),
    S_I("game", game.slowdown),
    S_B("game", game.nodeathmode),
    S_I("game", game.gameoverdelay),
    S_B("game", game.nocutscenes),
    S_I("game", game.ndmresultcrewrescued), S_I("game", game.ndmresulttrinkets),
    S_S("game", game.ndmresulthardestroom),
    S_I("game", game.ndmresulthardestroom_x), S_I("game", game.ndmresulthardestroom_y),
    S_B("game", game.ndmresulthardestroom_specialname),
    S_B("game", game.intimetrial), S_B("game", game.timetrialparlost),
    S_I("game", game.timetrialcountdown), S_I("game", game.timetrialshinytarget),
    S_I("game", game.timetriallevel),
    S_I("game", game.timetrialpar), S_I("game", game.timetrialresulttime),
    S_I("game", game.timetrialresultframes), S_I("game", game.timetrialrank),
    S_B("game", game.timetrialcheater),
    S_I("game", game.timetrialresultshinytarget), S_I("game", game.timetrialresulttrinkets),
    S_I("game", game.timetrialresultpar), S_I("game", game.timetrialresultdeaths),
    S_B("game", game.start_translator_exploring),
    S_B("game", game.translator_exploring),
    S_B("game", game.translator_exploring_allowtele),
    S_B("game", game.translator_cutscene_test),
    S_I("game", game.creditposition), S_I("game", game.oldcreditposition),
    S_B("game", game.insecretlab), S_B("game", game.inintermission),
    S_B("game", game.alarmon), S_I("game", game.alarmdelay),
    S_B("game", game.blackout),
    S_I("game", game.stat_trinkets),
    S_I("game", game.bestgamedeaths),
    S_I("game", game.screenshake), S_I("game", game.flashlight),
    S_B("game", game.advancetext), S_B("game", game.pausescript),
    S_I("game", game.deathseq), S_I("game", game.lifeseq),
    S_I("game", game.savepoint), S_I("game", game.teleportxpos),
    S_B("game", game.teleport),
    S_I("game", game.edteleportent),
    S_B("game", game.completestop),
    S_F("game", game.inertia),
    S_I("game", game.companion),
    S_B("game", game.activetele),
    S_I("game", game.readytotele), S_I("game", game.oldreadytotele),
    S_I("game", game.activity_r), S_I("game", game.activity_g),
    S_I("game", game.activity_b), S_I("game", game.activity_y),
    S_S("game", game.activity_lastprompt),
    S_B("game", game.activity_gettext),
    S_B("game", game.backgroundtext),
    S_I("game", game.activeactivity), S_I("game", game.act_fade), S_I("game", game.prev_act_fade),
    S_B("game", game.press_left), S_B("game", game.press_right),
    S_B("game", game.press_action), S_B("game", game.press_map),
    S_B("game", game.press_interact),
    S_B("game", game.interactheld),
    S_B("game", game.separate_interact),
    S_I("game", game.totalflips),
    S_S("game", game.hardestroom),
    S_I("game", game.hardestroomdeaths), S_I("game", game.currentroomdeaths),
    S_I("game", game.hardestroom_x), S_I("game", game.hardestroom_y),
    S_B("game", game.hardestroom_specialname),
    S_B("game", game.hardestroom_finalstretch),
    S_B("game", game.quickrestartkludge),
    S_I("game", game.customcol),
    S_I("game", game.levelpage),
    S_I("game", game.playcustomlevel),
    S_S("game", game.customleveltitle), S_S("game", game.customlevelfilename),
    S_B("game", game.skipfakeload),
    S_B("game", game.ghostsenabled),
    S_B("game", game.cliplaytest),
    S_I("game", game.playx), S_I("game", game.playy),
    S_I("game", game.playrx), S_I("game", game.playry),
    S_I("game", game.playgc), S_I("game", game.playmusic),
    S_S("game", game.playassets),
    S_B("game", game.fadetomenu), S_I("game", game.fadetomenudelay),
    S_B("game", game.fadetolab), S_I("game", game.fadetolabdelay),
    S_B("game", game.over30mode),
    S_B("game", game.showingametimer),
    S_B("game", game.ingame_titlemode), S_B("game", game.ingame_editormode),
    S_B("game", game.disablepause), S_B("game", game.disableaudiopause),
    S_B("game", game.disabletemporaryaudiopause),
    S_B("game", game.inputdelay),
    S_B("game", game.statelocked),
    S_I("game", game.old_skip_message_timer), S_I("game", game.skip_message_timer),
    S_I("game", game.old_mode_indicator_timer), S_I("game", game.mode_indicator_timer),
    S_I("game", game.old_screenshot_border_timer), S_I("game", game.screenshot_border_timer),
    S_B("game", game.screenshot_saved_success),
};

static const ArrayDef game_arrays[] = {
    A_B("game", game.crewstats),
    A_B("game", game.ndmresultcrewstats),
    A_B("game", game.unlock),
    A_B("game", game.unlocknotify),
    A_I("game", game.besttimes),
    A_I("game", game.bestframes),
    A_I("game", game.besttrinkets),
    A_I("game", game.bestlives),
    A_I("game", game.bestrank),
};

/* --- obj -------------------------------------------------------------- */

static const ScalarDef obj_scalars[] = {
    S_I("obj", obj.k),
    S_I("obj", obj.platformtile),
    S_B("obj", obj.vertplatforms), S_B("obj", obj.horplatforms),
    S_B("obj", obj.nearelephant), S_B("obj", obj.upsetmode),
    S_I("obj", obj.upset),
    S_I("obj", obj.trophytext), S_I("obj", obj.trophytype), S_I("obj", obj.oldtrophytext),
    S_I("obj", obj.altstates),
    S_I("obj", obj.customenemy), S_I("obj", obj.customplatformtile),
    S_B("obj", obj.customwarpmode), S_B("obj", obj.customwarpmodevon), S_B("obj", obj.customwarpmodehon),
    S_S("obj", obj.customscript),
    S_S("obj", obj.customactivitycolour), S_S("obj", obj.customactivitytext),
    S_I("obj", obj.customactivitypositiony),
};

static const ArrayDef obj_arrays[] = {
    A_B("obj", obj.flags),
    A_B("obj", obj.collect),
    A_B("obj", obj.customcollect),
    A_B("obj", obj.customcrewmoods),
};

static const MemberDef<entclass> entity_members[] = {
    M_B(entclass, invis),
    M_I(entclass, type), M_I(entclass, size), M_I(entclass, tile), M_I(entclass, rule),
    M_I(entclass, state), M_I(entclass, statedelay),
    M_I(entclass, behave), M_I(entclass, animate),
    M_F(entclass, para),
    M_I(entclass, life), M_I(entclass, colour),
    M_I(entclass, oldxp), M_I(entclass, oldyp),
    M_F(entclass, ax), M_F(entclass, ay), M_F(entclass, vx), M_F(entclass, vy),
    M_I(entclass, cx), M_I(entclass, cy), M_I(entclass, w), M_I(entclass, h),
    M_F(entclass, newxp), M_F(entclass, newyp),
    M_B(entclass, isplatform),
    M_I(entclass, x1), M_I(entclass, y1), M_I(entclass, x2), M_I(entclass, y2),
    M_I(entclass, onentity),
    M_B(entclass, harmful),
    M_I(entclass, onwall), M_I(entclass, onxwall), M_I(entclass, onywall),
    M_B(entclass, gravity),
    M_I(entclass, onground), M_I(entclass, onroof),
    M_I(entclass, framedelay), M_I(entclass, drawframe), M_I(entclass, walkingframe),
    M_I(entclass, dir), M_I(entclass, actionframe),
    M_I(entclass, collisionframedelay), M_I(entclass, collisiondrawframe),
    M_I(entclass, collisionwalkingframe),
    M_I(entclass, visualonground), M_I(entclass, visualonroof),
    M_I(entclass, yp), M_I(entclass, xp),
    M_I(entclass, lerpoldxp), M_I(entclass, lerpoldyp),
};

static const MemberDef<blockclass> block_members[] = {
    M_I(blockclass, type), M_I(blockclass, trigger),
    M_I(blockclass, xp), M_I(blockclass, yp), M_I(blockclass, wp), M_I(blockclass, hp),
    M_S(blockclass, script), M_S(blockclass, prompt),
    M_I(blockclass, r), M_I(blockclass, g), M_I(blockclass, b),
    M_I(blockclass, activity_y),
    M_B(blockclass, gettext),
};

/* --- map -------------------------------------------------------------- */

static const ScalarDef map_scalars[] = {
    S_B("map", map.nexttowercolour_set),
    S_I("map", map.background), S_I("map", map.rcol), S_I("map", map.tileset),
    S_B("map", map.warpx), S_B("map", map.warpy),
    S_C("map", map.roomname),
    S_B("map", map.roomname_special), S_B("map", map.roomnameset),
    S_C("map", map.hiddenname),
    S_B("map", map.towermode),
    S_I("map", map.ypos), S_I("map", map.oldypos),
    S_I("map", map.cameramode), S_I("map", map.cameraseek), S_I("map", map.cameraseekframe),
    S_I("map", map.resumedelay),
    S_B("map", map.minitowermode),
    S_I("map", map.colstatedelay), S_I("map", map.colsuperstate),
    S_I("map", map.spikeleveltop), S_I("map", map.spikelevelbottom),
    S_I("map", map.oldspikeleveltop), S_I("map", map.oldspikelevelbottom),
    S_B("map", map.finalmode), S_B("map", map.finalstretch),
    S_B("map", map.custommode), S_B("map", map.custommodeforreal),
    S_I("map", map.custommmxoff), S_I("map", map.custommmyoff),
    S_I("map", map.custommmxsize), S_I("map", map.custommmysize),
    S_I("map", map.customzoom), S_B("map", map.customshowmm),
    S_B("map", map.final_colormode), S_I("map", map.final_mapcol),
    S_I("map", map.final_aniframe), S_I("map", map.final_aniframedelay),
    S_I("map", map.final_colorframe), S_I("map", map.final_colorframedelay),
    S_B("map", map.showteleporters), S_B("map", map.showtargets), S_B("map", map.showtrinkets),
    S_B("map", map.roomtexton),
    S_I("map", map.extrarow),
    S_B("map", map.invincibility),
    S_I("map", map.cursorstate), S_I("map", map.cursordelay),
    S_B("map", map.tower.minitowermode),
};

static const ArrayDef map_arrays[] = {
    A_I("map", map.contents),
    A_I("map", map.roomdeaths),
    A_I("map", map.roomdeathsfinal),
    A_B("map", map.explored),
};

/* --- script ----------------------------------------------------------- */

static const ScalarDef script_scalars[] = {
    S_S("script", script.scriptname),
    S_I("script", script.position),
    S_I("script", script.looppoint), S_I("script", script.loopcount),
    S_I("script", script.scriptdelay),
    S_B("script", script.running),
    S_I("script", script.textx), S_I("script", script.texty),
    S_I("script", script.r), S_I("script", script.g), S_I("script", script.b),
    S_B("script", script.textflipme),
    S_B("script", script.textbuttons), S_B("script", script.textlarge),
    S_I("script", script.textboxtimer),
    S_I("script", script.i), S_I("script", script.j), S_I("script", script.k),
};

/* --- graphics --------------------------------------------------------- */

static const ScalarDef graphics_scalars[] = {
    S_I("graphics", graphics.screenshake_x), S_I("graphics", graphics.screenshake_y),
    S_I("graphics", graphics.rcol), S_I("graphics", graphics.m),
    S_B("graphics", graphics.flipmode), S_B("graphics", graphics.setflipmode),
    S_B("graphics", graphics.notextoutline),
    S_I("graphics", graphics.linestate), S_I("graphics", graphics.linedelay),
    S_I("graphics", graphics.backoffset),
    S_B("graphics", graphics.backgrounddrawn), S_B("graphics", graphics.foregrounddrawn),
    S_I("graphics", graphics.menuoffset), S_I("graphics", graphics.oldmenuoffset),
    S_B("graphics", graphics.resumegamemode),
    S_I("graphics", graphics.crewframe), S_I("graphics", graphics.crewframedelay),
    S_I("graphics", graphics.fadeamount), S_I("graphics", graphics.oldfadeamount),
    S_B("graphics", graphics.trinketcolset),
    S_I("graphics", graphics.trinketr), S_I("graphics", graphics.trinketg), S_I("graphics", graphics.trinketb),
    S_B("graphics", graphics.showcutscenebars),
    S_I("graphics", graphics.cutscenebarspos), S_I("graphics", graphics.oldcutscenebarspos),
    S_I("graphics", graphics.spcol), S_I("graphics", graphics.spcoldel),
    S_I("graphics", graphics.warpskip),
    S_B("graphics", graphics.translucentroomname),
    S_F("graphics", graphics.alpha),
    S_I("graphics", graphics.col_tr), S_I("graphics", graphics.col_tg), S_I("graphics", graphics.col_tb),
    S_B("graphics", graphics.kludgeswnlinewidth),
};

static const ArrayDef graphics_arrays[] = {
    A_I("graphics", graphics.fadebars),
};

static const ScalarDef background_scalars[] = {
    S_F("background", graphics.backboxmult),
    S_B("background", graphics.towerbg.tdrawback),
    S_I("background", graphics.towerbg.bypos), S_I("background", graphics.towerbg.bscroll),
    S_I("background", graphics.towerbg.colstate), S_I("background", graphics.towerbg.scrolldir),
    S_I("background", graphics.towerbg.r), S_I("background", graphics.towerbg.g), S_I("background", graphics.towerbg.b),
    S_B("background", graphics.titlebg.tdrawback),
    S_I("background", graphics.titlebg.bypos), S_I("background", graphics.titlebg.bscroll),
    S_I("background", graphics.titlebg.colstate), S_I("background", graphics.titlebg.scrolldir),
    S_I("background", graphics.titlebg.r), S_I("background", graphics.titlebg.g), S_I("background", graphics.titlebg.b),
};

static const ArrayDef background_arrays[] = {
    A_I("background", graphics.starsspeed),
    A_I("background", graphics.backboxvx),
    A_I("background", graphics.backboxvy),
};

static const MemberDef<textboxclass> textbox_members[] = {
    M_I(textboxclass, xp), M_I(textboxclass, yp), M_I(textboxclass, w), M_I(textboxclass, h),
    M_I(textboxclass, r), M_I(textboxclass, g), M_I(textboxclass, b),
    M_I(textboxclass, linegap),
    M_I(textboxclass, timer),
    M_F(textboxclass, tl), M_F(textboxclass, prev_tl),
    M_I(textboxclass, tm),
    M_B(textboxclass, flipme),
    M_I(textboxclass, rand),
    M_B(textboxclass, large),
    M_B(textboxclass, should_centerx), M_B(textboxclass, should_centery),
    M_B(textboxclass, fill_buttons),
    M_I(textboxclass, other_textbox_index),
};

/* --- music / help ----------------------------------------------------- */

static const ScalarDef music_scalars[] = {
    S_I("music", music.currentsong), S_I("music", music.haltedsong),
    S_B("music", music.safeToProcessMusic),
    S_I("music", music.nicechange), S_B("music", music.nicefade),
    S_B("music", music.m_doFadeInVol), S_B("music", music.m_doFadeOutVol),
    S_I("music", music.musicVolume),
    S_B("music", music.quick_fade),
    S_B("music", music.mmmmmm), S_B("music", music.usingmmmmmm),
};

static const ScalarDef help_scalars[] = {
    S_I("help", help.glow), S_I("help", help.slowsine), S_I("help", help.glowdir),
};

static const char* const all_groups[] = {
    "loop", "input", "rng", "game", "obj", "entities", "blocks", "map",
    "script", "graphics", "textboxes", "background", "music", "help",
    NULL
};

/* ------------------------------------------------------------------------
 * Collector
 * ------------------------------------------------------------------------ */

class Collector
{
public:
    Collector(State& o, const std::vector<std::string>& g, bool schema_mode)
        : out(o), groups(g), schema(schema_mode) {}

    State& out;
    const std::vector<std::string>& groups;
    bool schema;

    /* Group of the value currently being emitted; used for the schema. */
    std::vector<std::string> emitted_groups;
    const char* current_group;

    bool enabled(const char* group)
    {
        current_group = group;
        if (groups.empty())
        {
            return true;
        }
        for (size_t i = 0; i < groups.size(); ++i)
        {
            if (groups[i] == group)
            {
                return true;
            }
        }
        return false;
    }

    void push(const std::string& name, const ProbeValue& v)
    {
        out.push_back(std::make_pair(name, v));
        if (schema)
        {
            emitted_groups.push_back(current_group);
        }
    }

    void i32(const std::string& name, int v)
    {
        ProbeValue pv;
        pv.type = PT_I32;
        pv.bits = (uint32_t) v;
        push(name, pv);
    }

    void u32(const std::string& name, uint32_t v)
    {
        ProbeValue pv;
        pv.type = PT_U32;
        pv.bits = v;
        push(name, pv);
    }

    void boolean(const std::string& name, bool v)
    {
        ProbeValue pv;
        pv.type = PT_BOOL;
        pv.bits = v ? 1 : 0;
        push(name, pv);
    }

    void f32(const std::string& name, float v)
    {
        ProbeValue pv;
        pv.type = PT_F32;
        memcpy(&pv.bits, &v, sizeof(pv.bits));
        push(name, pv);
    }

    void str(const std::string& name, const std::string& v)
    {
        ProbeValue pv;
        pv.type = PT_STR;
        pv.s = v;
        push(name, pv);
    }

    static std::string idx(const std::string& base, int i)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "[%d]", i);
        return base + buf;
    }

    std::string index_name(const std::string& base, int i)
    {
        return schema ? base + "[]" : idx(base, i);
    }

    void scalars(const ScalarDef* defs, size_t n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const ScalarDef& d = defs[i];
            if (!enabled(d.group))
            {
                continue;
            }
            switch (d.kind)
            {
            case 'i': i32(d.name, *(int*) d.ptr); break;
            case 'b': boolean(d.name, *(bool*) d.ptr); break;
            case 'f': f32(d.name, *(float*) d.ptr); break;
            case 's': str(d.name, *(std::string*) d.ptr); break;
            case 'c':
            {
                const char* s = *(const char**) d.ptr;
                str(d.name, s != NULL ? s : "");
                break;
            }
            }
        }
    }

    void arrays(const ArrayDef* defs, size_t n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const ArrayDef& d = defs[i];
            if (!enabled(d.group))
            {
                continue;
            }
            const int count = schema ? 1 : d.count;
            for (int k = 0; k < count; ++k)
            {
                const std::string name = index_name(d.name, k);
                if (d.kind == 'i')
                {
                    i32(name, ((int*) d.ptr)[k]);
                }
                else
                {
                    boolean(name, ((bool*) d.ptr)[k]);
                }
            }
        }
    }

    template <class T>
    void members(const std::string& prefix, const T* obj, const MemberDef<T>* defs, size_t n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            const MemberDef<T>& d = defs[i];
            const std::string name = prefix + "." + d.name;
            switch (d.kind)
            {
            case 'i': i32(name, obj != NULL ? obj->*(d.pi) : 0); break;
            case 'b': boolean(name, obj != NULL ? obj->*(d.pb) : false); break;
            case 'f': f32(name, obj != NULL ? obj->*(d.pf) : 0.0f); break;
            case 's': str(name, obj != NULL ? obj->*(d.ps) : std::string()); break;
            }
        }
    }
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static uint32_t pack_colour(const SDL_Color& c)
{
    return ((uint32_t) c.r << 24) | ((uint32_t) c.g << 16) | ((uint32_t) c.b << 8) | (uint32_t) c.a;
}

static void rect(Collector& c, const std::string& base, const SDL_Rect* r)
{
    c.i32(base + ".x", r != NULL ? r->x : 0);
    c.i32(base + ".y", r != NULL ? r->y : 0);
    c.i32(base + ".w", r != NULL ? r->w : 0);
    c.i32(base + ".h", r != NULL ? r->h : 0);
}

static void collect(Collector& c, const LoopInfo& loop)
{
    const bool schema = c.schema;

    /* loop */
    if (c.enabled("loop"))
    {
        c.i32("loop.gamestate_func_index", loop.gamestate_func_index);
        c.i32("loop.num_gamestate_funcs", loop.num_gamestate_funcs);
        c.i32("loop.meta_func_index", loop.meta_func_index);
        c.i32("game.gamestate", (int) game.gamestate);
        c.i32("game.prevgamestate", (int) game.prevgamestate);
        c.i32("glitchrunnermode", (int) GlitchrunnerMode_get());
        c.i32("game.get_timestep()", game.get_timestep());
    }

    /* input */
    if (c.enabled("input"))
    {
        c.boolean("key.isActive", key.isActive);
        c.boolean("key.linealreadyemptykludge", key.linealreadyemptykludge);
        if (schema)
        {
            c.boolean("key.keymap[]", false);
        }
        else
        {
            /* Only held keys: KeyPoll::isDown() inserts `false` entries into
             * the std::map as a side effect, which carries no information. */
            for (std::map<SDL_Keycode, bool>::const_iterator it = key.keymap.begin(); it != key.keymap.end(); ++it)
            {
                if (it->second)
                {
                    c.boolean(Collector::idx("key.keymap", (int) it->first), true);
                }
            }
        }
    }

    /* rng */
    if (c.enabled("rng"))
    {
        if (SCENARIO_rand_is_interposed())
        {
            c.u32("rng.crt.state", SCENARIO_rand_state());
            c.u32("rng.crt.calls", SCENARIO_rand_calls());
        }
        uint32_t s[4];
        xoshiro_get_state(s);
        for (int k = 0; k < (schema ? 1 : 4); ++k)
        {
            c.u32(c.index_name("rng.xoshiro.s", k), s[k]);
        }
    }

    /* game */
    c.scalars(game_scalars, COUNT(game_scalars));
    if (c.enabled("game"))
    {
        c.i32("game.swngame", (int) game.swngame);
        c.i32("game.currentmenuname", (int) game.currentmenuname);
        c.i32("game.kludge_ingametemp", (int) game.kludge_ingametemp);
        c.i32("game.slidermode", (int) game.slidermode);
        c.i32("game.menudest", (int) game.menudest);
        c.i32("game.gpmenu_lastbutton", (int) game.gpmenu_lastbutton);
        rect(c, "game.teleblock", &game.teleblock);
    }
    c.arrays(game_arrays, COUNT(game_arrays));

    /* obj */
    c.scalars(obj_scalars, COUNT(obj_scalars));
    c.arrays(obj_arrays, COUNT(obj_arrays));
    if (c.enabled("entities"))
    {
        c.i32("obj.entities.len", schema ? 0 : (int) obj.entities.size());
        const size_t count = schema ? 1 : obj.entities.size();
        for (size_t k = 0; k < count; ++k)
        {
            const std::string base = c.index_name("obj.entities", (int) k);
            const entclass* e = schema ? NULL : &obj.entities[k];
            c.members(base, e, entity_members, COUNT(entity_members));
            c.u32(base + ".realcol", e != NULL ? pack_colour(e->realcol) : 0);
        }
    }
    if (c.enabled("blocks"))
    {
        c.i32("obj.blocks.len", schema ? 0 : (int) obj.blocks.size());
        const size_t count = schema ? 1 : obj.blocks.size();
        for (size_t k = 0; k < count; ++k)
        {
            const std::string base = c.index_name("obj.blocks", (int) k);
            const blockclass* b = schema ? NULL : &obj.blocks[k];
            c.members(base, b, block_members, COUNT(block_members));
            rect(c, base + ".rect", b != NULL ? &b->rect : NULL);
        }
    }

    /* map */
    c.scalars(map_scalars, COUNT(map_scalars));
    c.arrays(map_arrays, COUNT(map_arrays));
    if (c.enabled("map"))
    {
        c.i32("map.teleporters.len", schema ? 0 : (int) map.teleporters.size());
        for (size_t k = 0; k < (schema ? 1 : map.teleporters.size()); ++k)
        {
            const std::string base = c.index_name("map.teleporters", (int) k);
            c.i32(base + ".x", schema ? 0 : map.teleporters[k].x);
            c.i32(base + ".y", schema ? 0 : map.teleporters[k].y);
        }
        c.i32("map.shinytrinkets.len", schema ? 0 : (int) map.shinytrinkets.size());
        for (size_t k = 0; k < (schema ? 1 : map.shinytrinkets.size()); ++k)
        {
            const std::string base = c.index_name("map.shinytrinkets", (int) k);
            c.i32(base + ".x", schema ? 0 : map.shinytrinkets[k].x);
            c.i32(base + ".y", schema ? 0 : map.shinytrinkets[k].y);
        }
        c.i32("map.roomtext.len", schema ? 0 : (int) map.roomtext.size());
        for (size_t k = 0; k < (schema ? 1 : map.roomtext.size()); ++k)
        {
            const std::string base = c.index_name("map.roomtext", (int) k);
            const Roomtext* rt = schema ? NULL : &map.roomtext[k];
            c.i32(base + ".x", rt != NULL ? rt->x : 0);
            c.i32(base + ".y", rt != NULL ? rt->y : 0);
            c.str(base + ".text", rt != NULL && rt->text != NULL ? rt->text : "");
            c.boolean(base + ".rtl", rt != NULL ? rt->rtl : false);
        }
        c.i32("map.specialroomnames.len", schema ? 0 : (int) map.specialroomnames.size());
        for (size_t k = 0; k < (schema ? 1 : map.specialroomnames.size()); ++k)
        {
            const std::string base = c.index_name("map.specialroomnames", (int) k);
            const Roomname* rn = schema ? NULL : &map.specialroomnames[k];
            c.i32(base + ".x", rn != NULL ? rn->x : 0);
            c.i32(base + ".y", rn != NULL ? rn->y : 0);
            c.boolean(base + ".loop", rn != NULL ? rn->loop : false);
            c.i32(base + ".flag", rn != NULL ? rn->flag : 0);
            c.i32(base + ".type", rn != NULL ? (int) rn->type : 0);
            c.i32(base + ".progress", rn != NULL ? rn->progress : 0);
            c.i32(base + ".delay", rn != NULL ? rn->delay : 0);
            c.i32(base + ".text.len", rn != NULL ? (int) rn->text.size() : 0);
        }
    }

    /* script */
    c.scalars(script_scalars, COUNT(script_scalars));
    if (c.enabled("script"))
    {
        c.i32("script.textcase", (int) script.textcase);
        c.i32("script.commands.len", schema ? 0 : (int) script.commands.size());
        c.i32("script.txt.len", schema ? 0 : (int) script.txt.size());
    }

    /* graphics */
    c.scalars(graphics_scalars, COUNT(graphics_scalars));
    c.arrays(graphics_arrays, COUNT(graphics_arrays));
    if (c.enabled("graphics"))
    {
        c.i32("graphics.fademode", (int) graphics.fademode);
        c.i32("graphics.ingame_fademode", (int) graphics.ingame_fademode);
    }
    if (c.enabled("textboxes"))
    {
        c.i32("graphics.textboxes.len", schema ? 0 : (int) graphics.textboxes.size());
        const size_t count = schema ? 1 : graphics.textboxes.size();
        for (size_t k = 0; k < count; ++k)
        {
            const std::string base = c.index_name("graphics.textboxes", (int) k);
            const textboxclass* tb = schema ? NULL : &graphics.textboxes[k];
            c.members(base, tb, textbox_members, COUNT(textbox_members));
            c.u32(base + ".print_flags", tb != NULL ? tb->print_flags : 0);
            c.i32(base + ".lines.len", tb != NULL ? (int) tb->lines.size() : 0);
            const size_t nlines = schema ? 1 : tb->lines.size();
            for (size_t l = 0; l < nlines; ++l)
            {
                c.str(c.index_name(base + ".lines", (int) l), tb != NULL ? tb->lines[l] : "");
            }
        }
    }

    /* background */
    c.scalars(background_scalars, COUNT(background_scalars));
    c.arrays(background_arrays, COUNT(background_arrays));
    if (c.enabled("background"))
    {
        for (int k = 0; k < (schema ? 1 : Graphics::numstars); ++k)
        {
            rect(c, c.index_name("graphics.stars", k), schema ? NULL : &graphics.stars[k]);
        }
        for (int k = 0; k < (schema ? 1 : Graphics::numbackboxes); ++k)
        {
            rect(c, c.index_name("graphics.backboxes", k), schema ? NULL : &graphics.backboxes[k]);
        }
    }

    /* music, help */
    c.scalars(music_scalars, COUNT(music_scalars));
    c.scalars(help_scalars, COUNT(help_scalars));
}

/* ------------------------------------------------------------------------
 * Writable probes ([start.set])
 * ------------------------------------------------------------------------ */

static bool parse_index(const std::string& name, std::string& base, int& index, std::string& rest)
{
    size_t open = name.find('[');
    if (open == std::string::npos)
    {
        return false;
    }
    size_t close = name.find(']', open);
    if (close == std::string::npos || close == open + 1)
    {
        return false;
    }
    char* end = NULL;
    const std::string num = name.substr(open + 1, close - open - 1);
    long v = strtol(num.c_str(), &end, 10);
    if (end == NULL || *end != '\0' || v < 0)
    {
        return false;
    }
    base = name.substr(0, open);
    index = (int) v;
    rest = name.substr(close + 1);
    return true;
}

static bool write_value(char kind, void* ptr, const TomlValue& v, const std::string& name, std::string& error)
{
    switch (kind)
    {
    case 'i':
        if (v.kind != TomlValue::INTEGER)
        {
            error = name + " expects an integer";
            return false;
        }
        *(int*) ptr = (int) v.i;
        return true;
    case 'b':
        if (v.kind != TomlValue::BOOLEAN)
        {
            error = name + " expects true or false";
            return false;
        }
        *(bool*) ptr = v.b;
        return true;
    case 'f':
        if (v.kind == TomlValue::FLOAT)
        {
            *(float*) ptr = (float) v.f;
        }
        else if (v.kind == TomlValue::INTEGER)
        {
            *(float*) ptr = (float) v.i;
        }
        else
        {
            error = name + " expects a number";
            return false;
        }
        return true;
    case 's':
        if (v.kind != TomlValue::STRING)
        {
            error = name + " expects a string";
            return false;
        }
        *(std::string*) ptr = v.s;
        return true;
    }
    error = name + " is read-only";
    return false;
}

static const ScalarDef* find_scalar(const std::string& name)
{
    static const struct { const ScalarDef* defs; size_t n; } tables[] = {
        {game_scalars, COUNT(game_scalars)},
        {obj_scalars, COUNT(obj_scalars)},
        {map_scalars, COUNT(map_scalars)},
        {script_scalars, COUNT(script_scalars)},
        {graphics_scalars, COUNT(graphics_scalars)},
        {background_scalars, COUNT(background_scalars)},
        {music_scalars, COUNT(music_scalars)},
        {help_scalars, COUNT(help_scalars)},
    };
    for (size_t t = 0; t < COUNT(tables); ++t)
    {
        for (size_t i = 0; i < tables[t].n; ++i)
        {
            if (name == tables[t].defs[i].name)
            {
                return &tables[t].defs[i];
            }
        }
    }
    return NULL;
}

static const ArrayDef* find_array(const std::string& name)
{
    static const struct { const ArrayDef* defs; size_t n; } tables[] = {
        {game_arrays, COUNT(game_arrays)},
        {obj_arrays, COUNT(obj_arrays)},
        {map_arrays, COUNT(map_arrays)},
        {graphics_arrays, COUNT(graphics_arrays)},
        {background_arrays, COUNT(background_arrays)},
    };
    for (size_t t = 0; t < COUNT(tables); ++t)
    {
        for (size_t i = 0; i < tables[t].n; ++i)
        {
            if (name == tables[t].defs[i].name)
            {
                return &tables[t].defs[i];
            }
        }
    }
    return NULL;
}

template <class T>
static bool write_member(T& target, const MemberDef<T>* defs, size_t n, const std::string& field, const TomlValue& v, const std::string& name, std::string& error)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (field != defs[i].name)
        {
            continue;
        }
        switch (defs[i].kind)
        {
        case 'i': return write_value('i', &(target.*(defs[i].pi)), v, name, error);
        case 'b': return write_value('b', &(target.*(defs[i].pb)), v, name, error);
        case 'f': return write_value('f', &(target.*(defs[i].pf)), v, name, error);
        case 's': return write_value('s', &(target.*(defs[i].ps)), v, name, error);
        }
    }
    error = "unknown or read-only probe '" + name + "'";
    return false;
}

} /* namespace */

const std::vector<ProbeFamily>& probe_families(void)
{
    static std::vector<ProbeFamily> families;
    if (families.empty())
    {
        State state;
        std::vector<std::string> all;
        Collector c(state, all, true);
        LoopInfo loop = {0, 0, 0};
        collect(c, loop);
        for (size_t i = 0; i < state.size(); ++i)
        {
            ProbeFamily f;
            f.name = state[i].first;
            f.type = state[i].second.type;
            f.group = c.emitted_groups[i];
            families.push_back(f);
        }
    }
    return families;
}

const std::vector<std::string>& probe_groups(void)
{
    static std::vector<std::string> groups;
    if (groups.empty())
    {
        for (const char* const* g = all_groups; *g != NULL; ++g)
        {
            groups.push_back(*g);
        }
    }
    return groups;
}

void collect_state(State& out, const std::vector<std::string>& groups, const LoopInfo& loop)
{
    out.clear();
    Collector c(out, groups, false);
    collect(c, loop);
}

bool set_probe(const std::string& name, const TomlValue& value, std::string& error)
{
    const ScalarDef* s = find_scalar(name);
    if (s != NULL)
    {
        return write_value(s->kind, s->ptr, value, name, error);
    }

    std::string base, rest;
    int index;
    if (!parse_index(name, base, index, rest))
    {
        error = "unknown or read-only probe '" + name + "'";
        return false;
    }

    if (rest.empty())
    {
        const ArrayDef* a = find_array(base);
        if (a == NULL)
        {
            error = "unknown or read-only probe '" + name + "'";
            return false;
        }
        if (index >= a->count)
        {
            error = name + ": index out of range";
            return false;
        }
        void* ptr = a->kind == 'i' ? (void*) &((int*) a->ptr)[index] : (void*) &((bool*) a->ptr)[index];
        return write_value(a->kind, ptr, value, name, error);
    }

    if (rest[0] != '.')
    {
        error = "unknown or read-only probe '" + name + "'";
        return false;
    }
    const std::string field = rest.substr(1);

    if (base == "obj.entities")
    {
        if (index >= (int) obj.entities.size())
        {
            error = name + ": no such entity";
            return false;
        }
        return write_member(obj.entities[index], entity_members, COUNT(entity_members), field, value, name, error);
    }
    if (base == "obj.blocks")
    {
        if (index >= (int) obj.blocks.size())
        {
            error = name + ": no such block";
            return false;
        }
        return write_member(obj.blocks[index], block_members, COUNT(block_members), field, value, name, error);
    }

    error = "unknown or read-only probe '" + name + "'";
    return false;
}

void append_json_string(std::string& out, const std::string& s)
{
    out += '"';
    for (size_t i = 0; i < s.size(); ++i)
    {
        const unsigned char ch = (unsigned char) s[i];
        switch (ch)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (ch < 0x20)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", ch);
                out += buf;
            }
            else
            {
                out += (char) ch;
            }
        }
    }
    out += '"';
}

void append_json_value(std::string& out, const ProbeValue& v)
{
    char buf[48];
    switch (v.type)
    {
    case PT_I32:
        snprintf(buf, sizeof(buf), "%d", (int32_t) v.bits);
        out += buf;
        break;
    case PT_U32:
        snprintf(buf, sizeof(buf), "%u", (unsigned) v.bits);
        out += buf;
        break;
    case PT_BOOL:
        out += v.bits ? "true" : "false";
        break;
    case PT_F32:
    {
        float f;
        memcpy(&f, &v.bits, sizeof(f));
        if (f != f)
        {
            out += "\"nan\"";
        }
        else if (f == HUGE_VALF)
        {
            out += "\"inf\"";
        }
        else if (f == -HUGE_VALF)
        {
            out += "\"-inf\"";
        }
        else
        {
            /* 9 significant digits round-trip every float exactly. */
            snprintf(buf, sizeof(buf), "%.9g", (double) f);
            out += buf;
        }
        break;
    }
    case PT_STR:
        append_json_string(out, v.s);
        break;
    }
}

} /* namespace scenario */

#include "angband.h"

#include <assert.h>

/************************************************************************
 * Give birth to a new player.
 * TODO: quickstart/welcome/beginner & monster mode/birth options
 *
 * *_ui() functions run menu loops according to the following flow graph:
 ***********************************************************************/
extern int py_birth(void);
    static int _welcome_ui(void);
    static int _race_class_ui(void);
        static void _pers_ui(void);
        static void _race_group_ui(void);
            static int _race_ui(int ids[]);
                static int _subrace_ui(void);
                    static int _demigod_ui(void);
                    static int _draconian_ui(void);
        static void _class_group_ui(void);
            static int _class_ui(int ids[]);
                static int _subclass_ui(void);
                    static int _warlock_ui(void);
                    static int _weaponmaster_ui(void);
                    static int _devicemaster_ui(void);
                    static int _gray_mage_ui(void);
        static int _realm1_ui(void);
            static int _realm2_ui(void);
    static int _stats_ui(void);

/* I prefer to render menus to a document rather than directly to the terminal */
static doc_ptr _doc = NULL;

static int _inkey(void);
static void _sync_term(doc_ptr doc);

/************************************************************************
 * Public Entrypoint
 ***********************************************************************/ 
int py_birth(void)
{
    int result = UI_OK;

    assert(!_doc);
    _doc = doc_alloc(80);

    msg_line_clear();
    Term_clear();
    Term_save();
    result = _welcome_ui();
    Term_load();

    doc_free(_doc);
    _doc = NULL;

    return result;
}

/************************************************************************
 * Welcome to Poschengband!
 ***********************************************************************/ 
static void _set_mode(int mode);

static int _welcome_ui(void)
{
    for (;;)
    {
        int cmd;

        doc_clear(_doc);

        doc_insert(_doc,
            "Welcome to <color:keyword>Poschengband</color>, a dungeon exploration "
            "role playing game. Your goal is to defeat the dreaded <color:keyword>"
            "Serpent of Chaos</color>, but before you can face it, you must battle "
            "many foes. Your first step is to create a character for this quest. "
            "The following screens will guide you through this process so that you "
            "may quickly begin playing. You can get general help at any time by "
            "pressing <color:keypress>?</color>.\n\n"
        );

        doc_insert(_doc,
            "First, you must decide what type of game to play. If you are new, "
            "it is recommended you play in <color:keyword>Beginner Mode</color>.\n\n"
        );

        doc_insert(_doc, "<color:G>Choose the Type of Game to Play</color>\n");
        doc_insert(_doc, "  <color:y>b</color>) Beginner\n");
        doc_insert(_doc, "  <color:y>n</color>) Normal\n");
        doc_insert(_doc, "  <color:y>m</color>) Monster\n");
        doc_newline(_doc);
        doc_insert(_doc, "  <color:y>?</color>) Help\n");
        doc_insert(_doc, "<color:y>ESC</color>) <color:v>Quit</color>\n");


        doc_newline(_doc);
        doc_insert(_doc, "<color:G>Tip:</color> <indent>You can often get specific "
                         "help by entering the uppercase letter for a command. For "
                         "example, type <color:keypress>B</color> on this screen "
                         "to receive help on Beginner Mode.</indent>");
        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == '?')
            doc_display_help("birth.txt", "Welcome");
        else if (cmd == ESCAPE)
            return UI_CANCEL;
        else if (cmd == 'n')
        {
            _set_mode(GAME_MODE_NORMAL);
            if (_race_class_ui() == UI_OK)
                return UI_OK;
        }
        else if (cmd == 'N')
            doc_display_help("birth.txt", "NormalMode");
        else if (cmd == 'b')
        {
            _set_mode(GAME_MODE_BEGINNER);
            if (_race_class_ui() == UI_OK)
                return UI_OK;
        }
        else if (cmd == 'B')
            doc_display_help("birth.txt", "BeginnerMode");
    }
}

static void _stats_init(void);

static void _set_mode(int mode)
{
    static bool first = TRUE;
    if (mode == GAME_MODE_BEGINNER)
    {
        if (first || game_mode != mode)
        {
            p_ptr->prace = RACE_HOBBIT;
            p_ptr->psubrace = 0;
            p_ptr->pclass = CLASS_ROGUE;
            p_ptr->psubclass = 0;
            p_ptr->realm1 = REALM_BURGLARY;
            p_ptr->realm2 = REALM_NONE;
            p_ptr->dragon_realm = DRAGON_REALM_NONE;
            p_ptr->personality = PERS_ORDINARY;
            _stats_init();
        }
    }
    else if (mode == GAME_MODE_NORMAL)
    {
        if (first || game_mode == GAME_MODE_MONSTER)
        {
            p_ptr->prace = RACE_HOBBIT;
            p_ptr->psubrace = 0;
            p_ptr->pclass = CLASS_ROGUE;
            p_ptr->psubclass = 0;
            p_ptr->realm1 = REALM_BURGLARY;
            p_ptr->realm2 = REALM_NONE;
            p_ptr->dragon_realm = DRAGON_REALM_NONE;
            p_ptr->personality = PERS_ORDINARY;
            _stats_init();
        }
    }
    else if (mode == GAME_MODE_MONSTER)
    {
        if (first || game_mode != mode)
        {
            p_ptr->prace = RACE_MON_TROLL;
            p_ptr->psubrace = TROLL_ETTIN;
            p_ptr->pclass = CLASS_MONSTER;
            p_ptr->psubclass = 0;
            p_ptr->realm1 = REALM_NONE;
            p_ptr->realm2 = REALM_NONE;
            p_ptr->dragon_realm = DRAGON_REALM_NONE;
            _stats_init();
        }
    }
    game_mode = mode;
    first = FALSE;
}

/************************************************************************
 * 2) Race/Class/Personality
 *
 * Note: It assumed througout that the player starts with a valid
 * race, class and personality, and that throughout the birth process
 * these fields remain legal. Currently, player_wipe gives an Ordinary
 * Human Warrior to start.
 ***********************************************************************/ 
static void _race_class_top(doc_ptr doc);
static void _inc_rcp_state(void);
static void _change_name(void);

static int _beginner_races[] = {
  RACE_HUMAN,
  RACE_DWARF,
  RACE_HOBBIT,
  RACE_GNOME,
  RACE_DUNADAN,
  RACE_HIGH_ELF,
  RACE_HALF_TROLL,
  -1
};

static int _beginner_classes[] = {
  CLASS_WARRIOR,
  CLASS_MAGE,
  CLASS_PRIEST,
  CLASS_ROGUE,
  CLASS_RANGER,
  CLASS_PALADIN,
  CLASS_NINJA,
  -1
};

static int _race_class_ui(void)
{
    for (;;)
    {
        int cmd;
        doc_ptr cols[2];

        doc_clear(_doc);
        _race_class_top(_doc);

        cols[0] = doc_alloc(30);
        cols[1] = doc_alloc(46);

        doc_insert(cols[0], "  <color:y>n</color>) Change Name\n");
        doc_insert(cols[0], "  <color:y>s</color>) Change Sex\n");
        if (game_mode != GAME_MODE_BEGINNER)
            doc_insert(cols[0], "  <color:y>p</color>) Change Personality\n");
        doc_insert(cols[0], "  <color:y>r</color>) Change Race\n");
        doc_insert(cols[0], "  <color:y>c</color>) Change Class\n");
        if (p_ptr->realm1)
            doc_insert(cols[0], "  <color:y>m</color>) Change Magic\n");

        doc_insert(cols[1], "<color:y>  ?</color>) Help\n");
        doc_insert(cols[1], "<color:y>TAB</color>) More Info\n");
        doc_insert(cols[1], "<color:y>RET</color>) Next Screen\n");
        doc_insert(cols[1], "<color:y>ESC</color>) Prev Screen\n");

        doc_insert_cols(_doc, cols, 2, 1);
        doc_free(cols[0]);
        doc_free(cols[1]);

        doc_insert(_doc, "<color:G>Tip:</color> <indent>You can often get specific "
                         "help by entering the uppercase letter for a command. For "
                         "example, type <color:keypress>R</color> on this screen "
                         "to receive help on your currently selected race.</indent>");
        _sync_term(_doc);

        cmd = _inkey();
        switch (cmd)
        {
        case '\r':
            if (_stats_ui() == UI_OK)
                return UI_OK;
            break;
        case ESCAPE:
            return UI_CANCEL;
        case '\t':
            _inc_rcp_state();
            break;
        case '?':
            doc_display_help("birth.txt", NULL);
            break;
        case 'n':
            _change_name();
            break;
        case 'r':
            if (game_mode == GAME_MODE_BEGINNER)
                _race_ui(_beginner_races);
            else
                _race_group_ui();
            break;
        case 'R':
        {
            race_t *race_ptr = get_race();
            if (p_ptr->prace == RACE_DEMIGOD)
                doc_display_help("Demigods.txt", race_ptr->subname);
            else if (p_ptr->prace == RACE_DRACONIAN)
                doc_display_help("Draconians.txt", race_ptr->subname);
            else
                doc_display_help("Races.txt", race_ptr->name);
            break;
        }
        case 'c':
            if (game_mode == GAME_MODE_BEGINNER)
                _class_ui(_beginner_classes);
            else
                _class_group_ui();
            break;
        case 'C':
        {
            class_t *class_ptr = get_class();
            if (p_ptr->pclass == CLASS_WARLOCK)
                doc_display_help("Warlocks.txt", class_ptr->subname);
            else
                doc_display_help("Classes.txt", class_ptr->name);
            break;
        }
        case 'p':
            if (game_mode != GAME_MODE_BEGINNER)
                _pers_ui();
            break;
        case 'P':
        {
            if (game_mode != GAME_MODE_BEGINNER)
            {
                personality_ptr pers_ptr = get_personality();
                doc_display_help("Personalities.txt", pers_ptr->name);
            }
            break;
        }
        case 'm':
            if (p_ptr->realm1)
                _realm1_ui();
            break;
        case 'M':
            if (p_ptr->realm1)
                doc_display_help("magic.txt", realm_names[p_ptr->realm1]);
            break;
        case 's':
            if (p_ptr->psex == SEX_MALE)
            {
                if (p_ptr->personality != PERS_LUCKY)
                    p_ptr->psex = SEX_FEMALE;
            }
            else if (p_ptr->personality != PERS_SEXY)
                p_ptr->psex = SEX_MALE;
            break;
        }
    }
}

/************************************************************************
 * 2.1) Race
 ***********************************************************************/ 
static vec_ptr _get_races_aux(int ids[]);
static bool _is_valid_race_class(int race_id, int class_id);

#define _MAX_RACES_PER_GROUP 23
#define _MAX_RACE_GROUPS      8
typedef struct _race_group_s {
    cptr name;
    int ids[_MAX_RACES_PER_GROUP];
} _race_group_t, *_race_group_ptr;
static _race_group_t _race_groups[_MAX_RACE_GROUPS] = {
    { "Human",
        {RACE_AMBERITE, RACE_BARBARIAN, RACE_DEMIGOD, RACE_DUNADAN, RACE_HUMAN, -1} },
    { "Elf",
        {RACE_DARK_ELF, RACE_HIGH_ELF, RACE_WOOD_ELF, -1} },
    { "Hobbit/Dwarf",
        {RACE_DWARF, RACE_GNOME, RACE_HOBBIT, RACE_NIBELUNG, -1} },
    { "Fairy",
        {RACE_SHADOW_FAIRY, RACE_SPRITE, -1} },
    { "Angel/Demon",
        {RACE_ARCHON, RACE_BALROG, RACE_IMP, -1} },
    { "Orc/Troll/Giant",
        {RACE_CYCLOPS, RACE_HALF_GIANT, RACE_HALF_OGRE,
         RACE_HALF_TITAN, RACE_HALF_TROLL, RACE_KOBOLD, RACE_SNOTLING, -1} },
    { "Undead",
        {RACE_SKELETON, RACE_SPECTRE, RACE_VAMPIRE, RACE_ZOMBIE, -1} },
    { "Other",
        {RACE_ANDROID, RACE_BEASTMAN, RACE_CENTAUR, RACE_DRACONIAN, RACE_DOPPELGANGER, RACE_ENT,
         RACE_GOLEM, RACE_KLACKON, RACE_KUTAR, RACE_MIND_FLAYER, RACE_TONBERRY, RACE_YEEK,-1 } },
};

static void _race_group_ui(void)
{
    vec_ptr groups = vec_alloc(NULL);
    int     i;

    for (i = 0; i < _MAX_RACE_GROUPS; i++)
    {
        _race_group_ptr g_ptr = &_race_groups[i];
        vec_ptr         races = _get_races_aux(g_ptr->ids);

        if (vec_length(races))
            vec_add(groups, g_ptr);
        vec_free(races);
    }

    for (;;)
    {
        int cmd;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose a Type of Race to Play</color>\n");
        for (i = 0; i < vec_length(groups); i++)
        {
            _race_group_ptr g_ptr = vec_get(groups, i);
            doc_printf( _doc, "  <color:y>%c</color>) %s\n", I2A(i), g_ptr->name);
        }
        _sync_term(_doc);

        cmd = _inkey();

        if (cmd == ESCAPE) break;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Races.txt", NULL);
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(groups))
            {
                _race_group_ptr g_ptr = vec_get(groups, i);
                if (_race_ui(g_ptr->ids) == UI_OK) break;
            }
        }
    }
    vec_free(groups);
}

static int _race_ui(int ids[])
{
    vec_ptr v = _get_races_aux(ids);
    int     result = UI_NONE;

    while (result == UI_NONE)
    {
        int cmd, i, split = vec_length(v);
        doc_ptr cols[2];

        doc_clear(_doc);
        _race_class_top(_doc);

        cols[0] = doc_alloc(30);
        cols[1] = doc_alloc(30);

        if (split > 10)
            split = (split + 1)/2;
        for (i = 0; i < vec_length(v); i++)
        {
            race_t *race_ptr = vec_get(v, i);
            doc_printf(
                cols[i < split ? 0 : 1],
                "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                race_ptr->id == p_ptr->prace ? 'B' : 'w',
                race_ptr->name
            );
        }

        doc_insert(_doc, "<color:G>Choose Your Race</color>\n");
        doc_insert_cols(_doc, cols, 2, 1);
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        doc_free(cols[0]);
        doc_free(cols[1]);

        _sync_term(_doc);

        cmd = _inkey();

        if (cmd == ESCAPE) result = UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Races.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < vec_length(v))
            {
                race_t *race_ptr = vec_get(v, i);
                doc_display_help("Races.txt", race_ptr->name);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(v))
            {
                int     old_id = p_ptr->prace;
                race_t *race_ptr = vec_get(v, i);

                if (p_ptr->prace != race_ptr->id)
                {
                    p_ptr->prace = race_ptr->id;
                    p_ptr->psubrace = 0;
                }
                result = _subrace_ui();
                if (result == UI_CANCEL)
                {
                    p_ptr->prace = old_id;
                    result = UI_NONE;
                }
            }
        }
    }
    vec_free(v);
    return result;
}

static vec_ptr _get_races_aux(int ids[])
{
    vec_ptr v = vec_alloc(NULL);
    int     i;

    for (i = 0; ; i++)
    {
        int id = ids[i];
        if (id == -1) break;
        if (!_is_valid_race_class(id, p_ptr->pclass)) continue;
        vec_add(v, get_race_aux(id, 0));
    }

    return v;
}

static bool _is_valid_race_class(int race_id, int class_id)
{
    if (class_id == CLASS_BLOOD_KNIGHT || class_id == CLASS_BLOOD_MAGE)
    {
        if (get_race_aux(race_id, 0)->flags & RACE_IS_NONLIVING)
            return FALSE;
    }
    if (class_id == CLASS_BEASTMASTER || class_id == CLASS_CAVALRY)
    {
        if (race_id == RACE_CENTAUR)
            return FALSE;
    }
    if (class_id == CLASS_DUELIST || class_id == CLASS_MAULER)
    {
        if (race_id == RACE_TONBERRY)
            return FALSE;
    }
    return TRUE;
}

/************************************************************************
 * 2.1.1) Subrace
 ***********************************************************************/ 
static int _subrace_ui(void)
{
    if (p_ptr->prace == RACE_DEMIGOD)
        return _demigod_ui();
    else if (p_ptr->prace == RACE_DRACONIAN)
        return _draconian_ui();
    else
    {
        p_ptr->psubrace = 0;
        return UI_OK;
    }
}

static int _demigod_ui(void)
{
    assert(p_ptr->prace == RACE_DEMIGOD);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Demigod Parentage</color>\n");
        for (i = 0; i < DEMIGOD_MAX; i++)
        {
            race_t *race_ptr = get_race_aux(p_ptr->prace, i);
            doc_printf(_doc, "  <color:y>%c</color>) %s\n", I2A(i), race_ptr->subname);
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Demigods.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < DEMIGOD_MAX)
            {
                race_t *race_ptr = get_race_aux(p_ptr->prace, i);
                doc_display_help("Demigods.txt", race_ptr->subname);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < DEMIGOD_MAX)
            {
                p_ptr->psubrace = i;
                return UI_OK;
            }
        }
    }
    return UI_OK;
}

static int _draconian_ui(void)
{
    assert(p_ptr->prace == RACE_DRACONIAN);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Draconian Subrace</color>\n");
        for (i = 0; i < DRACONIAN_MAX; i++)
        {
            race_t *race_ptr = get_race_aux(p_ptr->prace, i);
            doc_printf(_doc, "  <color:y>%c</color>) %s\n", I2A(i), race_ptr->subname);
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Draconians.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < DRACONIAN_MAX)
            {
                race_t *race_ptr = get_race_aux(p_ptr->prace, i);
                doc_display_help("Draconians.txt", race_ptr->subname);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < DRACONIAN_MAX)
            {
                p_ptr->psubrace = i;
                return UI_OK;
            }
        }
    }
}

/************************************************************************
 * 2.2) Class
 ***********************************************************************/ 
static vec_ptr _get_classes_aux(int ids[])
{
    vec_ptr v = vec_alloc(NULL);
    int     i;

    for (i = 0; ; i++)
    {
        int id = ids[i];
        if (id == -1) break;
        if (!_is_valid_race_class(p_ptr->prace, id)) continue;
        vec_add(v, get_class_aux(id, 0));
    }

    return v;
}

#define _MAX_CLASSES_PER_GROUP 20
#define _MAX_CLASS_GROUPS      11
typedef struct _class_group_s {
    cptr name;
    int ids[_MAX_CLASSES_PER_GROUP];
} _class_group_t, *_class_group_ptr;
static _class_group_t _class_groups[_MAX_CLASS_GROUPS] = {
    { "Melee", {CLASS_BERSERKER, CLASS_BLOOD_KNIGHT, CLASS_DUELIST, CLASS_MAULER,
                    CLASS_RUNE_KNIGHT, CLASS_SAMURAI, CLASS_WARRIOR, CLASS_WEAPONMASTER,
                    CLASS_WEAPONSMITH, -1} },
    { "Archery", {CLASS_ARCHER, CLASS_SNIPER, -1} },
    { "Martial Arts", {CLASS_FORCETRAINER, CLASS_MONK, CLASS_MYSTIC, -1} },
    { "Magic", {CLASS_BLOOD_MAGE, CLASS_BLUE_MAGE, CLASS_GRAY_MAGE, CLASS_HIGH_MAGE, CLASS_MAGE,
                    CLASS_NECROMANCER, CLASS_SORCERER, CLASS_YELLOW_MAGE, -1} },
    { "Devices", {CLASS_DEVICEMASTER, CLASS_MAGIC_EATER, -1} },
    { "Prayer", {CLASS_PRIEST, -1} },
    { "Stealth", {CLASS_NINJA, CLASS_ROGUE, CLASS_SCOUT, -1} },
    { "Hybrid", {CLASS_CHAOS_WARRIOR, CLASS_PALADIN, CLASS_RANGER, CLASS_RED_MAGE,
                    CLASS_WARRIOR_MAGE, -1} },
    { "Riding", {CLASS_BEASTMASTER, CLASS_CAVALRY, -1} },
    { "Mind", {CLASS_MINDCRAFTER, CLASS_MIRROR_MASTER, CLASS_PSION,
                    CLASS_TIME_LORD, CLASS_WARLOCK, -1} },
    { "Other", {CLASS_ARCHAEOLOGIST, CLASS_BARD, CLASS_IMITATOR, CLASS_RAGE_MAGE,
                    CLASS_TOURIST, CLASS_WILD_TALENT, -1} },
};

static void _class_group_ui(void)
{
    vec_ptr groups = vec_alloc(NULL);
    int     i;

    for (i = 0; i < _MAX_CLASS_GROUPS; i++)
    {
        _class_group_ptr g_ptr = &_class_groups[i];
        vec_ptr          classes = _get_classes_aux(g_ptr->ids);

        if (vec_length(classes))
            vec_add(groups, g_ptr);
        vec_free(classes);
    }

    for (;;)
    {
        int cmd;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose a Type of Class to Play</color>\n");
        for (i = 0; i < vec_length(groups); i++)
        {
            _class_group_ptr g_ptr = vec_get(groups, i);
            doc_printf( _doc, "  <color:y>%c</color>) %s\n", I2A(i), g_ptr->name);
        }
        _sync_term(_doc);

        cmd = _inkey();

        if (cmd == ESCAPE) break;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Classes.txt", NULL);
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(groups))
            {
                _class_group_ptr g_ptr = vec_get(groups, i);
                if (_class_ui(g_ptr->ids) == UI_OK)
                    break;
            }
        }
    }
    vec_free(groups);
}

static int _class_ui(int ids[])
{
    vec_ptr v = _get_classes_aux(ids);
    int     result = UI_NONE;

    while (result == UI_NONE)
    {
        int cmd, i, split = vec_length(v);
        doc_ptr cols[2];

        doc_clear(_doc);
        _race_class_top(_doc);

        cols[0] = doc_alloc(30);
        cols[1] = doc_alloc(30);

        if (split > 10)
            split = (split + 1)/2;
        for (i = 0; i < vec_length(v); i++)
        {
            class_t *class_ptr = vec_get(v, i);
            doc_printf(
                cols[i < split ? 0 : 1],
                "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                class_ptr->id == p_ptr->pclass ? 'B' : 'w',
                class_ptr->name
            );
        }

        doc_insert(_doc, "<color:G>Choose Your Class</color>\n");
        doc_insert_cols(_doc, cols, 2, 1);
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        doc_free(cols[0]);
        doc_free(cols[1]);

        _sync_term(_doc);

        cmd = _inkey();

        if (cmd == ESCAPE) result = UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Classes.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < vec_length(v))
            {
                class_t *class_ptr = vec_get(v, i);
                doc_display_help("Classes.txt", class_ptr->name);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(v))
            {
                class_t *class_ptr = vec_get(v, i);
                int      old_id = p_ptr->pclass;

                p_ptr->pclass = class_ptr->id;
                result = _subclass_ui();
                if (result == UI_CANCEL)
                {
                    p_ptr->pclass = old_id;
                    result = UI_NONE;
                }
            }
        }
    }
    vec_free(v);
    return result;
}

/************************************************************************
 * 2.2.1) Subclass
 ***********************************************************************/ 
static int _subclass_ui(void)
{
    for (;;)
    {
        int rc;
        bool has_subclass = TRUE;

        /* Prompt for a subclass */
        if (p_ptr->pclass == CLASS_WARLOCK)
            rc = _warlock_ui();
        else if (p_ptr->pclass == CLASS_WEAPONMASTER)
            rc = _weaponmaster_ui();
        else if (p_ptr->pclass == CLASS_DEVICEMASTER)
            rc = _devicemaster_ui();
        else if (p_ptr->pclass == CLASS_GRAY_MAGE)
            rc = _gray_mage_ui();
        else
        {
            p_ptr->psubclass = 0;
            rc = UI_OK;
            has_subclass = FALSE;
        }
        /* Cancel subclass returns to _class_ui */
        if (rc == UI_CANCEL) return UI_CANCEL;

        /* Prompt for magic */
        rc =_realm1_ui();

        /* Cancel magic stays here if there is a sublass ui */
        if (rc == UI_OK || !has_subclass) return rc;
    }
}

static int _warlock_ui(void)
{
    assert(p_ptr->pclass == CLASS_WARLOCK);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Warlock Pact</color>\n");
        for (i = 0; i < WARLOCK_MAX; i++)
        {
            class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                p_ptr->psubclass == i ? 'B' : 'w',
                class_ptr->subname
            );
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Warlocks.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < WARLOCK_MAX)
            {
                class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
                doc_display_help("Warlocks.txt", class_ptr->subname);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < WARLOCK_MAX)
            {
                p_ptr->psubclass = i;
                return UI_OK;
            }
        }
    }
}

static int _weaponmaster_ui(void)
{
    assert(p_ptr->pclass == CLASS_WEAPONMASTER);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Speciality</color>\n");
        for (i = 0; i < WEAPONMASTER_MAX; i++)
        {
            class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                p_ptr->psubclass == i ? 'B' : 'w',
                class_ptr->subname
            );
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Weaponmasters.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < WEAPONMASTER_MAX)
            {
                class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
                doc_display_help("Weaponmasters.txt", class_ptr->subname);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < WEAPONMASTER_MAX)
            {
                p_ptr->psubclass = i;
                return UI_OK;
            }
        }
    }
}

static int _devicemaster_ui(void)
{
    assert(p_ptr->pclass == CLASS_DEVICEMASTER);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Speciality</color>\n");
        for (i = 0; i < DEVICEMASTER_MAX; i++)
        {
            class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                p_ptr->psubclass == i ? 'B' : 'w',
                class_ptr->subname
            );
        }

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Classes.txt", "Devicemaster");
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < DEVICEMASTER_MAX)
            {
                p_ptr->psubclass = i;
                return UI_OK;
            }
        }
    }
}

static int _gray_mage_ui(void)
{
    assert(p_ptr->pclass == CLASS_GRAY_MAGE);
    for (;;)
    {
        int cmd, i;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Bias</color>\n");
        for (i = 0; i < GRAY_MAGE_MAX; i++)
        {
            class_t *class_ptr = get_class_aux(p_ptr->pclass, i);
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                p_ptr->psubclass == i ? 'B' : 'w',
                class_ptr->subname
            );
        }

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE) return UI_CANCEL;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Classes.txt", "Gray-Mage");
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < GRAY_MAGE_MAX)
            {
                p_ptr->psubclass = i;
                return UI_OK;
            }
        }
    }
}

/************************************************************************
 * 2.2.2) Magic
 ***********************************************************************/ 
static int _realm1_ui(void)
{
    u32b bits = realm_choices1[p_ptr->pclass];
    int  choices[MAX_REALM];
    int  ct = 0, i;

    if (!bits)
    {
        p_ptr->realm1 = 0;
        p_ptr->realm2 = 0;
        return UI_OK;
    }

    for (i = 0; i < 32; i++)
    {
        if (bits & (1L << i))
            choices[ct++] = i+1;
    }
    choices[ct] = -1;

    for (;;)
    {
        int cmd;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Your Primary Magic Realm</color>\n");
        for (i = 0; i < ct; i++)
        {
            int id = choices[i];
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                id == p_ptr->realm1 ? 'B' : 'w',
                realm_names[id]
            );
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE)
            return UI_CANCEL;
        else if (cmd == '?')
            doc_display_help("magic.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                int id = choices[i];
                doc_display_help("magic.txt", realm_names[id]);
            }
        }
        else
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                int id = choices[i];
                int old_id = p_ptr->realm1;
                int rc;

                p_ptr->realm1 = id;
                rc = _realm2_ui();
                if (rc == UI_CANCEL)
                    p_ptr->realm1 = old_id;
                else
                    return UI_OK;
            }
        }
    }
}

static int _realm2_ui(void)
{
    u32b bits = realm_choices2[p_ptr->pclass];
    int  choices[MAX_REALM];
    int  ct = 0, i;

    if (!bits)
    {
        p_ptr->realm2 = 0;
        return UI_OK;
    }

    if (p_ptr->pclass == CLASS_PRIEST)
    {
        if (is_good_realm(p_ptr->realm1))
            bits &= ~(CH_DEATH | CH_DAEMON);
        else
            bits &= ~(CH_LIFE | CH_CRUSADE);
    }

    for (i = 0; i < 32; i++)
    {
        int id = i + 1;
        if (bits & (1L << i) && p_ptr->realm1 != id)
            choices[ct++] = id;
    }
    choices[ct] = -1;

    for (;;)
    {
        int cmd;

        doc_clear(_doc);
        _race_class_top(_doc);

        doc_insert(_doc, "<color:G>Choose Your Secondary Magic Realm</color>\n");
        for (i = 0; i < ct; i++)
        {
            int id = choices[i];
            doc_printf(_doc, "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                id == p_ptr->realm2 ? 'B' : 'w',
                realm_names[id]
            );
        }
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == ESCAPE)
            return UI_CANCEL;
        else if (cmd == '?')
            doc_display_help("magic.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                int id = choices[i];
                doc_display_help("magic.txt", realm_names[id]);
            }
        }
        else
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < ct)
            {
                int id = choices[i];
                p_ptr->realm2 = id;
                return UI_OK;
            }
        }
    }
}

/************************************************************************
 * 2.3) Personality
 ***********************************************************************/ 
static vec_ptr _pers_choices(void);

static void _pers_ui(void)
{
    vec_ptr v = _pers_choices();
    for (;;)
    {
        int cmd, i, split = vec_length(v);
        doc_ptr cols[2];

        doc_clear(_doc);
        _race_class_top(_doc);

        cols[0] = doc_alloc(20);
        cols[1] = doc_alloc(20);

        if (split > 10)
            split = (split + 1)/2;
        for (i = 0; i < vec_length(v); i++)
        {
            personality_ptr pers_ptr = vec_get(v, i);
            doc_printf(
                cols[i < split ? 0 : 1],
                "  <color:y>%c</color>) <color:%c>%s</color>\n",
                I2A(i),
                pers_ptr->id == p_ptr->personality ? 'B' : 'w',
                pers_ptr->name
            );
        }

        doc_insert(_doc, "<color:G>Choose Your Personality</color>\n");
        doc_insert_cols(_doc, cols, 2, 1);
        doc_insert(_doc, "     Use SHIFT+choice to display help topic\n");

        doc_free(cols[0]);
        doc_free(cols[1]);

        _sync_term(_doc);

        cmd = _inkey();

        if (cmd == ESCAPE) break;
        else if (cmd == '\t') _inc_rcp_state();
        else if (cmd == '?') doc_display_help("Personalities.txt", NULL);
        else if (isupper(cmd))
        {
            i = A2I(tolower(cmd));
            if (0 <= i && i < vec_length(v))
            {
                personality_ptr pers_ptr = vec_get(v, i);
                doc_display_help("Personalities.txt", pers_ptr->name);
            }
        }
        else
        {
            i = A2I(cmd);
            if (0 <= i && i < vec_length(v))
            {
                personality_ptr pers_ptr = vec_get(v, i);
                p_ptr->personality = pers_ptr->id;
                break;
            }
        }
    }
    vec_free(v);
}

static int _pers_cmp(personality_ptr l, personality_ptr r)
{
    return strcmp(l->name, r->name);
}

static vec_ptr _pers_choices(void)
{
    vec_ptr v = vec_alloc(NULL);
    int i;
    for (i = 0; i < MAX_PERSONALITIES; i++)
    {
        personality_ptr pers_ptr = get_personality_aux(i);
        if (p_ptr->psex == SEX_MALE && i == PERS_SEXY) continue;
        if (p_ptr->psex == SEX_FEMALE && i == PERS_LUCKY) continue;
        vec_add(v, pers_ptr);
    }
    vec_sort(v, (vec_cmp_f)_pers_cmp);
    return v;
}

/************************************************************************
 * 3) Stats
 ***********************************************************************/ 

static int _stat_points[18] =
{ 0, 0, 0,
 -8,-7,-6,   /*  3  4  5 */
 -5,-4,-3,   /*  6  7  8 */
 -2,-1, 0,   /*  9 10 11 */
  1, 2, 3,   /* 12 13 14 */
  6,10,16 }; /* 15 16 17 */

#define _MAX_SCORE 30

static int _stats_score(void);
static void _stat_dec(int which);
static void _stat_inc(int which);
static cptr _stat_desc(int stat);
static bool _stats_changed = FALSE;

static cptr _stat_names[MAX_STATS] = { "STR", "INT", "WIS", "DEX", "CON", "CHR" };
static char _stat_to_char(int which);
static int _char_to_stat(char which);

static int _stats_ui(void)
{
    race_t         *race_ptr = get_race();
    class_t        *class_ptr = get_class();
    personality_ptr pers_ptr = get_personality();

    /* Initialize stats with reasonable defaults.
       If the user backs up and changes their class,
       pick new defaults (unless they manually made
       changes. */
    if (!_stats_changed)
        _stats_init();

    for (;;)
    {
        int cmd, score, i;
        doc_ptr cols[2];

        cols[0] = doc_alloc(32);
        cols[1] = doc_alloc(30);

        doc_clear(_doc);
        _race_class_top(_doc);

        score = _stats_score();
        doc_insert(cols[0], "<color:G>Enter Your Starting Stats</color>\n");
        doc_insert(cols[0], "<tab:9>Base  Pts  Mod  Total\n");
        for (i = 0; i < MAX_STATS; i++)
        {
            int stat = p_ptr->stat_cur[i];
            int bonus = pers_ptr->stats[i] + race_ptr->stats[i] + class_ptr->stats[i];
            doc_printf(cols[0], "<color:y>%c</color>/<color:y>%c</color>) %-3.3s ",
                _stat_to_char(i),
                toupper(_stat_to_char(i)),
                _stat_names[i]
            );
            doc_printf(cols[0], "%4d  <color:w>%3d</color>  <color:R>%+3d</color> %6.6s\n",
                stat,
                _stat_points[stat],
                bonus,
                _stat_desc(stat + bonus)
            );
        }
        doc_printf(cols[0], "<tab:15><color:%c>%3d</color>\n",
            score <= _MAX_SCORE ? 'G' : 'r', score);

        doc_newline(cols[1]);
        doc_newline(cols[1]);
        doc_insert(cols[1], "<color:y>  n</color>) Change Name\n");
        doc_insert(cols[1], "<color:y>  ?</color>) Help\n");
        doc_insert(cols[1], "<color:y>TAB</color>) More Info\n");
        doc_printf(cols[1], "<color:%c>RET</color>) <color:%c>Begin Play</color>\n",
            score <= _MAX_SCORE ? 'y' : 'D',
            score <= _MAX_SCORE ? 'v' : 'D');
        doc_insert(cols[1], "<color:y>ESC</color>) Prev Screen\n");

        doc_insert_cols(_doc, cols, 2, 1);
        doc_free(cols[0]);
        doc_free(cols[1]);

        doc_insert(_doc,
            "<color:G>Note: </color><indent>"
            "You may adjust each starting stat using the indicated "
            "keys. The lower case key decrements while the upper case increments "
            "the starting value. Values may range from 8 to 17 and each value that "
            "you enter is charged the indicated number of points. You only have 30 "
            "points to spend so be sure to adjust those stats that matter most."
            "</indent>");

        _sync_term(_doc);
        cmd = _inkey();
        if (cmd == '\r' && score <= _MAX_SCORE)
            return UI_OK;
        else if (cmd == ESCAPE)
            return UI_CANCEL;
        else if (cmd == '\t')
            _inc_rcp_state();
        else if (cmd == 'n')
            _change_name();
        else if (isupper(cmd))
        {
            i = _char_to_stat(tolower(cmd));
            if (i != A_NONE)
                _stat_inc(i);
        }
        else
        {
            i = _char_to_stat(cmd);
            if (i != A_NONE)
                _stat_dec(i);
        }
    }
}

static char _stat_cmd_map[MAX_STATS] = { 's', 'i', 'w', 'd', 'c', 'r' };
static char _stat_to_char(int which)
{
    return _stat_cmd_map[which];
}

static int _char_to_stat(char which)
{
    int i;
    for (i = 0; i < MAX_STATS; i++)
    {
        char c = _stat_cmd_map[i];
        if (c == which) return i;
    }
    return A_NONE;
}

static cptr _stat_desc(int stat)
{
    static char buf[10];
    if (stat < 3) stat = 3;
    if (stat > 40) stat = 40;
    if (stat < 19)
        sprintf(buf, "%2d", stat);
    else
        sprintf(buf, "18/%d", 10*(stat - 18));
    return buf;
}

static void _stats_init_aux(int stats[MAX_STATS])
{
    int i;
    for (i = 0; i < MAX_STATS; i++)
    {
        p_ptr->stat_cur[i] = stats[i];
        p_ptr->stat_max[i] = stats[i];
    }
}

static void _stats_init(void)
{
    switch (p_ptr->pclass)
    {
    case CLASS_WARRIOR:
    case CLASS_CAVALRY:
    case CLASS_BERSERKER:
    case CLASS_MAULER:
    case CLASS_ARCHER:
    case CLASS_SAMURAI:
    case CLASS_WEAPONSMITH:
    case CLASS_WEAPONMASTER:
    case CLASS_NINJA:
    case CLASS_SNIPER:
    case CLASS_DUELIST:
    case CLASS_RAGE_MAGE:
    {
        int stats[6] = { 17, 8, 8, 17, 15, 9 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_BLOOD_KNIGHT:
    {
        int stats[6] = { 17, 8, 8, 15, 17, 9 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
    case CLASS_BLUE_MAGE:
    case CLASS_GRAY_MAGE:
    case CLASS_YELLOW_MAGE:
    case CLASS_MIRROR_MASTER:
    case CLASS_BLOOD_MAGE:
    case CLASS_NECROMANCER:
    {
        int stats[6] = { 16, 17, 9, 9, 16, 9 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_PRIEST:
    case CLASS_RANGER:
    case CLASS_PALADIN:
    case CLASS_MINDCRAFTER:
    case CLASS_FORCETRAINER:
    case CLASS_TIME_LORD:
    case CLASS_ARCHAEOLOGIST:
    {
        int stats[6] = { 16, 8, 17, 16, 11, 8 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_ROGUE:
    case CLASS_WARRIOR_MAGE:
    case CLASS_CHAOS_WARRIOR:
    case CLASS_TOURIST:
    case CLASS_MAGIC_EATER:
    case CLASS_RED_MAGE:
    case CLASS_RUNE_KNIGHT:
    case CLASS_SCOUT:
    case CLASS_DEVICEMASTER:
    {
        int stats[6] = { 16, 16, 9, 16, 14, 10 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_MONK:
    {
        int stats[6] = { 16, 8, 16, 17, 11, 8 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_MYSTIC:
    {
        int stats[6] = { 16, 8, 8, 17, 11, 16 };
        _stats_init_aux(stats);
        break;
    }
    case CLASS_BEASTMASTER:
    case CLASS_BARD:
    {
        int stats[6] = { 16, 8, 8, 16, 11, 17 };
        _stats_init_aux(stats);
        break;
    }
    /* TODO */
    case CLASS_WARLOCK:
    case CLASS_WILD_TALENT:
    case CLASS_PSION:
    case CLASS_MONSTER:
    default:
    {
        int stats[6] = { 15, 15, 15, 15, 15, 11 };
        _stats_init_aux(stats);
    }
    }
    _stats_changed = FALSE;
}

static void _stat_dec(int which)
{
    int val = p_ptr->stat_cur[which];
    if (val > 8)
        val--;
    else
        val = 17;
    p_ptr->stat_cur[which] = val;
    p_ptr->stat_max[which] = val;
    _stats_changed = TRUE;
}

static void _stat_inc(int which)
{
    int val = p_ptr->stat_cur[which];
    if (val < 17)
        val++;
    else
        val = 8;
    p_ptr->stat_cur[which] = val;
    p_ptr->stat_max[which] = val;
    _stats_changed = TRUE;
}

static int _stats_score()
{
    int i, score = 0;

    for (i = 0; i < MAX_STATS; i++)
        score += _stat_points[p_ptr->stat_cur[i]];

    return score;
}

/************************************************************************
 * Low level utilities
 ***********************************************************************/ 
/* The UI is organized as follows:
    <player fields> | <info panel>
   -----------------------------------
              <menu choices>
   -----------------------------------
              <tips and help>
   ----------------------------------- */

static void _race_class_header(doc_ptr doc);
static void _race_class_info(doc_ptr doc);

static void _race_class_top(doc_ptr doc)
{
    doc_ptr cols[2];

    cols[0] = doc_alloc(26);
    cols[1] = doc_alloc(54);

    _race_class_header(cols[0]);
    _race_class_info(cols[1]);
    doc_insert_cols(doc, cols, 2, 1);

    doc_free(cols[0]);
    doc_free(cols[1]);
}

/* Player Fields */
static void _name_line(doc_ptr doc);
static void _sex_line(doc_ptr doc);
static void _pers_line(doc_ptr doc);
static void _race_line(doc_ptr doc);
static void _class_line(doc_ptr doc);
static void _magic_line(doc_ptr doc);

static void _race_class_header(doc_ptr doc)
{
    _name_line(doc);
    _sex_line(doc);
    if (game_mode != GAME_MODE_BEGINNER)
        _pers_line(doc);
    _race_line(doc);
    _class_line(doc);
    _magic_line(doc);
}

static void _name_line(doc_ptr doc)
{
    doc_printf(doc, "Name : <color:B>%s</color>\n", player_name);
}

static void _sex_line(doc_ptr doc)
{
    doc_printf(doc, "Sex  : <color:B>%s</color>\n", sex_info[p_ptr->psex].title);
}

static void _pers_line(doc_ptr doc)
{
    personality_ptr pers_ptr = get_personality();
    doc_printf(doc, "Pers : <color:B>%s</color>\n", pers_ptr->name);
}

static void _race_line(doc_ptr doc)
{
    race_t *race_ptr = get_race();
    doc_insert(doc, "Race : <indent><color:B>");
    if (race_ptr->subname)
        doc_printf(doc, "%s ", race_ptr->subname);
    doc_printf(doc, "%s</indent></color>\n", race_ptr->name);
}

static void _class_line(doc_ptr doc)
{
    class_t *class_ptr = get_class();
    doc_printf(doc, "Class: <color:B>%s</color>\n", class_ptr->name);
    if (class_ptr->subname)
    {
        if (p_ptr->pclass == CLASS_WARLOCK)
            doc_printf(doc, "Pact : <color:B>%s</color>\n", class_ptr->subname);
        else if (p_ptr->pclass == CLASS_WEAPONMASTER || p_ptr->pclass == CLASS_DEVICEMASTER)
            doc_printf(doc, "Spec : <color:B>%s</color>\n", class_ptr->subname);
        else if (p_ptr->pclass == CLASS_GRAY_MAGE)
            doc_printf(doc, "Bias : <color:B>%s</color>\n", class_ptr->subname);
        else
            doc_printf(doc, "       <color:B>%s</color>\n", class_ptr->subname);
    }
}

static void _magic_line(doc_ptr doc)
{
    if (!p_ptr->realm1) return;
    doc_printf(doc, "Magic: <color:B>%s</color>\n", realm_names[p_ptr->realm1]);
    if (p_ptr->realm2)
        doc_printf(doc, "       <color:B>%s</color>\n", realm_names[p_ptr->realm2]);
}

/* The Info Panel displays Stats and Skills as
   the player builds the character. Since this
   won't all fit in 80x27, we let the user <TAB>
   through the following sections */
enum {
    _RCP_STATS,
    _RCP_SKILLS1,
    _RCP_SKILLS2,
    _RCP_COUNT
};
static int _rcp_state = _RCP_STATS;

/* <TAB> rotates the state of the Info Panel */
static void _inc_rcp_state(void)
{
    ++_rcp_state;
    if (_rcp_state == _RCP_COUNT)
        _rcp_state = _RCP_STATS;
}

static void _race_class_info(doc_ptr doc)
{
    race_t *race_ptr = get_race();
    class_t *class_ptr = get_class();
    personality_ptr pers_ptr = get_personality();

    if (_rcp_state == _RCP_STATS)
    {
        doc_insert(doc, "<style:heading><color:w>STR  INT  WIS  DEX  CON  CHR  Life  BHP  Exp</color>\n");
        if (game_mode != GAME_MODE_BEGINNER)
        {
            doc_printf(doc, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%       %3d%%\n",
                pers_ptr->stats[A_STR], pers_ptr->stats[A_INT], pers_ptr->stats[A_WIS],
                pers_ptr->stats[A_DEX], pers_ptr->stats[A_CON], pers_ptr->stats[A_CHR],
                pers_ptr->life, pers_ptr->exp);
        }
        doc_printf(doc, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %+3d  %3d%%\n",
            race_ptr->stats[A_STR], race_ptr->stats[A_INT], race_ptr->stats[A_WIS],
            race_ptr->stats[A_DEX], race_ptr->stats[A_CON], race_ptr->stats[A_CHR],
            race_ptr->life, race_ptr->base_hp, race_ptr->exp);
        doc_printf(doc, "%+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %+3d  %3d%%\n",
            class_ptr->stats[A_STR], class_ptr->stats[A_INT], class_ptr->stats[A_WIS],
            class_ptr->stats[A_DEX], class_ptr->stats[A_CON], class_ptr->stats[A_CHR],
            class_ptr->life, class_ptr->base_hp, class_ptr->exp);
        doc_printf(doc, "<color:R>%+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %+3d  %3d%%</color>\n",
            race_ptr->stats[A_STR] + class_ptr->stats[A_STR] + pers_ptr->stats[A_STR],
            race_ptr->stats[A_INT] + class_ptr->stats[A_INT] + pers_ptr->stats[A_INT],
            race_ptr->stats[A_WIS] + class_ptr->stats[A_WIS] + pers_ptr->stats[A_WIS],
            race_ptr->stats[A_DEX] + class_ptr->stats[A_DEX] + pers_ptr->stats[A_DEX],
            race_ptr->stats[A_CON] + class_ptr->stats[A_CON] + pers_ptr->stats[A_CON],
            race_ptr->stats[A_CHR] + class_ptr->stats[A_CHR] + pers_ptr->stats[A_CHR],
            race_ptr->life * class_ptr->life * pers_ptr->life / 10000,
            race_ptr->base_hp + class_ptr->base_hp,
            race_ptr->exp * class_ptr->exp * pers_ptr->exp / 10000
        );
        doc_insert(doc, "</style>");
    }
    else if (_rcp_state == _RCP_SKILLS1 || _rcp_state == _RCP_SKILLS2)
    {
        skills_desc_t r_desc, c_desc, p_desc, tot_desc;
        skills_t      base, xtra;

        base = class_ptr->base_skills;
        skills_add(&base, &race_ptr->skills);
        skills_add(&base, &pers_ptr->skills);

        xtra = pers_ptr->skills;
        skills_scale(&xtra, 1, 5);
        skills_add(&xtra, &class_ptr->extra_skills);

        skills_desc_race(race_ptr, &r_desc);
        skills_desc_class(class_ptr, &c_desc);
        skills_desc_pers(pers_ptr, &p_desc);
        skills_desc_aux(&base, &xtra, &tot_desc);

        if (_rcp_state == _RCP_SKILLS1)
        {
            doc_printf(doc, "<color:w>%-10.10s %-10.10s %-10.10s %-10.10s</color>\n",
                "Disarming", "Device", "Save", "Stealth");
            if (game_mode != GAME_MODE_BEGINNER)
            {
                doc_printf(doc, "%s %s %s %s\n",
                    p_desc.dis, p_desc.dev, p_desc.sav, p_desc.stl);
            }
            doc_printf(doc, "%s %s %s %s\n", /* Note: descriptions contain formatting tags, so %-10.10s won't work ... cf skills.c */
                r_desc.dis, r_desc.dev, r_desc.sav, r_desc.stl);
            doc_printf(doc, "%s %s %s %s\n",
                c_desc.dis, c_desc.dev, c_desc.sav, c_desc.stl);
            doc_printf(doc, "%s %s %s %s\n",
                tot_desc.dis, tot_desc.dev, tot_desc.sav, tot_desc.stl);
        }
        else
        {
            doc_printf(doc, "<color:w>%-10.10s %-10.10s %-10.10s %-10.10s</color>\n",
                "Searching", "Perception", "Melee", "Bows");
            if (game_mode != GAME_MODE_BEGINNER)
            {
                doc_printf(doc, "%s %s %s %s\n",
                    p_desc.srh, p_desc.fos, p_desc.thn, p_desc.thb);
            }
            doc_printf(doc, "%s %s %s %s\n",
                r_desc.srh, r_desc.fos, r_desc.thn, r_desc.thb);
            doc_printf(doc, "%s %s %s %s\n",
                c_desc.srh, c_desc.fos, c_desc.thn, c_desc.thb);
            doc_printf(doc, "%s %s %s %s\n",
                tot_desc.srh, tot_desc.fos, tot_desc.thn, tot_desc.thb);
        }
    }
}

static void _change_name(void)
{
    if (!arg_lock_name)
    {
        char tmp[64];
        strcpy(tmp, player_name);
        Term_gotoxy(7, 0); /* Hack */
        if (askfor(tmp, 15))
            strcpy(player_name, tmp);
        if (0 == strlen(player_name))
            strcpy(player_name, "PLAYER");
    }
}

static int _inkey(void)
{
    return inkey_special(TRUE);
}

static void _sync_term(doc_ptr doc)
{
    rect_t r = ui_screen_rect();
    Term_load();
    doc_sync_term(doc, doc_range_top_lines(doc, r.cy), doc_pos_create(r.x, r.y));
}


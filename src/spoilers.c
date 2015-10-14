#include "angband.h"

bool spoiler_hack = FALSE;

#ifdef ALLOW_SPOILERS

typedef void(*_file_fn)(FILE*);
static void _text_file(cptr name, _file_fn fn)
{
    FILE    *fp = NULL;
    char    buf[1024];

    path_build(buf, sizeof(buf), ANGBAND_DIR_HELP, name);
    fp = my_fopen(buf, "w");

    if (!fp)
    {
        path_build(buf, sizeof(buf), ANGBAND_DIR_USER, name);
        fp = my_fopen(buf, "w");

        if (!fp)
        {
            prt("Failed!", 0, 0);
            (void)inkey();
            return;
        }
    }

    fn(fp);
    fprintf(fp, "\n\n<color:s>Automatically generated for PosChengband %d.%d.%d.</color>\n",
            VER_MAJOR, VER_MINOR, VER_PATCH);

    my_fclose(fp);
    msg_format("Created %s", buf);
}

static cptr _skill_desc(int amt, int div)
{
    static char buf[255];
    skill_desc_t desc = skills_describe(amt, div);
    sprintf(buf, "<color:%c>%-13.13s</color>", attr_to_attr_char(desc.color), desc.desc);
    return buf;
}

static void _race_help_table(FILE *fp, race_t *race_ptr)
{
    fputs("  <indent><style:table><color:G>Stat Modifiers          Skills</color>\n", fp);
    fprintf(fp, "Strength     %+3d        Disarming   %s\n",
        race_ptr->stats[A_STR],
        _skill_desc(race_ptr->skills.dis + 10, 2));

    fprintf(fp, "Intelligence %+3d        Device      %s\n",
        race_ptr->stats[A_INT],
        _skill_desc(race_ptr->skills.dev + 5, 1));

    fprintf(fp, "Wisdom       %+3d        Save        %s\n",
        race_ptr->stats[A_WIS],
        _skill_desc(race_ptr->skills.sav + 5, 1));

    fprintf(fp, "Dexterity    %+3d        Stealth     %s\n",
        race_ptr->stats[A_DEX],
        _skill_desc(race_ptr->skills.stl * 3, 1));

    fprintf(fp, "Constitution %+3d        Searching   %s\n",
        race_ptr->stats[A_CON],
        _skill_desc(race_ptr->skills.srh, 1));

    fprintf(fp, "Charisma     %+3d        Perception  %s\n",
        race_ptr->stats[A_CHR],
        _skill_desc(race_ptr->skills.fos, 1));

    fprintf(fp, "Life Rating  %3d%%       Melee       %s\n",
        race_ptr->life,
        _skill_desc(race_ptr->skills.thn + 10, 2));

    fprintf(fp, "Base HP      %3d        Bows        %s\n",
        race_ptr->base_hp,
        _skill_desc(race_ptr->skills.thb + 10, 2));

    fprintf(fp, "Experience   %3d%%       Infravision %d'\n", race_ptr->exp, race_ptr->infra*10);
    fputs("</style></indent>\n", fp);
}

static void _race_help(FILE *fp, int idx)
{
    race_t *race_ptr = get_race_t_aux(idx, 0);

    fprintf(fp, "<topic:%s><color:o>%s</color>\n", race_ptr->name, race_ptr->name);
    fprintf(fp, "%s\n\n", race_ptr->desc);
    switch(idx)
    {
    case RACE_DEMIGOD:
        fputs("See <link:Demigods.txt> for more details on demigod parentage.\n\n", fp);
        break;
    case RACE_DRACONIAN:
        fputs("See <link:Draconians.txt> for more details on draconians.\n\n", fp);
        break;
    }

    _race_help_table(fp, race_ptr);
}

/* TODO: This is copied/duplicated in birth.txt ... Spoiler generation is a convenience
   hack, so I'll turn a blind eye for now :) */
#define _MAX_RACES_PER_GROUP 23
#define _MAX_RACE_GROUPS      8
typedef struct _race_group_s {
    cptr name;
    int ids[_MAX_RACES_PER_GROUP];
} _race_group_t;
static _race_group_t _race_groups[_MAX_RACE_GROUPS] = {
    { "Humans",
        {RACE_AMBERITE, RACE_BARBARIAN, RACE_DEMIGOD, RACE_DUNADAN, RACE_HUMAN, -1} },
    { "Elves",
        {RACE_DARK_ELF, RACE_HIGH_ELF, RACE_WOOD_ELF, -1} },
    { "Hobbits/Dwarves",
        {RACE_DWARF, RACE_GNOME, RACE_HOBBIT, RACE_NIBELUNG, -1} },
    { "Fairies",
        {RACE_SHADOW_FAIRY, RACE_SPRITE, -1} },
    { "Angels/Demons",
        {RACE_ARCHON, RACE_BALROG, RACE_IMP, -1} },
    { "Orcs/Trolls/Giants",
        {RACE_CYCLOPS, RACE_HALF_GIANT, RACE_HALF_OGRE,
         RACE_HALF_TITAN, RACE_HALF_TROLL, RACE_KOBOLD, RACE_SNOTLING, -1} },
    { "The Undead",
        {RACE_SKELETON, RACE_SPECTRE, RACE_VAMPIRE, RACE_ZOMBIE, -1} },
    { "Other Races",
        {RACE_ANDROID, RACE_BEASTMAN, RACE_CENTAUR, RACE_DRACONIAN, RACE_DOPPELGANGER, RACE_ENT,
         RACE_GOLEM, RACE_KLACKON, RACE_KUTAR, RACE_MIND_FLAYER, RACE_TONBERRY, RACE_YEEK,-1 } },
};

#define _MAX_MON_RACE_GROUPS      12
static _race_group_t _mon_race_groups[_MAX_MON_RACE_GROUPS] = {
    { "Animal",
        {/*RACE_MON_ANT, RACE_MON_BEETLE, RACE_MON_BIRD, RACE_MON_CAT,*/ RACE_MON_CENTIPEDE,
            RACE_MON_HOUND, /*RACE_MON_HORSE, */ RACE_MON_HYDRA, RACE_MON_SPIDER, -1} },
    { "Angel/Demon",
        {RACE_MON_ANGEL, RACE_MON_DEMON, -1} },
    { "Beholder",
        {RACE_MON_BEHOLDER, -1} },
    { "Dragon",
        {RACE_MON_DRAGON, -1} },
    { "Elemental",
        {RACE_MON_ELEMENTAL, -1} },
    { "Golem",
        {RACE_MON_GOLEM, -1} },
    { "Jelly",
        {RACE_MON_JELLY, /*RACE_MON_MOLD,*/ RACE_MON_QUYLTHULG, -1} },
    { "Leprechaun",
        {RACE_MON_LEPRECHAUN, -1} },
    { "Mimic/Possessor",
        {RACE_MON_SWORD, /*RACE_MON_ARMOR,*/ RACE_MON_MIMIC, RACE_MON_POSSESSOR, RACE_MON_RING, -1} },
    { "Orc/Troll/Giant",
        {RACE_MON_GIANT, /*RACE_MON_KOBOLD, RACE_MON_ORC,*/ RACE_MON_TROLL, -1} },
    { "Undead",
        {/*RACE_MON_GHOST,*/ RACE_MON_LICH, RACE_MON_VAMPIRE, /*RACE_MON_WRAITH, RACE_MON_ZOMBIE,*/ -1 } },
    { "Xorn",
        {RACE_MON_XORN, -1} },
};

static void _races_help(FILE* fp)
{
    int i, j;

    fputs("<style:title>The Races</style>\n", fp);
    fputs("There are many races in the world, each, for the most part, with both "
          "strengths and weaknesses. Humans are the base race, and serve as the benchmark "
          "of comparison for all the others. In general, the stronger a race is relative to "
          "humans, the higher the <color:keyword>Experience Penalty</color> and the longer "
          "it will take to gain levels.\n\n"
          "For details on the <color:keyword>Primary Statistics</color>, see "
          "<link:birth.txt#PrimaryStats>. For information about the various <color:keyword>Skills</color>, see "
          "<link:birth.txt#PrimarySkills>. "
          "The skill descriptions in this document are for comparison purposes only. "
          "For example, your fledgling high-elf will not be born with <color:v>Legendary[2]</color> "
          "device skill. In general, skills are influenced by level, race, class, stats and equipment. "
          "To compare the various races, you might want to take a look "
          "at <link:Races.txt#Tables> the race tables below.\n\n", fp);
    for (i = 0; i < _MAX_RACE_GROUPS; i++)
    {
        fprintf(fp, "<style:heading>%s</style>\n  <indent>", _race_groups[i].name);
        for (j = 0; ; j++)
        {
            int race_idx = _race_groups[i].ids[j];
            if (race_idx == -1) break;
            _race_help(fp, race_idx);
        }
        fputs("</indent>\n", fp);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Race Statistic Bonus Table</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:G>STR  INT  WIS  DEX  CON  CHR  Life  BHP  Exp  Shop</color>\n", "");
    for (i = 0; i < _MAX_RACE_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int     race_idx = _race_groups[i].ids[j];
            race_t *race_ptr;

            if (race_idx == -1) break;
            race_ptr = get_race_t_aux(race_idx, 0);
            fprintf(fp, "%-12.12s %+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %+3d  %3d%% %4d%%\n",
                race_ptr->name,
                race_ptr->stats[A_STR], race_ptr->stats[A_INT], race_ptr->stats[A_WIS],
                race_ptr->stats[A_DEX], race_ptr->stats[A_CON], race_ptr->stats[A_CHR],
                race_ptr->life, race_ptr->base_hp, race_ptr->exp, race_ptr->shop_adjust
            );
        }
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills1><style:heading>Table 2 - Race Skill Bonus Table I</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s</color>\n", "", "Disarming", "Device", "Save", "Stealth");
    for (i = 0; i < _MAX_RACE_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int     race_idx = _race_groups[i].ids[j];
            race_t *race_ptr;

            if (race_idx == -1) break;
            race_ptr = get_race_t_aux(race_idx, 0);
            fprintf(fp, "%-12.12s", race_ptr->name);
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.dis + 10, 2));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.dev + 5, 1));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.sav + 5, 1));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.stl * 3, 1));
            fputc('\n', fp);
        }
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills2><style:heading>Table 3 - Race Skill Bonus Table II</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s %s</color>\n", "", "Searching", "Perception", "Melee", "Bows", "Infra");
    for (i = 0; i < _MAX_RACE_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int     race_idx = _race_groups[i].ids[j];
            race_t *race_ptr;

            if (race_idx == -1) break;
            race_ptr = get_race_t_aux(race_idx, 0);
            fprintf(fp, "%-12.12s", race_ptr->name);
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.srh, 1));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.fos, 1));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.thn + 10, 2));
            fprintf(fp, " %s", _skill_desc(race_ptr->skills.thb + 10, 2));
            fprintf(fp, " %4d'", race_ptr->infra * 10);
            fputc('\n', fp);
        }
    }
    fputs("\n</style>\n", fp);
}

static void _mon_race_help(FILE *fp, int idx)
{
    race_t *race_ptr = get_race_t_aux(idx, 0);
    caster_info *caster_ptr = NULL;

    if (race_ptr->caster_info)
        caster_ptr = race_ptr->caster_info();

    fprintf(fp, "<topic:%s><color:o>%s</color>\n", race_ptr->name, race_ptr->name);
    fprintf(fp, "%s\n\n", race_ptr->desc);
    switch(idx)
    {
    case RACE_MON_RING:
        fputs("See <link:rings.txt> for more details on rings.\n\n", fp);
        break;
    case RACE_MON_DRAGON:
        fputs("See <link:DragonRealms.txt> for more details on dragons.\n\n", fp);
        break;
    }

    fputs("  <indent><style:table><color:G>Stat Modifiers          Skill Modifiers</color>\n", fp);
    fprintf(fp, "Strength     <color:%c>%+3d</color>        Disarming   %2d+%-2d\n",
        (caster_ptr && caster_ptr->which_stat == A_STR) ? 'v' : 'w',
        race_ptr->stats[A_STR],
        race_ptr->skills.dis,
        race_ptr->extra_skills.dis);
    fprintf(fp, "Intelligence <color:%c>%+3d</color>        Device      %2d+%-2d\n",
        (caster_ptr && caster_ptr->which_stat == A_INT) ? 'v' : 'w',
        race_ptr->stats[A_INT],
        race_ptr->skills.dev,
        race_ptr->extra_skills.dev);
    fprintf(fp, "Wisdom       <color:%c>%+3d</color>        Save        %2d+%-2d\n",
        (caster_ptr && caster_ptr->which_stat == A_WIS) ? 'v' : 'w',
        race_ptr->stats[A_WIS],
        race_ptr->skills.sav,
        race_ptr->extra_skills.sav);
    fprintf(fp, "Dexterity    <color:%c>%+3d</color>        Stealth     %2d\n",
        (caster_ptr && caster_ptr->which_stat == A_DEX) ? 'v' : 'w',
        race_ptr->stats[A_DEX],
        race_ptr->skills.stl);
    fprintf(fp, "Constitution <color:%c>%+3d</color>        Searching   %2d\n",
        (caster_ptr && caster_ptr->which_stat == A_CON) ? 'v' : 'w',
        race_ptr->stats[A_CON],
        race_ptr->skills.srh);
    fprintf(fp, "Charisma     <color:%c>%+3d</color>        Perception  %2d\n",
        (caster_ptr && caster_ptr->which_stat == A_CHR) ? 'v' : 'w',
        race_ptr->stats[A_CHR],
        race_ptr->skills.fos);
    fprintf(fp, "Life Rating  %3d%%       Melee       %2d+%-2d\n",
        race_ptr->life,
        race_ptr->skills.thn,
        race_ptr->extra_skills.thn);
    fprintf(fp, "Base HP      %3d        Bows        %2d+%-2d\n",
        race_ptr->base_hp,
        race_ptr->skills.thb,
        race_ptr->extra_skills.thb);
    fprintf(fp, "Experience   %3d%%     Infravision %4d'\n", race_ptr->exp, race_ptr->infra*10);
    fputs("</style></indent>\n", fp);
}

static void _monster_races_help(FILE* fp)
{
    int i, j;

    fprintf(fp, "<style:title>Monster Races</style>\n\n");
    fputs("So, you have decided to play as a monster. There are many options "
            "to choose from and the various types of monsters are loosely grouped "
            "by type below: Animals, Dragons, Demons, etc.\n\n"
            "As a monster, you won't be able to choose a class the way normal "
            "players do. Rather, most monster types gain abilities and powers "
            "as they gain experience. For example, dragons can breathe fire as "
            "well as access magical abilities. So you will want to check out both "
            "the magic command (<color:keypress>m</color>) as well as the racial "
            "power command (<color:keypress>U</color> or <color:keypress>O</color>) "
            "to see what powers are available as you play.\n\n"
            "In addition, most monsters have custom body types, and this will "
            "severely constrain the amount and kind of equipment you may wear. "
            "For example, a beholder cannot wear armor or wield a sword ... That "
            "would be an odd sight indeed! Instead, they may wear rings on their "
            "numerous eyestalks. Details of this kind should be described below.\n\n"
            "Finally, all monsters evolve. When they gain enough experience, they "
            "will assume a more powerful form. To continue with our example of dragons, "
            "you might evolve from Baby to Young to Mature and finally Ancient forms, "
            "becoming vastly more powerful in the process. The stats and skills listed "
            "in this document only apply to the starting form, which is usually "
            "very weak.\n\n"
            "For details on the <color:keyword>Primary Statistics</color>, see "
            "<link:birth.txt#PrimaryStats>. For information about the various "
            "<color:keyword>Skills</color>, see <link:birth.txt#PrimarySkills>. "
            "To compare the various races at a glance, you might want to take a "
            "look at <link:MonsterRaces.txt#Tables> the race tables below.\n\n", fp);

    for (i = 0; i < _MAX_MON_RACE_GROUPS; i++)
    {
        fprintf(fp, "<style:heading>%s</style>\n  <indent>", _mon_race_groups[i].name);
        for (j = 0; ; j++)
        {
            int race_idx = _mon_race_groups[i].ids[j];
            if (race_idx == -1) break;
            _mon_race_help(fp, race_idx);
        }
        fputs("</indent>\n", fp);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Race Statistic Bonus Table</style>\n<style:table>\n", fp);

    for (i = 0; i < _MAX_MON_RACE_GROUPS; i++)
    {
        fprintf(fp, "<color:o>%s</color>\n", _mon_race_groups[i].name);
        fprintf(fp, "<color:G>%-14.14s</color> <color:G>STR  INT  WIS  DEX  CON  CHR  Life  BHP  Exp  Shop</color>\n", "Race");
        for (j = 0; ; j++)
        {
            int     race_idx = _mon_race_groups[i].ids[j];
            race_t *race_ptr;

            if (race_idx == -1) break;
            race_ptr = get_race_t_aux(race_idx, 0);
            fprintf(fp, "%-14s %+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %+3d  %3d%% %4d%%\n",
                race_ptr->name,
                race_ptr->stats[A_STR], race_ptr->stats[A_INT], race_ptr->stats[A_WIS],
                race_ptr->stats[A_DEX], race_ptr->stats[A_CON], race_ptr->stats[A_CHR],
                race_ptr->life, race_ptr->base_hp, race_ptr->exp, race_ptr->shop_adjust
            );
        }
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);

    fputs("<style:heading>Table 2 - Race Skill Bonus Table</style>\n<style:table>\n", fp);

    for (i = 0; i < _MAX_MON_RACE_GROUPS; i++)
    {
        fprintf(fp, "<color:o>%s</color>\n", _mon_race_groups[i].name);
        fprintf(fp, "<color:G>%-14.14s</color> <color:G>Dsrm  Dvce  Save  Stlh  Srch  Prcp  Melee  Bows  Infra</color>\n", "Race");
        for (j = 0; ; j++)
        {
            int     race_idx = _mon_race_groups[i].ids[j];
            race_t *race_ptr;

            if (race_idx == -1) break;
            race_ptr = get_race_t_aux(race_idx, 0);
            fprintf(fp, "%-14s %+4d  %+4d  %+4d  %+4d  %+4d  %+4d  %+5d  %+4d  %4d'\n",
                race_ptr->name,
                race_ptr->skills.dis, race_ptr->skills.dev, race_ptr->skills.sav,
                race_ptr->skills.stl, race_ptr->skills.srh, race_ptr->skills.fos,
                race_ptr->skills.thn, race_ptr->skills.thb, race_ptr->infra*10
            );
        }
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);
}

struct _name_desc_s { string_ptr name; string_ptr desc; };
typedef struct _name_desc_s _name_desc_t, *_name_desc_ptr;
static int _compare_name_desc(const _name_desc_ptr left, const _name_desc_ptr right) {
    return string_compare(left->name, right->name);
}
static void _name_desc_free(_name_desc_ptr p) {
    string_free(p->name);
    string_free(p->desc);
    free(p);
}
static _name_desc_ptr _name_desc_alloc(void) {
    _name_desc_ptr result = malloc(sizeof(_name_desc_t));
    result->name = string_alloc();
    result->desc = string_alloc();
    return result;
}

static void _demigods_help(FILE* fp)
{
    int i;

    fputs("<style:title>Demigod Parentage</style>\n\n", fp);
    fputs(get_race_t_aux(RACE_DEMIGOD, 0)->desc, fp);
    fputs("\n\n", fp);

    for (i = 0; i < MAX_DEMIGOD_TYPES; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DEMIGOD, i);

        fprintf(fp, "<topic:%s><color:o>%s</color>\n", race_ptr->subname, race_ptr->subname);
        fprintf(fp, "%s\n\n", race_ptr->subdesc);

        _race_help_table(fp, race_ptr);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Demigod Statistic Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:G>               STR  INT  WIS  DEX  CON  CHR  Life  Exp  Shop</color>\n", fp);

    for (i = 0; i < MAX_DEMIGOD_TYPES; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DEMIGOD, i);

        fprintf(fp, "%-14s %+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %3d%% %4d%%\n",
            race_ptr->subname,
            race_ptr->stats[A_STR], race_ptr->stats[A_INT], race_ptr->stats[A_WIS], 
            race_ptr->stats[A_DEX], race_ptr->stats[A_CON], race_ptr->stats[A_CHR], 
            race_ptr->life, race_ptr->exp, race_ptr->shop_adjust
        );
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills1><style:heading>Table 2 - Demigod Skill Bonus Table I</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s</color>\n", "", "Disarming", "Device", "Save", "Stealth");
    for (i = 0; i < MAX_DEMIGOD_TYPES; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DEMIGOD, i);
        fprintf(fp, "%-12.12s", race_ptr->subname);
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.dis + 10, 2));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.dev + 5, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.sav + 5, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.stl * 3, 1));
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills2><style:heading>Table 3 - Demigod Skill Bonus Table II</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s %s</color>\n", "", "Searching", "Perception", "Melee", "Bows", "Infra");
    for (i = 0; i < MAX_DEMIGOD_TYPES; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DEMIGOD, i);
        fprintf(fp, "%-12.12s", race_ptr->subname);
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.srh, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.fos, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.thn + 10, 2));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.thb + 10, 2));
        fprintf(fp, " %4d'", race_ptr->infra * 10);
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);

    {
        vec_ptr vec = vec_alloc((vec_free_f)_name_desc_free);

        fputs("<topic:Powers><style:heading>Table 4 - Demigod Special Powers</style>\n\n", fp);
        fputs("All demigods have access to special powers. When they reach level 20, they may choose "
                    "a single power from the following list. When they reach level, 40, they may choose another. "
                    "These powers can never be removed or changed, so you might want to study this list to "
                    "decide which powers you will choose for your character.\n\n", fp);

        for (i = 0; i < MAX_MUTATIONS; i++)
        {
            if (mut_demigod_pred(i))
            {
                char buf[1024];
                _name_desc_ptr nd = _name_desc_alloc();

                mut_name(i, buf);
                string_append_s(nd->name, buf);

                mut_help_desc(i, buf);
                string_append_s(nd->desc, buf);
                vec_add(vec, nd);
            }
        }

        vec_sort(vec, (vec_cmp_f)_compare_name_desc);

        for (i = 0; i < vec_length(vec); i++)
        {
            _name_desc_ptr nd = vec_get(vec, i);
            /*fprintf(fp, "<color:G>%s: </color>%s\n",*/
            fprintf(fp, "  <indent><color:G>%s</color>\n%s</indent>\n\n",
                string_buffer(nd->name), string_buffer(nd->desc));
        }

        vec_free(vec);
    }
    fputs("\n\n", fp);
}

static void _draconians_help(FILE* fp)
{
    int i;

    fputs("<style:title>Draconians</style>\n\n", fp);
    fputs(get_race_t_aux(RACE_DRACONIAN, 0)->desc, fp);
    fputs("\n\n", fp);

    for (i = 0; i < DRACONIAN_MAX; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DRACONIAN, i);

        fprintf(fp, "<topic:%s><color:o>%s</color>\n", race_ptr->subname, race_ptr->subname);
        fprintf(fp, "%s\n\n", race_ptr->subdesc);

        _race_help_table(fp, race_ptr);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Draconian Statistic Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:G>               STR  INT  WIS  DEX  CON  CHR  Life  Exp  Shop</color>\n", fp);

    for (i = 0; i < DRACONIAN_MAX; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DRACONIAN, i);

        fprintf(fp, "%-14s %+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %3d%% %4d%%\n",
            race_ptr->subname,
            race_ptr->stats[A_STR], race_ptr->stats[A_INT], race_ptr->stats[A_WIS],
            race_ptr->stats[A_DEX], race_ptr->stats[A_CON], race_ptr->stats[A_CHR],
            race_ptr->life, race_ptr->exp, race_ptr->shop_adjust
        );
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills1><style:heading>Table 2 - Draconian Skill Bonus Table I</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s</color>\n", "", "Disarming", "Device", "Save", "Stealth");
    for (i = 0; i < DRACONIAN_MAX; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DRACONIAN, i);
        fprintf(fp, "%-12.12s", race_ptr->subname);
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.dis + 10, 2));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.dev + 5, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.sav + 5, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.stl * 3, 1));
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills2><style:heading>Table 3 - Draconian Skill Bonus Table II</style>\n<style:table>\n", fp);
    fprintf(fp, "%-12.12s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s %s</color>\n", "", "Searching", "Perception", "Melee", "Bows", "Infra");
    for (i = 0; i < DRACONIAN_MAX; i++)
    {
        race_t *race_ptr = get_race_t_aux(RACE_DRACONIAN, i);
        fprintf(fp, "%-12.12s", race_ptr->subname);
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.srh, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.fos, 1));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.thn + 10, 2));
        fprintf(fp, " %s", _skill_desc(race_ptr->skills.thb + 10, 2));
        fprintf(fp, " %4d'", race_ptr->infra * 10);
        fputc('\n', fp);
    }
    fputs("\n</style>\n", fp);

    {
        vec_ptr vec = vec_alloc((vec_free_f)_name_desc_free);

        fputs("<topic:Powers><style:heading>Table 4 - Draconian Special Powers</style>\n\n", fp);
        fputs("All draconians have access to special powers. When they reach level 35, they may choose "
                "a single power from the following list. "
                "These powers can never be removed or changed, so you might want to study this list to "
                "decide which power you will choose for your character.\n\n", fp);

        for (i = 0; i < MAX_MUTATIONS; i++)
        {
            if (mut_draconian_pred(i))
            {
                char buf[1024];
                _name_desc_ptr nd = _name_desc_alloc();

                mut_name(i, buf);
                string_append_s(nd->name, buf);

                mut_help_desc(i, buf);
                string_append_s(nd->desc, buf);
                vec_add(vec, nd);
            }
        }

        vec_sort(vec, (vec_cmp_f)_compare_name_desc);

        for (i = 0; i < vec_length(vec); i++)
        {
            _name_desc_ptr nd = vec_get(vec, i);
            /*fprintf(fp, "<color:G>%s: </color>%s\n",*/
            fprintf(fp, "  <indent><color:G>%s</color>\n%s</indent>\n\n",
                string_buffer(nd->name), string_buffer(nd->desc));
        }

        vec_free(vec);
    }
    fputs("\n\n", fp);
}

static void _dragon_realms_help(FILE* fp)
{
    int i, j;
    fputs("<style:title>Dragon Realms</style>\n\n", fp);
    fputs("Dragons are magical creatures and may choose to learn a particular branch of "
           "dragon magic. Unlike normal spell casters, dragons do not need spell books to "
           "cast or learn powers. Instead, they simply gain spells as they mature. Each "
           "realm of dragon magic has a direct impact on the player's stats and skills, and "
           "each realm also requires a different stat for casting purposes.\n\n", fp);
    for (i = 1; i < DRAGON_REALM_MAX; i++)
    {
        dragon_realm_ptr realm = dragon_get_realm(i);
        fprintf(fp, "<style:heading>%s</style>\n\n", realm->name);
        fputs(realm->desc, fp);
        fputs("\n\n", fp);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Dragon Realm Statistic Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:U>               STR  INT  WIS  DEX  CON  CHR  Life  Exp</color>\n", fp);
    for (i = 1; i < DRAGON_REALM_MAX; i++)
    {
        dragon_realm_ptr realm = dragon_get_realm(i);
        char             line[255];
        char             tmp[255];

        sprintf(line, "%-14s", realm->name);
        for (j = 0; j < 6; j++)
        {
            if (j == realm->spell_stat)
                sprintf(tmp, "<color:G> %+3d </color>", realm->stats[j]);
            else
                sprintf(tmp, " %+3d ", realm->stats[j]);
            strcat(line, tmp);
        }
        sprintf(tmp, " %3d%%  %3d%%", realm->life, realm->exp);
        strcat(line, tmp);
        fprintf(fp, "%s\n", line);
    }
    fputs("\n</style>\n", fp);

    fputs("<style:heading>Table 2 - Dragon Realm Skill Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:U>               Dsrm  Dvce  Save  Stlh  Srch  Prcp  Melee  Attack  Breath</color>\n", fp);
    for (i = 1; i < DRAGON_REALM_MAX; i++)
    {
        dragon_realm_ptr realm = dragon_get_realm(i);
        fprintf(fp, "%-14s %+4d  %+4d  %+4d  %+4d  %+4d  %+4d  %+5d  %5d%%  %5d%%\n",
            realm->name,
            realm->skills.dis, 
            realm->skills.dev,
            realm->skills.sav,
            realm->skills.stl, 
            realm->skills.srh, 
            realm->skills.fos,
            realm->skills.thn,
            realm->attack,
            realm->breath
        );
    }
    fputs("\n</style>\n", fp);
}

static void _class_help(FILE *fp, int idx)
{
    class_t     *class_ptr = get_class_t_aux(idx, 0);
    caster_info *caster_ptr = NULL;

    if (class_ptr->caster_info && idx != CLASS_PSION)
        caster_ptr = class_ptr->caster_info();

    fprintf(fp, "<topic:%s><color:o>%s</color>\n", class_ptr->name, class_ptr->name);
    fputs(class_ptr->desc, fp);
    fputs("\n\n", fp);

    fputs("  <indent><style:table><color:G>Stat Modifiers          Skill Modifiers</color>\n", fp);
    fprintf(fp, "Strength     <color:%c>%+3d</color>        Disarming   %s\n",
        (caster_ptr && caster_ptr->which_stat == A_STR) ? 'v' : 'w',
        class_ptr->stats[A_STR],
        _skill_desc(class_ptr->base_skills.dis + 5*class_ptr->extra_skills.dis, 8));
    fprintf(fp, "Intelligence <color:%c>%+3d</color>        Device      %s\n",
        (caster_ptr && caster_ptr->which_stat == A_INT) ? 'v' : 'w',
        class_ptr->stats[A_INT],
        _skill_desc(class_ptr->base_skills.dev + 5*class_ptr->extra_skills.dev, 6));
    fprintf(fp, "Wisdom       <color:%c>%+3d</color>        Save        %s\n",
        (caster_ptr && caster_ptr->which_stat == A_WIS) ? 'v' : 'w',
        class_ptr->stats[A_WIS],
        _skill_desc(class_ptr->base_skills.sav + 5*class_ptr->extra_skills.sav, 7));
    fprintf(fp, "Dexterity    <color:%c>%+3d</color>        Stealth     %s\n",
        (caster_ptr && caster_ptr->which_stat == A_DEX) ? 'v' : 'w',
        class_ptr->stats[A_DEX],
        _skill_desc(3*(class_ptr->base_skills.stl + 5*class_ptr->extra_skills.stl), 1));
    fprintf(fp, "Constitution <color:%c>%+3d</color>        Searching   %s\n",
        (caster_ptr && caster_ptr->which_stat == A_CON) ? 'v' : 'w',
        class_ptr->stats[A_CON],
        _skill_desc(class_ptr->base_skills.srh + 5*class_ptr->extra_skills.srh, 6));
    fprintf(fp, "Charisma     <color:%c>%+3d</color>        Perception  %s\n",
        (caster_ptr && caster_ptr->which_stat == A_CHR) ? 'v' : 'w',
        class_ptr->stats[A_CHR],
        _skill_desc(class_ptr->base_skills.fos + 5*class_ptr->extra_skills.fos, 6));
    fprintf(fp, "Life Rating  %3d%%       Melee       %s\n",
        class_ptr->life,
        _skill_desc(class_ptr->base_skills.thn + 5*class_ptr->extra_skills.thn, 12));
    fprintf(fp, "Base HP      %3d        Bows        %s\n",
        class_ptr->base_hp,
        _skill_desc(class_ptr->base_skills.thb + 5*class_ptr->extra_skills.thb, 12));
    fprintf(fp, "Experience   %3d%%\n", class_ptr->exp);
    fputs("</style></indent>\n", fp);
}

#define _MAX_CLASSES_PER_GROUP 20
#define _MAX_CLASS_GROUPS      11
typedef struct _class_group_s {
    cptr name;
    int ids[_MAX_CLASSES_PER_GROUP];
} _class_group_t;
static _class_group_t _class_groups[_MAX_CLASS_GROUPS] = {
    { "Melee", {CLASS_BERSERKER, CLASS_BLOOD_KNIGHT, CLASS_DUELIST, CLASS_MAULER,
                    CLASS_RUNE_KNIGHT, CLASS_SAMURAI, CLASS_WARRIOR, CLASS_WEAPONMASTER,
                    CLASS_WEAPONSMITH, -1} },
    { "Archery", {CLASS_ARCHER, CLASS_SNIPER, -1} },
    { "Martial Arts", {CLASS_FORCETRAINER, CLASS_MONK, CLASS_MYSTIC, -1} },
    { "Magic", {CLASS_BLOOD_MAGE, CLASS_BLUE_MAGE, CLASS_HIGH_MAGE, CLASS_MAGE,
                    CLASS_NECROMANCER, CLASS_SORCERER, -1} },
    { "Devices", {CLASS_DEVICEMASTER, /*CLASS_MAGIC_EATER,*/ -1} },
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

static void _classes_help(FILE* fp)
{
    int i, j, k;

    fputs("<style:title>The Classes</style>\n\n", fp);
    fputs("No decision is so important as which class to play. Below, the many "
            "available classes are loosely grouped by their principle playstyle: "
            "Melee, Archery, Martial Arts, Magic, Devices, etc. The hybrid classes "
            "generally combine melee with a bit of magic and are a good option for "
            "a balanced playstyle. In this document, the primary spell stat for "
            "each class is <color:v>highlighted</color>.\n\n"
            "For details on the <color:keyword>Primary Statistics</color>, see "
            "<link:birth.txt#PrimaryStats>. For information about the various "
            "<color:keyword>Skills</color>, see <link:birth.txt#PrimarySkills>. "
            "The skill descriptions in this document are for comparison purposes only. "
            "For example, your fledgling berserker will not be born with <color:v>Legendary[32]</color> "
            "melee skill. In general, skills are influenced by level, race, class, stats and equipment. "
            "To compare the various classes at a glance, you might want to take "
            "a look at <link:Classes.txt#Tables> the class tables below.\n\n", fp);

    for (i = 0; i < _MAX_CLASS_GROUPS; i++)
    {
        fprintf(fp, "<style:heading>%s</style>\n  <indent>", _class_groups[i].name);
        for (j = 0; ; j++)
        {
            int class_idx = _class_groups[i].ids[j];
            if (class_idx == -1) break;
            _class_help(fp, class_idx);
        }
        fputs("</indent>\n", fp);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Class Statistic Bonus Table</style>\n<style:table>\n", fp);
    fprintf(fp, "%-13.13s <color:G>STR  INT  WIS  DEX  CON  CHR  Life  BHP  Exp</color>\n", "");
    for (i = 0; i < _MAX_CLASS_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int          class_idx = _class_groups[i].ids[j];
            class_t     *class_ptr;
            caster_info *caster_ptr = NULL;
            char         line[255];
            char         tmp[255];

            if (class_idx == -1) break;
            class_ptr = get_class_t_aux(class_idx, 0);
            if (class_ptr->caster_info)
                caster_ptr = class_ptr->caster_info();

            sprintf(line, "%-13.13s", class_ptr->name);
            for (k = 0; k < 6; k++)
            {
                if (caster_ptr && k == caster_ptr->which_stat && class_idx != CLASS_PSION)
                    sprintf(tmp, "<color:v> %+3d </color>", class_ptr->stats[k]);
                else
                    sprintf(tmp, " %+3d ", class_ptr->stats[k]);
                strcat(line, tmp);
            }
            sprintf(tmp, " %3d%%  %+3d  %3d%%", class_ptr->life, class_ptr->base_hp, class_ptr->exp);
            strcat(line, tmp);
            fprintf(fp, "%s\n", line);
        }
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills1><style:heading>Table 2 - Class Skill Bonus Table I</style>\n<style:table>\n", fp);
    fprintf(fp, "%-13.13s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s</color>\n", "", "Disarming", "Device", "Save", "Stealth");
    for (i = 0; i < _MAX_CLASS_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int     class_idx = _class_groups[i].ids[j];
            class_t *class_ptr;

            if (class_idx == -1) break;
            class_ptr = get_class_t_aux(class_idx, 0);
            fprintf(fp, "%-13.13s", class_ptr->name);
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.dis + 5*class_ptr->extra_skills.dis, 8));
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.dev + 5*class_ptr->extra_skills.dev, 6));
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.sav + 5*class_ptr->extra_skills.sav, 7));
            fprintf(fp, " %s", _skill_desc(3*(class_ptr->base_skills.stl + 5*class_ptr->extra_skills.stl), 1));
            fputc('\n', fp);
        }
    }
    fputs("\n</style>\n", fp);

    fputs("<topic:Skills2><style:heading>Table 3 - Class Skill Bonus Table II</style>\n<style:table>\n", fp);
    fprintf(fp, "%-13.13s <color:w>%-13.13s %-13.13s %-13.13s %-13.13s</color>\n", "", "Searching", "Perception", "Melee", "Bows");
    for (i = 0; i < _MAX_CLASS_GROUPS; i++)
    {
        for (j = 0; ; j++)
        {
            int     class_idx = _class_groups[i].ids[j];
            class_t *class_ptr;

            if (class_idx == -1) break;
            class_ptr = get_class_t_aux(class_idx, 0);
            fprintf(fp, "%-13.13s", class_ptr->name);
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.srh + 5*class_ptr->extra_skills.srh, 6));
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.fos + 5*class_ptr->extra_skills.fos, 6));
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.thn + 5*class_ptr->extra_skills.thn, 12));
            fprintf(fp, " %s", _skill_desc(class_ptr->base_skills.thb + 5*class_ptr->extra_skills.thb, 12));
            fputc('\n', fp);
        }
    }
    fputs("\n</style>\n", fp);
}

static void _personality_help(FILE *fp, int idx)
{
    personality_ptr pers_ptr = get_personality_aux(idx);

    fprintf(fp, "<topic:%s><color:o>%s</color>\n", pers_ptr->name, pers_ptr->name);
    fprintf(fp, "%s\n\n", pers_ptr->desc);

    fputs("  <indent><style:table><color:G>Stat Modifiers          Skill Modifiers</color>\n", fp);
    fprintf(fp, "Strength     %+3d        Disarming   %+4d\n", pers_ptr->stats[A_STR], pers_ptr->skills.dis);
    fprintf(fp, "Intelligence %+3d        Device      %+4d\n", pers_ptr->stats[A_INT], pers_ptr->skills.dev);
    fprintf(fp, "Wisdom       %+3d        Save        %+4d\n", pers_ptr->stats[A_WIS], pers_ptr->skills.sav);
    fprintf(fp, "Dexterity    %+3d        Stealth     %+4d\n", pers_ptr->stats[A_DEX], pers_ptr->skills.stl);
    fprintf(fp, "Constitution %+3d        Searching   %+4d\n", pers_ptr->stats[A_CON], pers_ptr->skills.srh);
    fprintf(fp, "Charisma     %+3d        Perception  %+4d\n", pers_ptr->stats[A_CHR], pers_ptr->skills.fos);
    fprintf(fp, "Life Rating  %3d%%       Melee       %+4d\n", pers_ptr->life, pers_ptr->skills.thn);
    fprintf(fp, "Experience   %3d%%       Bows        %+4d\n", pers_ptr->exp, pers_ptr->skills.thb);
    fputs("</style></indent>\n", fp);
}

static void _personalities_help(FILE* fp)
{
    int i;

    fprintf(fp, "<style:title>The Personalities</style>\n\n");
    for (i = 0; i < MAX_PERSONALITIES; i++)
    {
        _personality_help(fp, i);
    }

    fputs("<topic:Tables><style:heading>Table 1 - Personality Statistic Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:G>               STR  INT  WIS  DEX  CON  CHR  Life  Exp</color>\n", fp);

    for (i = 0; i < MAX_PERSONALITIES; i++)
    {
        personality_ptr pers_ptr = get_personality_aux(i);

        fprintf(fp, "%-14s %+3d  %+3d  %+3d  %+3d  %+3d  %+3d  %3d%%  %3d%%\n",
            pers_ptr->name,
            pers_ptr->stats[0], pers_ptr->stats[1], pers_ptr->stats[2],
            pers_ptr->stats[3], pers_ptr->stats[4], pers_ptr->stats[5],
            pers_ptr->life, pers_ptr->exp
        );
    }
    fputs("\n</style>\n", fp);

    fputs("<style:heading>Table 2 - Personality Skill Bonus Table</style>\n\n", fp);
    fputs("<style:table><color:G>               Dsrm  Dvce  Save  Stlh  Srch  Prcp  Melee  Bows</color>\n", fp);
    for (i = 0; i < MAX_PERSONALITIES; i++)
    {
        personality_ptr pers_ptr = get_personality_aux(i);

        fprintf(fp, "%-14s %+4d  %+4d  %+4d  %+4d  %+4d  %+4d  %+5d  %+4d\n",
            pers_ptr->name,
            pers_ptr->skills.dis, pers_ptr->skills.dev, pers_ptr->skills.sav,
            pers_ptr->skills.stl, pers_ptr->skills.srh, pers_ptr->skills.fos,
            pers_ptr->skills.thn, pers_ptr->skills.thb
        );
    }
    fputs("\n</style>\n", fp);
}

static void _mon_dam_help(FILE* fp)
{
    int i, j;
    fprintf(fp, "Name,Idx,Lvl,HP,Ac,El,Fi,Co,Po,Li,Dk,Cf,Nt,Nx,So,Sh,Ca,Di\n");
    for (i = 0; i < max_r_idx; i++)
    {
        monster_race *r_ptr = &r_info[i];
        int           hp = 0;
        int           dam[RES_MAX] = {0};
        bool          show = FALSE;

        if (r_ptr->flags1 & RF1_FORCE_MAXHP)
            hp = r_ptr->hdice * r_ptr->hside;
        else
            hp = r_ptr->hdice * (1 + r_ptr->hside)/2;

        /* Damage Logic Duplicated from mspells1.c */
        if (r_ptr->flags4 & RF4_ROCKET)
            dam[RES_SHARDS] = MAX(dam[RES_SHARDS], MIN(hp / 4, 600));
        if (r_ptr->flags4 & RF4_BR_ACID)
            dam[RES_ACID] = MAX(dam[RES_ACID], MIN(hp / 4, 900));
        if (r_ptr->flags4 & RF4_BR_ELEC)
            dam[RES_ELEC] = MAX(dam[RES_ELEC], MIN(hp / 4, 900));
        if (r_ptr->flags4 & RF4_BR_FIRE)
            dam[RES_FIRE] = MAX(dam[RES_FIRE], MIN(hp / 4, 900));
        if (r_ptr->flags4 & RF4_BR_COLD)
            dam[RES_COLD] = MAX(dam[RES_COLD], MIN(hp / 4, 900));
        if (r_ptr->flags4 & RF4_BR_POIS)
            dam[RES_POIS] = MAX(dam[RES_POIS], MIN(hp / 5, 600));
        if (r_ptr->flags4 & RF4_BR_NETH)
            dam[RES_NETHER] = MAX(dam[RES_NETHER], MIN(hp / 6, 550));
        if (r_ptr->flags4 & RF4_BR_LITE)
            dam[RES_LITE] = MAX(dam[RES_LITE], MIN(hp / 6, 400));
        if (r_ptr->flags4 & RF4_BR_DARK)
            dam[RES_DARK] = MAX(dam[RES_DARK], MIN(hp / 6, 400));
        if (r_ptr->flags4 & RF4_BR_CONF)
            dam[RES_CONF] = MAX(dam[RES_CONF], MIN(hp / 6, 400));
        if (r_ptr->flags4 & RF4_BR_SOUN)
            dam[RES_SOUND] = MAX(dam[RES_SOUND], MIN(hp / 6, 450));
        if (r_ptr->flags4 & RF4_BR_CHAO)
            dam[RES_CHAOS] = MAX(dam[RES_CHAOS], MIN(hp / 6, 600));
        if (r_ptr->flags4 & RF4_BR_DISE)
            dam[RES_DISEN] = MAX(dam[RES_DISEN], MIN(hp / 6, 500));
        if (r_ptr->flags4 & RF4_BR_NEXU)
            dam[RES_NEXUS] = MAX(dam[RES_NEXUS], MIN(hp / 3, 250));
        if (r_ptr->flags4 & RF4_BR_SHAR)
            dam[RES_SHARDS] = MAX(dam[RES_SHARDS], MIN(hp / 6, 500));
        if (r_ptr->flags4 & RF4_BR_NUKE)
            dam[RES_POIS] = MAX(dam[RES_POIS], MIN(hp / 5, 600));
        if (r_ptr->flags5 & RF5_BA_DARK)
            dam[RES_DARK] = MAX(dam[RES_DARK], r_ptr->level*4 + 105);
        if (r_ptr->flags5 & RF5_BA_LITE)
            dam[RES_LITE] = MAX(dam[RES_LITE], r_ptr->level*4 + 105);

        for (j = 0; j < RES_MAX; j++)
        {
            if (dam[j] > 0)
                show = TRUE;
        }

        if (show)
        {
            fprintf(fp, "\"%s\",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                r_name + r_ptr->name, i, r_ptr->level, hp,
                dam[RES_ACID], dam[RES_ELEC], dam[RES_FIRE], dam[RES_COLD], dam[RES_POIS],
                dam[RES_LITE], dam[RES_DARK], dam[RES_CONF], dam[RES_NETHER], dam[RES_NEXUS],
                dam[RES_SOUND], dam[RES_SHARDS], dam[RES_CHAOS], dam[RES_DISEN]
            );
        }
    }
}

static void _possessor_stats_help(FILE* fp)
{
    int i;
    fprintf(fp, "Name,Idx,Lvl,Speed,AC,Attacks,Dam,Body,Str,Int,Wis,Dex,Con,Chr,Life,Disarm,Device,Save,Stealth,Search,Perception,Melee,Bows\n");
    for (i = 0; i < max_r_idx; i++)
    {
        monster_race *r_ptr = &r_info[i];

        if (r_ptr->flags9 & RF9_DROP_CORPSE)
        {
            int ac = 0, dam = 0, attacks = 0, j;

            if (r_ptr->flags9 & RF9_POS_GAIN_AC)
                ac = r_ptr->ac;

            for (j = 0; j < 4; j++)
            {
                if (!r_ptr->blow[j].effect) continue;
                if (r_ptr->blow[j].method == RBM_EXPLODE) continue;
                if (r_ptr->blow[j].method == RBM_SHOOT) continue;

                dam += r_ptr->blow[j].d_dice * (r_ptr->blow[j].d_side + 1) / 2;
                attacks++;
            }

            fprintf(fp, "\"%s\",%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d,%d,%d,=\"%d+%d\",=\"%d+%d\",=\"%d+%d\",%d,%d,%d,=\"%d+%d\",=\"%d+%d\"\n",
                r_name + r_ptr->name, i, r_ptr->level, 
                r_ptr->speed - 110, ac, attacks, dam,
                b_name + b_info[r_ptr->body.body_idx].name,
                r_ptr->body.stats[A_STR], r_ptr->body.stats[A_INT], r_ptr->body.stats[A_WIS],
                r_ptr->body.stats[A_DEX], r_ptr->body.stats[A_CON], r_ptr->body.stats[A_CHR],
                r_ptr->body.life,
                r_ptr->body.skills.dis, r_ptr->body.extra_skills.dis, 
                r_ptr->body.skills.dev, r_ptr->body.extra_skills.dev, 
                r_ptr->body.skills.sav, r_ptr->body.extra_skills.sav,
                r_ptr->body.skills.stl,
                r_ptr->body.skills.srh, 
                r_ptr->body.skills.fos,
                r_ptr->body.skills.thn, r_ptr->body.extra_skills.thn, 
                r_ptr->body.skills.thb, r_ptr->body.extra_skills.thb
            );
        }
    }
}

void generate_spoilers(void)
{
    spoiler_hack = TRUE;

    _text_file("Races.txt", _races_help);
    _text_file("MonsterRaces.txt", _monster_races_help);
    _text_file("Demigods.txt", _demigods_help);
    _text_file("Draconians.txt", _draconians_help);
    _text_file("Classes.txt", _classes_help);
    _text_file("Personalities.txt", _personalities_help);
    _text_file("PossessorStats.csv", _possessor_stats_help);
    _text_file("MonsterDam.csv", _mon_dam_help);
    _text_file("DragonRealms.txt", _dragon_realms_help);
    spoiler_hack = FALSE;
}

#endif

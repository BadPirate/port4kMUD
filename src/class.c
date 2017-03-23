/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
extern struct descriptor_data *descriptor_list;
extern void send_to_char(char *, struct char_data *);
extern void send_to_all(char *);

/* Names first */

const char *class_abbrevs[] = {
  "&rMu&w",
  "&bCl&w",
  "Th",
  "&gWa&w",
  "&mPa&w",
  "&yVa&w",
  "&cBa&w",
  "&GSc&w",
  "&uNj&n",
  "&u&rM*&n",
  "\n"
};


const char *pc_class_types[] = {
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "Paladin",
  "Vampyre",
  "Bard",
  "Scout",
  "Ninja",
  "Master",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [C]leric\r\n"
"  [T]hief\r\n"
"  [W]arrior\r\n"
"  [M]agic-user\r\n"
"  [P]aladin\r\n"
"  [V]ampyre\r\n"
"  [B]ard\r\n"
"  [S]cout\r\n";

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm':
    return CLASS_MAGIC_USER;
    break;
  case 'c':
    return CLASS_CLERIC;
    break;
  case 'w':
    return CLASS_WARRIOR;
    break;
  case 't':
    return CLASS_THIEF;
    break;
  case 'p':
    return CLASS_PALADIN;
    break;
  case 'v':
    return CLASS_VAMPYRE;
    break;
  case 'b':
    return CLASS_BARD;
    break;
  case 's':
    return CLASS_SCOUT;
    break;
  case 'n':
    return CLASS_NINJA;
    break;
  case '*':
    return CLASS_MASTER;
    break;
  default:
    return CLASS_UNDEFINED;
    break;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
    case 'm':
      return (1 << 0);
      break;
    case 'c':
      return (1 << 1);
      break;
    case 't':
      return (1 << 2);
      break;
    case 'w':
      return (1 << 3);
      break;
    case 'p':
      return (1 << 4);
      break;
    case 'v':
      return (1 << 5);
      break;
    case 'b':
      return (1 << 6);
      break;
    case 's':
      return (1 << 7);
      break;
    default:
      return 0;
      break;
  }
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* MAG	CLE	THE	WAR	PAL	Vamp	Bard	Scout */
  {95,		95,	85,	80,	85,	90,	50,	85,         
  /* Ninja      Master */
   97,          100 },
  {100,		100,	12,	12,	50,	45,	35,	12,        
   100,         100},
  {25,		25,	0,	0,	12,	12,	0,	0,         
   30,          40},
  {SPELL,	SPELL,	SKILL,	SKILL,	SKILL,	SPELL,	SPELL,	SKILL,     
   SPELL,       SPELL}

};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 */
int guild_info[][3] = {

/* Daggerfall */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_CLERIC,	3004,	SCMD_NORTH},
  {CLASS_THIEF,		3027,	SCMD_EAST},
  {CLASS_WARRIOR,	3021,	SCMD_EAST},
  {CLASS_PALADIN,	128,	SCMD_EAST},
  {CLASS_VAMPYRE,	130,	SCMD_DOWN},
  {CLASS_BARD,		132,    SCMD_UP},
  {CLASS_SCOUT,		134,	SCMD_NORTH},

/* Elven City  */
  {-1 /* all */,	136,	SCMD_NORTH},

/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* New Sparta */
  {CLASS_MAGIC_USER,	5525,	SCMD_SOUTH},
  {CLASS_CLERIC,	5512,	SCMD_SOUTH},
  {CLASS_THIEF,		5532,	SCMD_SOUTH},
  {CLASS_WARRIOR,	5526,	SCMD_SOUTH},
  {CLASS_PALADIN,	138,	SCMD_WEST},
  {CLASS_VAMPYRE,	138,	SCMD_NORTH},
  {CLASS_BARD,		138,	SCMD_SOUTH},
  {CLASS_SCOUT,		138,	SCMD_EAST},

/* Rome */
  {-999 /* ALL */,	143,	SCMD_NORTH},

/* God Zone, master and ninja class */
  {CLASS_NINJA,         1421,   SCMD_NORTH},
  {CLASS_MASTER,        1421,   SCMD_NORTH},

/* this must go last -- add new guards above! */
{-1, -1, -1}};




/* THAC0 for classes and levels.  (To Hit Armor Class 0) */

/* [class], [level] (all) */
const int thaco[NUM_CLASSES][LVL_IMPL + 1] = {

/* MAGE */
  /* 0                   5                  10                  15	    */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 19,
  19, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17, 17,
       /* 20                  25                  30		      35 */
 /*               40                  45                  50          53 */  
  17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15, 
  15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
  13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11,
  11, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9},
/*90                  95                  100                 105 */ 

/* CLERIC */
  /* 0                   5                  10                  15      17 */
  {100, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 17, 17,
  17, 17, 17, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14,
/*18      20                  25                  30                  35 */
/*               40                  45                  50          53 */
  13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10,
  10, 10,  9,  9,  9,  9,  9,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  6,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   6,  6,  6,  6,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,
   3,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
/*90                  95                  100                 105 */

/* THIEF */
  /* 0                   5                  10                  15          */
  {100, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
  18, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15,
       /* 20                  25                  30                  35 */
 /*               40                  45                  50          53 */
  15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12,
  12, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   9,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,
   5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3},
/*90                  95                  100                 105 */

/* WARRIOR */
  /* 0                   5                  10                  15      17 */
  {100, 20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15,
  15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10,  9,  9,
/*18      20                  25                  30                  35 */
/*               40                  45                  50          53 */
   9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5,  5,  4,  4,  4,  3,  3,
   3,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
/*90                  95                  100                 105 */

/* PALADIN */
  /* 0                   5                  10                  15      17 */
  {100, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 17, 17,
  17, 17, 17, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14,
/*18      20                  25                  30                  35 */
/*               40                  45                  50          53 */
  13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10,
  10, 10,  9,  9,  9,  9,  9,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  6,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   6,  6,  6,  6,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,
   3,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
/*90                  95                  100                 105 */

/* VAMPYRE */
  /* 0                   5                  10                  15          */
  {100, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
  18, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15,
       /* 20                  25                  30                  35 */
 /*               40                  45                  50          53 */
  15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12,
  12, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   9,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,
   5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3},
/*90                  95                  100                 105 */

/* BARD */
  /* 0                   5                  10                  15          */
  {100, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18,
  18, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15,
       /* 20                  25                  30                  35 */
 /*               40                  45                  50          53 */
  15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12,
  12, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   9,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,
   5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3},
/*90                  95                  100                 105 */

/* SCOUT */
  /* 0                   5                  10                  15      17 */
  {100, 20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15,
  15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10,  9,  9,
/*18      20                  25                  30                  35 */
/*               40                  45                  50          53 */
   9,  8,  8,  8,  7,  7,  7,  6,  6,  6,  5,  5,  5,  4,  4,  4,  3,  3,
   3,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
/*    55                  60                  65                  70  71 */
/*72          75                  80                  85              89 */
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
/*90                  95                  100                 105 */

/* Ninja */
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}, 

/* Master */
{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}

};

/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data * ch)
{
  extern void race_apply(struct char_data *);
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_PALADIN:
    ch->real_abils.cha = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.wis = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.dex = table[4];
    ch->real_abils.intel = table[5];
    break;
  case CLASS_VAMPYRE:
    ch->real_abils.intel = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.cha = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.str = table[4];
    ch->real_abils.con = table[5];
    break;
  case CLASS_BARD:
    ch->real_abils.cha = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.con = table[5];
    break;
  case CLASS_SCOUT:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.cha = table[4];
    ch->real_abils.intel = table[5];
    break;
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARRIOR:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
  default:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
  }
  race_apply(ch);
  if (ch->real_abils.str > 18)
    ch->real_abils.str = 18;
  if (ch->real_abils.dex > 18)
    ch->real_abils.dex = 18;
  if (ch->real_abils.con > 18)
    ch->real_abils.con = 18;
  if (ch->real_abils.wis > 18)
    ch->real_abils.wis = 18;
  if (ch->real_abils.intel > 18)
    ch->real_abils.intel = 18;
  if (ch->real_abils.cha > 18)
    ch->real_abils.cha = 18;

  ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  void advance_level(struct char_data * ch);

  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  set_title(ch, "\0");
/* roll_real_abils(ch); */
  ch->points.max_hit = 10;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    break;
  case CLASS_WARRIOR:
    break;
  default:
    break;
  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int add_hp = 0, add_mana = 0, add_move = 0, x = 0;

  extern struct con_app_type con_app[];

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp += number(3, 8);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;

  case CLASS_CLERIC:
    add_hp += number(5, 10);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;

  case CLASS_THIEF:
    add_hp += number(7, 13);
    add_mana = 0;
    add_move = number(1, 3);
    break;

  case CLASS_WARRIOR:
    add_hp += number(10, 15);
    add_mana = 0;
    add_move = number(1, 3);
    break;

  case CLASS_PALADIN:
    add_hp += number(7, 12);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(1, 3);
    break;

  case CLASS_BARD:
    add_hp += number(5, 12);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(1, 3);
    break;

  case CLASS_VAMPYRE:
    add_hp += number(8, 12);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(1, 3);
    break;

  case CLASS_SCOUT:
    add_hp += number(10, 15);
    add_mana = 5;
    add_move = number(1, 3);
    break;

  case CLASS_NINJA:
    add_hp += number(10,15);
    add_mana = number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
    add_mana += add_mana = MIN(add_mana, 10);
    add_move += number(0,3);
    break;

  case CLASS_MASTER:
    add_hp += number(15,20);
    add_mana = number(GET_LEVEL(ch), (int) (2 * GET_LEVEL(ch)));
    add_mana += add_mana = MIN(add_mana, 15);
    add_move += number(0,6);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC
      || GET_CLASS(ch) == CLASS_PALADIN || GET_CLASS(ch) == CLASS_BARD ||
      GET_CLASS(ch) == CLASS_VAMPYRE)
    GET_PRACTICES(ch)++;
  else if (GET_CLASS(ch) == CLASS_NINJA)
    GET_PRACTICES(ch)+=2;
  else if (GET_CLASS(ch) == CLASS_MASTER)
    GET_PRACTICES(ch)+=3;
  else
  {
    x = GET_LEVEL(ch);
    while (x >= 0)
    {
      x -= 2;
      if (x == 0)
        GET_PRACTICES(ch)++;
    }
  }
     
  save_char(ch, NOWHERE);

  if(GET_LEVEL(ch) == 101 && GET_CLASS(ch) == CLASS_MASTER)
    x = GET_LEVEL(ch) + EXTRA_LEVEL(ch);
  else
    x = GET_LEVEL(ch);
  sprintf(buf, "%s advanced to level %d\r\n", GET_NAME(ch), x);
  send_to_all(buf);
  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), x);
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 21)
    return 2;	  /* level 1 - 7 */
  else if (level <= 39)
    return 3;	  /* level 8 - 13 */
  else if (level <= 60)
    return 4;	  /* level 14 - 20 */
  else if (level <= 90)
    return 5;	  /* level 21 - 28 */
  else if (level < LVL_IMMORT)
    return 6;	  /* all remaining mortal levels */
  else
    return 20;	  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj) {
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_MASTER) && IS_MASTER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NINJA) && IS_NINJA(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_VAMPYRE) && IS_VAMPYRE(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_BARD) && IS_BARD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_SCOUT) && IS_SCOUT(ch)))
	return 1;
  else
	return 0;
}




/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void) {
  int i, j;
  int cls_mage = (1 << CLASS_MAGIC_USER);
  int cls_cleric = (1 << CLASS_CLERIC);
  int cls_thief = (1 << CLASS_THIEF);
  int cls_warrior = (1 << CLASS_WARRIOR);
  
  // Assign a spell/skill to a a whole group of classes (0 is all)
  // For instance, { SKILL_SECOND_ATTACK, cls_mage | cls_cleric, 14 },
  // will give mages and clerics the SECOND_ATTACK skill at level 14.
  // More convenient than individual spell_level()s.  Use 0 to give
  // a skill to all the classes.
  //   -dkoepke
  int base_skl[][3] = {
    { SKILL_MOUNT , 0, 12 },
    { SKILL_RIDING, 0, 16 },
    
    { -1, -1 } // THIS MUST END THE LIST
  };
  
  // give all the base_skl[]'s
  for (j = 0; base_skl[j][0] != -1; j++)
    for (i = 0; i < NUM_CLASSES; i++)
      if (!base_skl[j][1] || IS_SET(base_skl[j][1], (1 << i)))
        spell_level(base_skl[j][0], i, base_skl[j][2]);

  // in my base patch, cls_mage, etc. are unused and that leads to
  // annyoing warnings, so here I'll use them...
  j = (cls_mage-cls_mage)+(cls_cleric-cls_cleric)+(cls_thief-cls_thief)+
      (cls_warrior-cls_warrior);
  
  /* BARD */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_BARD, 		1);
  spell_level(SPELL_CREATE_FOOD, CLASS_BARD, 		3);
  spell_level(SPELL_INFRAVISION, CLASS_BARD, 		9);
  spell_level(SKILL_STEAL, CLASS_BARD, 			12);
  spell_level(SPELL_DETECT_POISON, CLASS_BARD,		20);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_BARD, 	26);
  spell_level(SKILL_TAME, CLASS_BARD, 			32);
  spell_level(SPELL_CHARM, CLASS_BARD, 			38);
  spell_level(SPELL_ARMOR, CLASS_BARD, 			44);
  spell_level(SPELL_ARCANE_WORD, CLASS_BARD,  		48);
  spell_level(SPELL_SLEEP, CLASS_BARD, 			50);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_BARD,		53);
  spell_level(SKILL_SECOND_ATTACK, CLASS_BARD, 		56);
  spell_level(SPELL_DETECT_INVIS, CLASS_BARD, 		62);
  spell_level(SKILL_PICK_LOCK, CLASS_BARD, 		68);
  spell_level(SKILL_TRACK, CLASS_BARD, 			74);
  spell_level(SKILL_RESCUE, CLASS_BARD, 		80);
  spell_level(SKILL_THIRD_ATTACK, CLASS_BARD,		86);
  spell_level(SPELL_SANCTUARY, CLASS_BARD, 		90);

  /* VAMPYRES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_VAMPYRE, 	1);
  spell_level(SPELL_DETECT_INVIS, CLASS_VAMPYRE, 	3);
  spell_level(SPELL_INFRAVISION, CLASS_VAMPYRE, 	9);
  spell_level(SKILL_HIDE, CLASS_VAMPYRE, 		12);
  spell_level(SPELL_INVISIBLE, CLASS_VAMPYRE, 		20);
  spell_level(SKILL_TAME, CLASS_VAMPYRE, 		27);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_VAMPYRE, 	34);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_VAMPYRE, 	41);
  spell_level(SPELL_BLINDNESS, CLASS_VAMPYRE, 		48);
  spell_level(SPELL_SLEEP, CLASS_VAMPYRE, 		56);
  spell_level(SPELL_DETECT_POISON, CLASS_VAMPYRE, 	63);
  spell_level(SKILL_SNEAK, CLASS_VAMPYRE,		70);
  spell_level(SPELL_CHARM, CLASS_VAMPYRE, 		77);
  spell_level(SKILL_SECOND_ATTACK, CLASS_VAMPYRE, 	84);
  spell_level(SKILL_BACKSTAB, CLASS_VAMPYRE, 		91);
  spell_level(SKILL_THIRD_ATTACK, CLASS_VAMPYRE, 	98);

  /* MAGES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 	1);
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 		3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 	5);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 	7);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 	9);
  spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 	11);
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 	15);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 	18);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 	20);
  spell_level(SKILL_TAME, CLASS_MAGIC_USER, 		26);
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 		32);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 	38);
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 		44);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 	50);
  spell_level(SPELL_ARCANE_WORD, CLASS_MAGIC_USER,  	53);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 	56);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_MAGIC_USER,    59);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 	62);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 	68);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 	74);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 	80);
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 		82);
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 	86);
  spell_level(SPELL_METEOR_SWARM, CLASS_MAGIC_USER,     92);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MAGIC_USER, 	98);
 
  /* PALADINS */
  spell_level(SPELL_CURE_LIGHT, CLASS_PALADIN, 		1);
  spell_level(SPELL_CREATE_FOOD, CLASS_PALADIN, 	3);
  spell_level(SPELL_DETECT_ALIGN, CLASS_PALADIN, 	9);
  spell_level(SKILL_KICK, CLASS_PALADIN, 		12);
  spell_level(SKILL_TAME, CLASS_PALADIN, 		20);
  spell_level(SPELL_INFRAVISION, CLASS_PALADIN, 	27);
  spell_level(SPELL_BLESS, CLASS_PALADIN, 		34);
  spell_level(SPELL_DISPEL_EVIL, CLASS_PALADIN, 	44);
  spell_level(SPELL_HARM, CLASS_PALADIN, 		51);
  spell_level(SKILL_SECOND_ATTACK, CLASS_PALADIN, 	58);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_PALADIN, 	66);
  spell_level(SKILL_TRACK, CLASS_PALADIN,		73);
  spell_level(SKILL_THIRD_ATTACK, CLASS_PALADIN, 	80);
  spell_level(SPELL_GROUP_ARMOR, CLASS_PALADIN, 	87);
  spell_level(SPELL_SANCTUARY, CLASS_PALADIN, 		94);
  spell_level(SPELL_SUMMON, CLASS_PALADIN, 		98);

  /* CLERICS */
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 		1);
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 		3);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 	5);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 	7);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 	9);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 		12);
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC,           1);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 		16);
  spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 	20);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 		23);
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 		26);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 	29);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 		32);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 		35);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 		38);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 	41);
  spell_level(SKILL_TAME, CLASS_CLERIC, 		44);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 	47);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 		50);
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 		53);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 		56);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 		59);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 	62);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 		65);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 	68);
  spell_level(SPELL_HARM, CLASS_CLERIC, 		71);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 		74);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 	77);
  spell_level(SKILL_SECOND_ATTACK, CLASS_CLERIC, 	90);

  /* THIEVES */
  spell_level(SKILL_SNEAK, CLASS_THIEF, 		1);
  spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 		3);
  spell_level(SKILL_BACKSTAB, CLASS_THIEF, 		9);
  spell_level(SKILL_TAME, CLASS_THIEF, 			12);
  spell_level(SKILL_STEAL, CLASS_THIEF, 		20);
  spell_level(SKILL_HIDE, CLASS_THIEF, 			40);
  spell_level(SKILL_TRACK, CLASS_THIEF, 		55);
  spell_level(SKILL_SECOND_ATTACK, CLASS_THIEF, 	65);
  spell_level(SKILL_THIRD_ATTACK, CLASS_THIEF, 		85);

  /* SCOUTS */
  spell_level(SKILL_SNEAK, CLASS_SCOUT, 		1);
  spell_level(SKILL_KICK, CLASS_SCOUT, 			3);
  spell_level(SKILL_HIDE, CLASS_SCOUT, 			9);
  spell_level(SKILL_TRACK, CLASS_SCOUT, 		12);
  spell_level(SKILL_BACKSTAB, CLASS_SCOUT, 		20);
  spell_level(SKILL_SECOND_ATTACK, CLASS_SCOUT, 	50);
  spell_level(SKILL_TAME, CLASS_SCOUT, 			70);
  spell_level(SKILL_THIRD_ATTACK, CLASS_SCOUT, 		85);

  /* WARRIORS */
  spell_level(SKILL_KICK, CLASS_WARRIOR, 		1);
  spell_level(SKILL_TAME, CLASS_WARRIOR, 		3);
  spell_level(SKILL_TRACK, CLASS_WARRIOR, 		9);
  spell_level(SKILL_SECOND_ATTACK, CLASS_WARRIOR, 	20);
  spell_level(SKILL_BASH, CLASS_WARRIOR, 		45);
  spell_level(SKILL_THIRD_ATTACK, CLASS_WARRIOR, 	60);
  spell_level(SKILL_RESCUE, CLASS_WARRIOR, 		85);

  
  /* Ninja */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_NINJA,    1);
  spell_level(SPELL_ARMOR, CLASS_NINJA,            3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_NINJA,     5);
  spell_level(SPELL_INFRAVISION, CLASS_NINJA,      7);
  spell_level(SPELL_CHILL_TOUCH, CLASS_NINJA,      9);
  spell_level(SPELL_STRENGTH, CLASS_NINJA,         11);
  spell_level(SPELL_INVISIBLE, CLASS_NINJA,        15);
  spell_level(SPELL_BURNING_HANDS, CLASS_NINJA,    18);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_NINJA,    20);
  spell_level(SKILL_TAME, CLASS_NINJA,             26);
  spell_level(SPELL_CHARM, CLASS_NINJA,            32);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_NINJA,   38);
  spell_level(SPELL_SLEEP, CLASS_NINJA,            44);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_NINJA,   50);
  spell_level(SPELL_BLINDNESS, CLASS_NINJA,        56);
  spell_level(SPELL_DETECT_POISON, CLASS_NINJA,    62);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_NINJA,   68);
  spell_level(SPELL_COLOR_SPRAY, CLASS_NINJA,      74);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_NINJA,     80);
  spell_level(SPELL_CURSE, CLASS_NINJA,            82);
  spell_level(SPELL_FIREBALL, CLASS_NINJA,         86);
  spell_level(SPELL_METEOR_SWARM, CLASS_NINJA,     92);
  spell_level(SKILL_SECOND_ATTACK, CLASS_NINJA,    98);
  spell_level(SKILL_KICK, CLASS_NINJA,                1);
  spell_level(SKILL_TAME, CLASS_NINJA,                3);
  spell_level(SKILL_TRACK, CLASS_NINJA,               9);
  spell_level(SKILL_SECOND_ATTACK, CLASS_NINJA,       20);
  spell_level(SKILL_BASH, CLASS_NINJA,                45);
  spell_level(SKILL_THIRD_ATTACK, CLASS_NINJA,        60);
  spell_level(SKILL_RESCUE, CLASS_NINJA,              85);
  spell_level(SPELL_ARCANE_WORD, CLASS_NINJA,  		80);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_NINJA,  	90);

  /*  Master */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MASTER,    1);
  spell_level(SPELL_ARMOR, CLASS_MASTER,            3);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MASTER,     5);
  spell_level(SPELL_INFRAVISION, CLASS_MASTER,      7);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MASTER,      9);
  spell_level(SPELL_STRENGTH, CLASS_MASTER,         11);
  spell_level(SPELL_INVISIBLE, CLASS_MASTER,        15);
  spell_level(SPELL_BURNING_HANDS, CLASS_MASTER,    18);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MASTER,    20);
  spell_level(SKILL_TAME, CLASS_MASTER,             26);
  spell_level(SPELL_CHARM, CLASS_MASTER,            32);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MASTER,   38);
  spell_level(SPELL_SLEEP, CLASS_MASTER,            44);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MASTER,   50);
  spell_level(SPELL_BLINDNESS, CLASS_MASTER,        56);
  spell_level(SPELL_DETECT_POISON, CLASS_MASTER,    62);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MASTER,   68);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MASTER,      74);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MASTER,     80);
  spell_level(SPELL_CURSE, CLASS_MASTER,            82);
  spell_level(SPELL_FIREBALL, CLASS_MASTER,         86);
  spell_level(SPELL_METEOR_SWARM, CLASS_MASTER,     92);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MASTER,    98);
  spell_level(SKILL_KICK, CLASS_MASTER,                1);
  spell_level(SKILL_TAME, CLASS_MASTER,                3);
  spell_level(SKILL_TRACK, CLASS_MASTER,               9);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MASTER,       20);
  spell_level(SKILL_BASH, CLASS_MASTER,                45);
  spell_level(SKILL_THIRD_ATTACK, CLASS_MASTER,        60);
  spell_level(SKILL_RESCUE, CLASS_MASTER,              85);
  spell_level(SPELL_ARMOR, CLASS_MASTER,                1);
  spell_level(SPELL_CREATE_FOOD, CLASS_MASTER,          3);
  spell_level(SPELL_CREATE_WATER, CLASS_MASTER,         5);
  spell_level(SPELL_DETECT_POISON, CLASS_MASTER,        7);
  spell_level(SPELL_DETECT_ALIGN, CLASS_MASTER,         9);
  spell_level(SPELL_CURE_BLIND, CLASS_MASTER,           12);
  spell_level(SPELL_BLESS, CLASS_MASTER,                15);
  spell_level(SPELL_DETECT_INVIS, CLASS_MASTER,         20);
  spell_level(SPELL_BLINDNESS, CLASS_MASTER,            23);
  spell_level(SPELL_INFRAVISION, CLASS_MASTER,          26);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_MASTER,       29);
  spell_level(SPELL_GROUP_ARMOR, CLASS_MASTER,          32);
  spell_level(SPELL_CURE_CRITIC, CLASS_MASTER,          35);
  spell_level(SPELL_SUMMON, CLASS_MASTER,               38);
  spell_level(SPELL_REMOVE_POISON, CLASS_MASTER,        41);
  spell_level(SKILL_TAME, CLASS_MASTER,                 44);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_MASTER,       47);
  spell_level(SPELL_EARTHQUAKE, CLASS_MASTER,           50);
  spell_level(SPELL_DISPEL_EVIL, CLASS_MASTER,          53);
  spell_level(SPELL_DISPEL_GOOD, CLASS_MASTER,          56);
  spell_level(SPELL_SANCTUARY, CLASS_MASTER,            59);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_MASTER,       62);
  spell_level(SPELL_HEAL, CLASS_MASTER,                 65);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_MASTER,      68);
  spell_level(SPELL_HARM, CLASS_MASTER,                 71);
  spell_level(SPELL_GROUP_HEAL, CLASS_MASTER,           74);
  spell_level(SPELL_REMOVE_CURSE, CLASS_MASTER,         77);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MASTER,        90);
  spell_level(SKILL_SNEAK, CLASS_MASTER,                 1);
  spell_level(SKILL_PICK_LOCK, CLASS_MASTER,             3);
  spell_level(SKILL_BACKSTAB, CLASS_MASTER,              9);
  spell_level(SKILL_TAME, CLASS_MASTER,                  12);
  spell_level(SKILL_STEAL, CLASS_MASTER,                 20);
  spell_level(SKILL_HIDE, CLASS_MASTER,                  40);
  spell_level(SKILL_TRACK, CLASS_MASTER,                 55);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MASTER,         65);
  spell_level(SKILL_THIRD_ATTACK, CLASS_MASTER,          85);
  spell_level(SPELL_ARCANE_WORD, CLASS_MASTER,  	80);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_MASTER,  	90);
}

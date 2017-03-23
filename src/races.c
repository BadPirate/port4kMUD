/***************************************************************
 * File: Races.c                                               *
 * Usage: Source-file for race-specific code                   *
 * Author: Henrik Stuart <hstuart@geocities.com>               *
 **************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"

/* Local functions */
const char *race_abbrevs[];
const char *pc_race_types[];
const char *race_menu;
int parse_race(char arg);
long find_race_bitvector(char arg);
int invalid_race(struct char_data *ch, struct obj_data *obj);

const char *race_abbrevs[] = {
  "Hum",
  "Dwa",
  "Elf",
  "Hob",
  "Wlf",
  "Gia",
  "Gnm",
  "Pix",
  "Gry",
  "Brn",
  "Trl",
  "Lzr",
  "Rat",
  "\n"
};

const char *pc_race_types[] = {
  "Human",
  "Dwarf",
  "Elf",
  "Hobbit",
  "Wolf",
  "Giant",
  "Gnome",
  "Pixie",
  "Gargoyle",
  "Brownie",
  "Troll",
  "\n"
};

const char *race_menu =
"\r\n"
"Select a race:\r\n"
"  1) Human	No Modifiers\r\n"
"  2) Dwarf	+2 Con +1 Str -2 Cha -1 Dex\r\n"
"  3) Elf	+2 Int +2 Cha +1 Dex -3 Con -2 Str\r\n"
"  4) Hobbit	+1 Dex -1 Str\r\n"
"  5) Wolf      +1 Dex +1 Cha -1 Con -1 Str\r\n"
"  6) Giant     +3 Str -2 Int -1 Wis\r\n"
"  7) Gnome     +2 Int -1 Wis -1 Dex\r\n"
"  8) Pixie     +2 Dex -2 Str\r\n"
"  9) Gargoyle  +2 Con +1 Str -2 Int -1 Cha\r\n"
"  A) Brownie   +1 Int +1 Dex -2 Cha\r\n"
"  B) Troll     +2 Str -2 Cha\r\n";

int parse_race(char arg) {

  switch (arg) {
    case '1': return RACE_HUMAN;
    case '2': return RACE_DWARF;
    case '3': return RACE_ELF;
    case '4': return RACE_HOBBIT;
    case '5': return RACE_WOLF;
    case '6': return RACE_GIANT;
    case '7': return RACE_GNOME;
    case '8': return RACE_PIXIE;
    case '9': return RACE_GARGOYLE;
    case 'a':
    case 'A': return RACE_BROWNIE;
    case 'b':
    case 'B': return RACE_TROLL;
    default: return RACE_UNDEFINED;
  }
}

long find_race_bitvector(char arg) {
  
  switch (arg) {
    case '1': return (1 << RACE_HUMAN);
    case '2': return (1 << RACE_DWARF);
    case '3': return (1 << RACE_ELF);
    case '4': return (1 << RACE_HOBBIT);
    case '5': return (1 << RACE_WOLF);
    case '6': return (1 << RACE_GIANT);
    case '7': return (1 << RACE_GNOME);
    case '8': return (1 << RACE_PIXIE);
    case '9': return (1 << RACE_GARGOYLE);
    case 'A': return (1 << RACE_BROWNIE);
    case 'B': return (1 << RACE_TROLL);
    default: return 0;
  }
}

int invalid_race(struct char_data *ch, struct obj_data *obj) {
  if (/* (IS_OBJ_STAT(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch)) || */
      (IS_OBJ_STAT(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch)) ||
      /* (IS_OBJ_STAT(obj, ITEM_ANTI_ELF) && IS_ELF(ch)) ||  */
      (IS_OBJ_STAT(obj, ITEM_ANTI_HOBBIT) && IS_HOBBIT(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WOLF) && IS_WOLF(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GIANT) && IS_GIANT(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_PIXIE) && IS_PIXIE(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_GARGOYLE) && IS_GARGOYLE(ch)))
    return 1;
  else
    return 0;
}

void race_apply(struct char_data *ch)
{
  switch (GET_RACE(ch)) {
  case RACE_HUMAN:
    break;
  case RACE_DWARF:
    ch->real_abils.str += 2;
    ch->real_abils.con += 1;
    ch->real_abils.dex -= 2;
    ch->real_abils.cha -= 1;
    break;
  case RACE_ELF:
    ch->real_abils.str -= 2;
    ch->real_abils.con -= 3;
    ch->real_abils.dex += 1;
    ch->real_abils.intel += 2;
    ch->real_abils.cha += 2;
    break;
  case RACE_HOBBIT:
    ch->real_abils.str -= 1;
    ch->real_abils.dex += 1;
    break;
  case RACE_WOLF:
    ch->real_abils.dex += 1;
    ch->real_abils.cha += 1;
    ch->real_abils.con -= 1;
    ch->real_abils.str -= 1;
    break;
  case RACE_GIANT:
    ch->real_abils.str += 3;
    ch->real_abils.intel -= 2;
    ch->real_abils.wis -= 1;
    break;
  case RACE_GNOME:
    ch->real_abils.intel += 2;
    ch->real_abils.wis -= 1;
    ch->real_abils.dex -= 1;
    break;
  case RACE_PIXIE:
    ch->real_abils.dex += 2;
    ch->real_abils.str -= 2;
    break;
  case RACE_GARGOYLE:
    ch->real_abils.con += 2;
    ch->real_abils.str += 1;
    ch->real_abils.intel -= 2;
    ch->real_abils.cha -= 1;
    break;
  case RACE_BROWNIE:
    ch->real_abils.intel += 1;
    ch->real_abils.dex += 1;
    ch->real_abils.cha -= 2;
    break;
  case RACE_TROLL:
    ch->real_abils.str += 2;
    ch->real_abils.cha -= 2;
    break;
  }
}

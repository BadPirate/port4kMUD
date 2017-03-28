/*************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "clan.h"
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])

/* local functions */
int true_clan(int);

/* extern variables */
extern struct clan_info *clan_index;
extern int cheese_allowed;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct command_info cmd_info[];
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char player_cmd[];
extern char wizlist_cmd[];
extern char *policies;
extern char *handbook;
extern char *dirs[];
extern char *AEdirs[];
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *connected_types[];
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern char *room_bits[];
extern char *spells[];

/* global */
int boot_high = 0;

long find_class_bitvector(char arg);

void list_scanned_chars(struct char_data * list, struct char_data * ch, 
			int distance, int door)
{
  const char *how_far[] = {
    "close by",
    "a ways off",
    "far off to the"
  };

  struct char_data *i;
  int count = 0;
  *buf = '\0';

  for (i = list; i; i = i->next_in_room)
    if (CAN_SEE(ch, i))
     count++;

  if (!count)
    return;

  for (i = list; i; i = i->next_in_room) {
    if (!CAN_SEE(ch, i))
      continue;
    if (!*buf)
      sprintf(buf, "You see %s", GET_NAME(i));
    else
      sprintf(buf, "%s%s", buf, GET_NAME(i));
    if (--count > 1)
      strcat(buf, ", ");
    else if (count == 1)
      strcat(buf, " and ");
    else {
      sprintf(buf2, " %s %s.\r\n", how_far[distance], dirs[door]);
      strcat(buf, buf2);
    }

  }
  send_to_char(buf, ch);
}

void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  bool found;

  *buf = '\0';
  if ((mode == 0) && object->description)
    strcpy(buf, object->description);
  else if (object->short_description && ((mode == 1) ||
				 (mode == 2) || (mode == 3) || (mode == 4)))
    strcpy(buf, object->short_description);
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
  }
  if (mode != 3) {
    found = FALSE;
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invisible)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_BLESS) && IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      strcat(buf, " ..It glows blue!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ..It glows yellow!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, " ..It has a soft glowing aura!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " ..It emits a faint humming sound!");
      found = TRUE;
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                           bool show)
{
  struct obj_data *i, *j;
  char buf[10];
  bool found;
  int num;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
      num = 0;
      for (j = list; j != i; j = j->next_content)
        if (j->item_number==NOTHING) {
            if(strcmp(j->short_description,i->short_description)==0) break;
        }
        else if (j->item_number==i->item_number) break;
      if (j!=i) continue;
      for (j = i; j; j = j->next_content)
        if (j->item_number==NOTHING) {
            if(strcmp(j->short_description,i->short_description)==0) num++;
          }
        else if (j->item_number==i->item_number) num++;

      if (CAN_SEE_OBJ(ch, i)) {
          if (num!=1) {
                sprintf(buf,"(%2i) ",num);
                send_to_char(buf,ch);
            }
          show_obj_to_char(i, ch, mode);
          found = TRUE;
      }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}
 
char *short_char_cond(struct char_data *i) {
  static char buf[100];
  int percent;
  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */
  buf[0]='\0';
  if (percent >= 100)
    strcat(buf, "excellent");
  else if (percent >= 90)
    strcat(buf, "few scratches");
  else if (percent >= 75)
    strcat(buf, "small wounds");
  else if (percent >= 50)
    strcat(buf, "many wounds");
  else if (percent >= 30)
    strcat(buf, "nasty wounds");
  else if (percent >= 15)
    strcat(buf, "pretty hurt");
  else if (percent >= 0)
    strcat(buf, "awful");
  else
    strcat(buf, "bleeding awfully");
  return(buf);
}

void diag_char_to_char(struct char_data * i, struct char_data * ch) {
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.\r\n");

  send_to_char(buf, ch);
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);
  
  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    if (RIDING(i) == ch)
      act("$e is mounted on you.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted upon %s.", PERS(RIDING(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  } else if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room) {
    if (RIDDEN_BY(i) == ch)
      act("You are mounted upon $m.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted by %s.", PERS(RIDDEN_BY(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  }

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(where[j], ch);
	show_obj_to_char(GET_EQ(i, j), ch, 1);
      }
  }
  if (ch != i && (GET_CLASS(ch) == CLASS_THIEF || GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}


void list_one_char(struct char_data * i, struct char_data * ch)
{
  char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (IS_AFFECTED(i, AFF_INVISIBLE))
      strcpy(buf, "*");
    else
      *buf = '\0';

    if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	strcat(buf, "(Red Aura) ");
      else if (IS_GOOD(i))
	strcat(buf, "(Blue Aura) ");
    }
    strcat(buf, i->player.long_descr);
    if (!IS_AFFECTED(i, AFF_STATUE)) {
      send_to_char(buf, ch);
    } else {
      act("A statue of $n is here.", FALSE, i, 0, ch, TO_VICT);
    }

    if (IS_AFFECTED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);

    return;
  }
  if (IS_NPC(i)) {
    strcpy(buf, i->player.short_descr);
    CAP(buf);
  } else
    sprintf(buf, "%s %s", i->player.name, GET_TITLE(i));

  if (IS_AFFECTED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (IS_AFFECTED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");

  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    strcat(buf, " is here, mounted upon ");
    if (RIDING(i) == ch)
      strcat(buf, "you");
    else
      strcat(buf, PERS(RIDING(i), ch));
    strcat(buf, ".");
  } else if (GET_POS(i) != POS_FIGHTING)
    strcat(buf, positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }

  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " (Red Aura)");
    else if (IS_GOOD(i))
      strcat(buf, " (Blue Aura)");
  }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (IS_AFFECTED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
}



void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room)
        continue;
        
      if (CAN_SEE(ch, i) || (!cheese_allowed && GET_LEVEL(i) < LVL_IMMORT))
	list_one_char(i, ch);
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
	       IS_AFFECTED(i, AFF_INFRAVISION))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
    }
}


void do_auto_exits(struct char_data * ch)
{
  int door;

  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
      sprintf(buf, "%s%s ", buf, AEdirs[door]);

  sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM),
	  *buf ? buf : "None! ", CCNRM(ch, C_NRM));

  send_to_char(buf2, ch);
}

ACMD(do_scan)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  /* may want to add more restrictions here, too */
  send_to_char("You quickly scan the area.\r\n", ch);
  for (door = 0; door < NUM_OF_DIRS - 2; door++) /* don't scan up/down */
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
        !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)
        && !IS_DARK(EXIT(ch, door)->to_room)) {
      if (world[EXIT(ch, door)->to_room].people) {
        list_scanned_chars(world[EXIT(ch, door)->to_room].people, ch, 0, door);
      } else if (_2ND_EXIT(ch, door) && _2ND_EXIT(ch, door)->to_room !=
                 NOWHERE && !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_CLOSED)
                 && !IS_DARK(_2ND_EXIT(ch, door)->to_room)) {
   /* check the second room away */
        if (world[_2ND_EXIT(ch, door)->to_room].people) {
          list_scanned_chars(world[_2ND_EXIT(ch, door)->to_room].people, ch, 1,door);
        } else if (_3RD_EXIT(ch, door) && _3RD_EXIT(ch, door)->to_room !=
                   NOWHERE && !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_CLOSED)
                   && !IS_DARK(_3RD_EXIT(ch, door)->to_room)) {
          /* check the third room */
          if (world[_3RD_EXIT(ch, door)->to_room].people) {
            list_scanned_chars(world[_3RD_EXIT(ch, door)->to_room].people, ch, 2, door); 
        }
      }
    }
  }
}

ACMD(do_rlist)
{
  extern struct room_data *world;
  extern int top_of_world;

  int first, last, nr, found = 0;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char("Usage: rlist <begining number> <ending number>\r\n", ch);
    return;
  }

  first = atoi(buf);
  last = atoi(buf2);

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Values must be between 0 and 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
   send_to_char("Second value must be greater than first.\n\r", ch);
    return;
  }

  for (nr = 0; nr <= top_of_world && (world[nr].number <= last); nr++) {
    if (world[nr].number >= first) {
      sprintf(buf, "%5d. [%5d] (%3d) %s\r\n", ++found,
              world[nr].number, world[nr].zone,
              world[nr].name);
      send_to_char(buf, ch);
    }
  }

  if (!found)
    send_to_char("No rooms were found in those parameters.\n\r", ch);
}


ACMD(do_mlist)
{
  extern struct index_data *mob_index;
  extern struct char_data *mob_proto;
  extern int top_of_mobt;

  int first, last, nr, found = 0;
  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char("Usage: mlist <begining number> <ending number>\r\n", ch);
    return;
  }

  first = atoi(buf);
  last = atoi(buf2);

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Values must be between 0 and 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
    send_to_char("Second value must be greater than first.\n\r", ch);
    return;
  }

  for (nr = 0; nr <= top_of_mobt && (mob_index[nr].virtual <= last); nr++) {
    if (mob_index[nr].virtual >= first) {
      sprintf(buf, "%5d. [%5d] %s\r\n", ++found,
              mob_index[nr].virtual,
              mob_proto[nr].player.short_descr);
      send_to_char(buf, ch);
    }
  }

  if (!found)
    send_to_char("No mobiles were found in those parameters.\n\r", ch);
}


ACMD(do_olist)
{
  extern struct index_data *obj_index;
  extern struct obj_data *obj_proto;
  extern int top_of_objt;

  int first, last, nr, found = 0;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char("Usage: olist <begining number> <ending number>\r\n", ch);
    return;
  }
  first = atoi(buf);
  last = atoi(buf2);

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Values must be between 0 and 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
    send_to_char("Second value must be greater than first.\n\r", ch);
    return;
  }

  for (nr = 0; nr <= top_of_objt && (obj_index[nr].virtual <= last); nr++) {
    if (obj_index[nr].virtual >= first) {
      sprintf(buf, "%5d. [%5d] %s\r\n", ++found,
              obj_index[nr].virtual,
              obj_proto[nr].short_description);
      send_to_char(buf, ch);
    }
  }

  if (!found)
    send_to_char("No objects were found in those parameters.\n\r", ch);
}

ACMD(do_exits) {
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT)
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		world[EXIT(ch, door)->to_room].number,
		world[EXIT(ch, door)->to_room].name);
      else {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else {
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }
  send_to_char(CCCYN(ch, C_NRM), ch);
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit((long) ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprintf(buf2, "[%5d] %s [ %s]", world[ch->in_room].number,
	    world[ch->in_room].name, buf);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\r\n", ch);

  if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
    send_to_char(world[ch->in_room].description, ch); 

  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  /* now list characters & objects */
  send_to_char(CCGRN(ch, C_NRM), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  send_to_char(CCYEL(ch, C_NRM), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  send_to_char(CCNRM(ch, C_NRM), ch);
}



void look_in_direction(struct char_data * ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }
  } else
    send_to_char("Nothing special there...\r\n", ch);
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char("There's nothing inside that!\r\n", ch);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  sprintf(buf, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
	  sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, 0);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  if (bits) {			/* If an object was found back in
				 * generic_find */
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
}


ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have %d gold coins.\r\n", GET_GOLD(ch));
    send_to_char(buf, ch);
  }
}


ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(where[i], ch);
	show_obj_to_char(GET_EQ(ch, i), ch, 1);
	found = TRUE;
      } else {
	send_to_char(where[i], ch);
	send_to_char("Something.\r\n", ch);
	found = TRUE;
      }
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}


ACMD(do_time)
{
  char *suf;
  int weekday, day;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{
  static char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
  "lit by flashes of lightning"};

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    (weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}


ACMD(do_help)
{
  extern int top_of_helpt;
  extern struct help_index_element *help_table;
  extern char *help;

  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char("No help available.\r\n", ch);
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char("There is no help on that word.\r\n", ch);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      page_string(ch->desc, help_table[mid].entry, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}


/*********************************************************************
* New 'do_who' by Daniel Koepke [aka., "Argyle Macleod"] of The Keep *
******************************************************************* */

char *WHO_USAGE =
  "Usage: who [minlev[-maxlev]] [-n name] [-c classes] [-rzqimo]\r\n"
  "\r\n"
  "Classes: (M)age, (C)leric, (T)hief, (W)arrior\r\n"
  "\r\n"
  " Switches: \r\n"
  "_.,-'^'-,._\r\n"
  "\r\n"
  "  -r = who is in the current room\r\n"
  "  -z = who is in the current zone\r\n"
  "\r\n"
  "  -q = only show questers\r\n"
  "  -i = only show immortals\r\n"
  "  -m = only show mortals\r\n"
  "  -o = only show outlaws\r\n"
  "\r\n";

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *wch;
  extern struct townstart_struct townstart[];
  char idle_color[3],  god_name[MAX_STRING_LENGTH];
  char Imm_buf[MAX_STRING_LENGTH], Mort_buf[MAX_STRING_LENGTH];
  char Short_buf[MAX_STRING_LENGTH], name_search[MAX_NAME_LENGTH+1], mode;
  char town[10], temp_title[MAX_TITLE_LENGTH], title_buf[MAX_TITLE_LENGTH];
  int low = 0, high = LVL_IMPL, showclass = 0, x, idle, cb;  
  int Wizards = 0, Mortals = 0, Short = 0, max_title;
  bool who_room = FALSE, who_zone = FALSE, who_quest = 0, outlaws = FALSE, 
       noimm = FALSE, nomort = FALSE;
  
  size_t i;
  int mln;
  
  const char *G_WizLevels[LVL_IMPL - (LVL_IMMORT-1)] = {
    "&w     Angel    &n ",
    "&g      God   &n   ",
    "&rCo-Implementor&n ",
    "&c* Implementor *&n",
    "&b* LORD OF ALL *&n"
  };

  const char *B_WizLevels[LVL_IMPL - (LVL_IMMORT-1)] = {
    "&m     Demon     &n",
    "&m     Devil     &n",
    "&R*&r   Nemesis   &R*&n",
    "&R*&r *&R  Hades  &r*&R *&n",
    "&f* Major Pain  *&n" 
  };
  
  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  /* the below is from stock CircleMUD -- found no reason to rewrite it */
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
	outlaws = TRUE;
	strcpy(buf, buf1);
	break;
      case 'z':
	who_zone = TRUE;
	strcpy(buf, buf1);
	break;
      case 'q':
	who_quest = TRUE;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = TRUE;
	strcpy(buf, buf1);
	break;
      case 'c':
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      case 'i':
        nomort = TRUE;
        strcpy(buf, buf1);
        break;
      case 'm':
        noimm = TRUE;
        strcpy(buf, buf1);
        break;
      default:
	send_to_char(WHO_USAGE, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_USAGE, ch);
      return;
    }
  }				/* end while (parser) */

  strcpy(Imm_buf, "&vImmortals&w currently on &bPort 4000&n\r\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n");
  strcpy(Mort_buf,"&rMortals&w currently on &bPort 4000&n\r\n-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\r\n");

  strcpy(Short_buf,"People currently on &bPort 4000&n-\r\n");
/* The dagger */
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected)
      continue;

    if (d->original)
      wch = d->original;
    else if (!(wch = d->character))
      continue;

    if (!CAN_SEE(ch, wch))
      continue;
    if (GET_LEVEL(wch) < low || GET_LEVEL(wch) > high)
      continue;
    if ((noimm && GET_LEVEL(wch) >= LVL_IMMORT) || (nomort && GET_LEVEL(wch) < LVL_IMMORT))
      continue;
    if (*name_search && str_cmp(GET_NAME(wch), name_search) && !strstr(GET_TITLE(wch), name_search))
      continue;
    if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER) && !PLR_FLAGGED(wch, PLR_THIEF))
      continue;
    if (who_quest && !PRF_FLAGGED(wch, PRF_QUEST))
      continue;
    if (who_zone && world[ch->in_room].zone != world[wch->in_room].zone)
      continue;
    if (who_room && (wch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(wch))))
      continue;

    idle = d->character->char_specials.timer * SECS_PER_MUD_HOUR / 
           SECS_PER_REAL_MIN;

    if(idle == 0)
      strcpy(idle_color, "&c");
    else if (idle > 40)
      strcpy(idle_color, "&r");
    else if (idle > 20)
      strcpy(idle_color, "&y");
    else
      strcpy(idle_color, "&g");
  
    if (GET_LEVEL(wch) >= LVL_IMMORT)
    {
      if (GET_ALIGNMENT(wch) >= 0)
        strcpy(god_name, G_WizLevels[GET_LEVEL(wch)-LVL_IMMORT]);
      else
        strcpy(god_name, B_WizLevels[GET_LEVEL(wch)-LVL_IMMORT]);
    }

    /* THE TOWN SUBRUTINE */
    strcpy(town, "&nO");
    if(!PLR_FLAGGED(wch, PLR_LOADROOM))
      strcpy(town, townstart[0].shortname);
    else for(x=0; townstart[x].minlevel != -1; x++)
      if (GET_LOADROOM(wch) == townstart[x].loadroom)
        strcpy(town, townstart[x].shortname);

    for(i=0, cb=0; i < NUM_CLASSES; i++)
      if(IS_SET(wch->player_specials->saved.classes_been, (1 << i)))
        cb++;
    if (GET_LEVEL(wch) >= LVL_IMMORT) {
      if (subcmd != SCMD_SHORT)
        sprintf(Imm_buf, "%s[%s] (%s&n) &n%s&n ", Imm_buf, god_name, 
                town, GET_NAME(wch));
      else if (subcmd == SCMD_SHORT)
      {
        sprintf(Short_buf,"%s&n{ &n%15s &n[&c%3d&n] } ", Short_buf,
                GET_NAME(wch), GET_LEVEL(wch) );
        Short++;
        if (Short >= 3)
        {
          sprintf(Short_buf, "%s\r\n", Short_buf);
          Short = 0;
        }
      }
      Wizards++;
    } else {
      if (GET_LEVEL(wch) == 101)
        if(GET_CLASS(wch) == CLASS_MASTER)
        {
          x = GET_LEVEL(wch) + EXTRA_LEVEL(wch);
          sprintf(Mort_buf, "%s[&mAVATAR&c %3d&n] (%s&n) %s%s&n ", 
                  Mort_buf, x, town, 
                  idle_color, GET_NAME(wch));
        }
        else
          sprintf(Mort_buf, "%s[&mAVATAR&c  %s&n] (%s&n) %s%s&n ", 
                  Mort_buf, CLASS_ABBR(wch), town, idle_color, GET_NAME(wch));
      else if (subcmd != SCMD_SHORT)
        switch(cb)
        {
          case 0:
        sprintf(Mort_buf, "%s[%3d %s %s] (%s&n) %s%s&n ", Mort_buf, 
                GET_LEVEL(wch), CLASS_ABBR(wch), RACE_ABBR(wch), town, 
                idle_color, GET_NAME(wch));
                break;
          case 1:
        sprintf(Mort_buf, "%s[&r%3d&n %s %s] (%s&n) %s%s&n ", Mort_buf, 
                GET_LEVEL(wch), CLASS_ABBR(wch), RACE_ABBR(wch), town, 
                idle_color, GET_NAME(wch));
                break;
          case 2:
        sprintf(Mort_buf, "%s[&y%3d&n %s %s] (%s&n) %s%s&n ", Mort_buf, 
                GET_LEVEL(wch), CLASS_ABBR(wch), RACE_ABBR(wch), town, 
                idle_color, GET_NAME(wch));
                break;
          case 3:
        sprintf(Mort_buf, "%s[&g%3d&n %s %s] (%s&n) %s%s&n ", Mort_buf, 
                GET_LEVEL(wch), CLASS_ABBR(wch), RACE_ABBR(wch), town, 
                idle_color, GET_NAME(wch));
                break;
          default:
        sprintf(Mort_buf, "%s[&m%3d&n %s %s] (%s&n) %s%s&n ", Mort_buf, 
                GET_LEVEL(wch), CLASS_ABBR(wch), RACE_ABBR(wch), town, 
                idle_color, GET_NAME(wch));
                break;
        }
      if (subcmd == SCMD_SHORT)
      {
        switch(cb)
        {
          case 0:
            sprintf(Short_buf,"%s&n( %s%15s &n[&n%3d&n] ) ", Short_buf,
                    idle_color, GET_NAME(wch), GET_LEVEL(wch) );
            break;
          case 1:
            sprintf(Short_buf,"%s&n( %s%15s &n[&r%3d&n] ) ", Short_buf,
                    idle_color, GET_NAME(wch), GET_LEVEL(wch) );
            break;
          case 2:
            sprintf(Short_buf,"%s&n( %s%15s &n[&y%3d&n] ) ", Short_buf,
                    idle_color, GET_NAME(wch), GET_LEVEL(wch) );
            break;
          case 3:
            sprintf(Short_buf,"%s&n( %s%15s &n[&g%3d&n] ) ", Short_buf,
                    idle_color, GET_NAME(wch), GET_LEVEL(wch) );
            break;
          default:
            sprintf(Short_buf,"%s&n( %s%15s &n[&m%3d&n] ) ", Short_buf,
                    idle_color, GET_NAME(wch), GET_LEVEL(wch) );
            break;
        }
        Short++;
        if (Short >= 3)
        {
          sprintf(Short_buf, "%s\r\n", Short_buf);
          Short = 0;
        }
      }

      Mortals++;
    }

    if (subcmd != SCMD_SHORT)
    {
      *buf = '\0';
      if (GET_INVIS_LEV(wch))
        sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(wch));
      else if (IS_AFFECTED(wch, AFF_INVISIBLE))
        strcat(buf, " (invis)");
 
      if (PLAYERCLAN(wch) != 0 && CLANRANK(wch) > CLAN_APPLY)
      {
        if(true_clan(PLAYERCLAN(wch)) != -1)
          sprintf(buf, " <%s>", CLANNAME(clan_index[true_clan(PLAYERCLAN(wch))]));
        else
          strcat(buf, " &rCLAN BUG&n");
      }
      if (PLR_FLAGGED(wch, PLR_MAILING))
        strcat(buf, " (mailing)");
      else if (PLR_FLAGGED(wch, PLR_WRITING))
        strcat(buf, " (writing)");

      if (PRF_FLAGGED(wch, PRF_DEAF))
        strcat(buf, " (deaf)");
      if (PRF_FLAGGED(wch, PRF_NOTELL))
        strcat(buf, " (notell)");
      if (PRF_FLAGGED(wch, PRF_QUEST))
        strcat(buf, " (quest)");
      if (PLR_FLAGGED(wch, PLR_THIEF))
        strcat(buf, " (THIEF)");
      if (PLR_FLAGGED(wch, PLR_KILLER))
        strcat(buf, " (KILLER)");
      if (PRF_FLAGGED(wch, PRF_AFK))
        strcat(buf, " (AFK)");
      if (PRF_FLAGGED(wch, PRF_PKILL))
        strcat(buf, " (PK)");
      if (GET_LEVEL(wch) >= LVL_IMMORT)
        strcat(buf, CCNRM(ch, C_SPR));
      
      if (GET_LEVEL(wch) < LVL_IMMORT)
        max_title = 80-(18+strlen(GET_NAME(wch)));
      else
        max_title = 80-(23+strlen(GET_NAME(wch)));

      if ( strlen(GET_TITLE(wch)) > max_title ||
           strlen(GET_TITLE(wch)) == 0)
      {
        set_title(wch, "the titleless.");
        sprintf(buf2, "Your title was too long or short, it was changed to:\r\n%s\r\n",
                GET_TITLE(wch));
        send_to_char(buf2, wch);
      }
 
      *title_buf = '\0';
      *temp_title = '\0';
      strcpy(temp_title, GET_TITLE(wch));
      mln=max_title-strlen(buf); if(mln<0)mln=0;
      for(x=0; x < mln; x++)
        title_buf[x] = temp_title[x];
      title_buf[mln]='\0';
      strcat(title_buf, "&n");
      strcat(title_buf, buf);
      strcat(title_buf, "\r\n");

      if (GET_LEVEL(wch) < LVL_IMMORT)
        strcat(Mort_buf, title_buf);
      else
        strcat(Imm_buf, title_buf);
    }
  }

  if (subcmd != SCMD_SHORT)
  {
    if (Wizards) {
      page_string(ch->desc, Imm_buf, 0);
      send_to_char("\r\n", ch);
    }
  
    if (Mortals) {
      page_string(ch->desc, Mort_buf, 0);
      send_to_char("\r\n", ch);
    }
  
    if ((Wizards + Mortals) == 0)
      strcpy(buf, "No wizards or mortals are currently visible to you.\r\n");
    if (Wizards)
      sprintf(buf, "There %s %d visible immortal%s%s",
                   (Wizards == 1 ? "is" : "are"),
                   Wizards,
                   (Wizards == 1 ? "" : "s"),
                   (Mortals ? " and there" : "."));
    if (Mortals)
      sprintf(buf, "%s %s %d visible mortal%s.",
                   (Wizards ? buf : "There"),
                   (Mortals == 1 ? "is" : "are"),
                   Mortals,
                   (Mortals == 1 ? "" : "s"));
    strcat(buf, "\r\n");
  
    if ((Wizards + Mortals) > boot_high)
      boot_high = Wizards+Mortals;
    sprintf(buf, "%sThere is a boot time high of %d player%s.\r\n", buf, boot_high, (boot_high == 1 ? "" : "s"));
    send_to_char(buf, ch);
  }
  else if (subcmd == SCMD_SHORT)
  {
    page_string(ch->desc, Short_buf, 0);
    if ((Wizards + Mortals) > boot_high)
      boot_high = Wizards+Mortals;
    sprintf(buf, "\r\nMortals: %d Immortals: %d High: %d\r\n", Mortals, 
            Wizards, boot_high);
    send_to_char(buf, ch);
  }
}

int true_clan(int clannum)
{
  extern int clan_top;
  int ccmd;

  for (ccmd = 0; ccmd < clan_top; ccmd++)
    if (CLANNUM(clan_index[ccmd]) == clannum)
      return ccmd;
  return -1;
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  extern char *connected_types[];
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, *format, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  size_t i;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "Num Class   Name         State          Idl Login@   Site\r\n");
  strcat(line,
	 "--- ------- ------------ -------------- --- -------- ------------------------\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing)
      continue;
    if (!d->connected && deadweight)
      continue;
    if (!d->connected) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

      if (d->original)
	sprintf(classname, "[%2d %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%2d %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
    } else
      strcpy(classname, "   -   ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[d->connected]);

    if (d->character && !d->connected && GET_LEVEL(d->character) < LVL_GOD)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    format = "%3d %-7s %-12s %-14s %-3s %-8s ";

    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED",
	      state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (d->connected) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  extern char circlemud_version[];

  switch (subcmd) {
  case SCMD_PLAYERS:
    page_string(ch->desc, player_cmd, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist_cmd, 0);
    break;
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(circlemud_version, ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  default:
    return;
    break;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct descriptor_data *d;

  if (!*arg) {
    send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE) &&
	    (world[ch->in_room].zone == world[i->in_room].zone)) {
	  sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next)
      if (world[i->in_room].zone == world[ch->in_room].zone && CAN_SEE(ch, i) &&
	  (i->in_room != NOWHERE) && isname(arg, i->player.name)) {
	sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	send_to_char(buf, ch);
	return;
      }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


void print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 0)
    sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "[%5d] %s\n\r",
	    world[obj->in_room].number, world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\n\r",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\n\r",
	    PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\n\r",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\n\r");
    send_to_char(buf, ch);
  }
}

int difficulty_lev(struct char_data * ch, struct char_data * victim)
{ 
  int gain_int = 0;

  gain_int += GET_LEVEL(victim)/2;
  if (GET_WIS(ch) > 15)
    gain_int += 2;
  else if(GET_WIS(ch) > 10)
    gain_int += 1;
  if (GET_AC(victim) < -50)
    gain_int += 2;
  else if (GET_AC(victim) < 0)
    gain_int += 1;
  if (IS_NPC(victim))
    gain_int += ((victim->mob_specials.damnodice *
      victim->mob_specials.damsizedice) * (victim->mob_specials.damnodice *
      1))/2;

  if (gain_int > ((7*GET_LEVEL(victim))+7))
    gain_int = (7*GET_LEVEL(victim))+7;

  return gain_int;
}

void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char("Players\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
		    GET_NAME(i), world[d->character->in_room].number,
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
		    world[i->in_room].number, world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
		world[i->in_room].number, world[i->in_room].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }

  diff = difficulty_lev(ch, victim) - difficulty_lev(victim, ch);

  sprintf(buf, "[ %d ] ", diff); 
  send_to_char(buf,ch);
  if (diff <= -50)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -24)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -10)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -5)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 15)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 30)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 45)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 75)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 150)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 500)
    send_to_char("You ARE mad!\r\n", ch);

}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


static char *ctypes[] = {
"off", "sparse", "normal", "complete", "\n"};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}


ACMD(do_toggle)
{
  if (IS_NPC(ch))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

  sprintf(buf,
"&bScrolling related -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\r\n"
"&rBrief                  &gCompact                 &yNorepeat\r\n"
"&rBrief mode: %3s        &gCompact mode: %3s       &yCommunication Repeat: %3s\r\n" 
"&bProtections -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&n\r\n"
"&rNosummon               &gWimp                    &yAFK\r\n"
"&rNo Summon: %3s         &gWimp Level: %3s         &yAway from Keyboard: %3s\r\n" 
"&bChannels -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-&n\r\n"
"&rQuest                  &gnogossip                  &ynoauction\r\n"
"&rOn Quest: %3s          &gGossip Channel: %3s       &yAuction Channel: %3s\r\n" 
"&mnograts                &cnomusic                   &rnoouch\r\n"
"&mCongrats Channel: %3s  &cMusic Channel: %3s        &rOuch Channel: %3s\r\n" 
"&rnotell                 &gnoshout\r\n"
"&rDeaf to tells: %3s     &gDeaf to shouts: %3s\r\n"
"&bAuto Preferences -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-&n\r\n"
"&rautoloot               &gautogold                  &yautosplit\r\n"
"&rAuto Looting: %3s      &gAuto take gold: %3s       &yAuto Splitting: %3s\r\n" 
"&mautoexit               &cautoassist\r\n"
"&mAuto Show Exit: %3s    &cAuto Assist: %3s&n\r\n", 
ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)), YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)), buf2, 
ONOFF(PRF_FLAGGED(ch, PRF_AFK)), YESNO(PRF_FLAGGED(ch, PRF_QUEST)),
ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)), ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)), ONOFF(!PRF_FLAGGED(ch, PRF_NOMUSIC)),
ONOFF(!PRF_FLAGGED(ch, PRF_NOOUCH)), ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
ONOFF(PRF_FLAGGED(ch, PRF_DEAF)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST))); 
  send_to_char(buf, ch);
}


struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  ACMD(do_action);

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
      sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
      if (!(no % 7))
	strcat(buf, "\r\n");
      no++;
    }
  }

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

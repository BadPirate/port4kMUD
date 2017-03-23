/*************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct cha_app_type cha_app[];
extern struct int_app_type int_app[];
extern struct index_data *obj_index;

extern struct weather_data weather_info;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern int pk_allowed;
extern int summon_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int roomaffect_allowed;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

void clearMemory(struct char_data * ch);
void act(char *str, int i, struct char_data * c, struct obj_data * o,
	      void *vict_obj, int j);

void damage(struct char_data * ch, struct char_data * victim,
	         int damage, int weapontype);

void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type);


/*
 * Special spells appear below.
 */

ASPELL(spell_mana)
{
  GET_MANA(ch) += level;
  if (GET_MANA(ch) > GET_MAX_MANA(ch))
    GET_MANA(ch) = GET_MAX_MANA(ch);
  send_to_char("You flow with the dao\r\n",ch);
}

ASPELL(spell_create_water)
{
  int water;

  void name_to_drinkcon(struct obj_data * obj, int type);
  void name_from_drinkcon(struct obj_data * obj);

  if (ch == NULL || obj == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	weight_change_object(obj, water);
	name_from_drinkcon(obj);
	name_to_drinkcon(obj, LIQ_WATER);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}


ASPELL(spell_recall)
{
  int to_room,x,y;
  extern struct townstart_struct townstart[];

  if (victim == NULL || IS_NPC(victim))
    return;

  to_room = real_room(townstart[0].loadroom);
  for(x=0;townstart[x].minlevel != -1;x++);
  y = number(0, (x-1));
  if (GET_LEVEL(ch) >= townstart[y].minlevel)
    to_room = real_room(townstart[y].loadroom);

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}


ASPELL(spell_teleport)
{
  int to_room;
  extern int top_of_world;

  if (victim != NULL)
    return;

  do {
    to_room = number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH));

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_arcane_portal)
{
  extern int max_portal_time;
  int x, y=0, m=10;
  sh_int roomto;
  struct obj_data *k, *portal;

  if (ch == NULL || ch->in_room == NOWHERE)
    return;

  if(!*arg)
  {
    send_to_char("Usage: cast 'arcane portal' <room name>\r\n",ch);
    return;
  }
  if(strlen(arg) < 3)
  {
    send_to_char("Too short, use arcane word to get room name\r\n",ch);
    return;
  }
  for(m=10, x=1; x < (strlen(arg)-2); x++, m*=10);
  for(x=1; x < (strlen(arg)); x++, m /= 10)
  {
    switch(tolower(arg[x]))
    {
      case 'a': y += 1*m; break;
      case 'e': y += 2*m; break;
      case 'i': y += 3*m; break;
      case 'o': y += 4*m; break;
      case 's': y += 5*m; break;
      case 't': y += 6*m; break;
      case 'm': y += 7*m; break;
      case 'p': y += 8*m; break;
      case 'l': y += 9*m; break;
      case 'n': y += 0; break;
      default:
        send_to_char("You spake a bad word, BLEH to thee! Use arcane word!\r\n",ch);
        return; break;
    }
  }
  y = y/24;

  if(real_room(y) == NOWHERE)
  {
    send_to_char("The portal opens, and fades.  The destination was wrong.\r\n",ch);
    return;
  }
  roomto = real_room(y);
  if(ROOM_FLAGGED(roomto, ROOM_NOTELEPORT) ||
     ROOM_FLAGGED(roomto, ROOM_DEATH) ||
     ROOM_FLAGGED(roomto, ROOM_HOUSE))
  {
    send_to_char("Some hostile magic prevents you opening a portal there.\r\n",ch);
    return;
  }  
  for(k = object_list; k; k = k->next)
    if((GET_OBJ_TYPE(k) == ITEM_PORTAL && GET_OBJ_VAL(k, 1) == GET_IDNUM(ch)) ||
       (k->in_room == ch->in_room))
    {
      send_to_room("A portal vanishes from the room.\r\n", k->in_room);
      extract_obj(k);
    }
  portal = create_obj();
  portal->item_number = NOTHING;
  portal->in_room = NOWHERE;
  portal->name = strdup("magic portal");
  sprintf(buf, "A portal created by %s to %s (&benter portal&g)", GET_NAME(ch),
          world[real_room(y)].name);
  portal->description = strdup(buf);
  portal->short_description = strdup("the magic portal");
  GET_OBJ_TYPE(portal) = ITEM_PORTAL;
  GET_OBJ_WEAR(portal) = ITEM_WEAR_HEAD;
  GET_OBJ_VAL(portal, 0) = y;
  GET_OBJ_VAL(portal, 1) = GET_IDNUM(ch);
  GET_OBJ_WEIGHT(portal) = 1;
  GET_OBJ_RENT(portal) = 0;
  GET_OBJ_TIMER(portal) = max_portal_time;
  obj_to_room(portal, ch->in_room);
  send_to_room("A glowing magical portal forms from space, wonder where it goes?\r\n",
               ch->in_room);    
  send_to_char("You create a portal!\r\n",ch);
}

ASPELL(spell_arcane_word)
{
  char converted[20], roomname[MAX_STRING_LENGTH] = "The word for this room is ";
  int x=0;

  if (ch == NULL || ch->in_room == NOWHERE)
    return;
  sprintf(converted, "%d", ((world[ch->in_room].number)*24));
  while(x < strlen(converted))
  {
    switch (converted[x])
    {
      case '1': strcat(roomname, "a"); break;
      case '2': strcat(roomname, "e"); break;
      case '3': strcat(roomname, "i"); break;
      case '4': strcat(roomname, "o"); break;
      case '5': strcat(roomname, "s"); break;
      case '6': strcat(roomname, "t"); break;
      case '7': strcat(roomname, "m"); break;
      case '8': strcat(roomname, "p"); break;
      case '9': strcat(roomname, "l"); break;
      case '0': strcat(roomname, "n"); break;
      default: sprintf(roomname, "%s(%d)",roomname, converted[x]); break;
    }
    x++;
  }
  strcat(roomname,".\r\n");
  send_to_char(roomname,ch);
}

ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  /* (FIDO) Changed from pk_allowed to summon_allowed */
  if (!summon_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[ch->in_room].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      sprintf(buf, "%s failed summoning %s to %s.",
	      GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, BRF, LVL_IMMORT, TRUE);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOCHARM) ||
      (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL))) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, ch->in_room);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH], output[MAX_STRING_LENGTH];
  int j, overflow =0;

  strcpy(output, "Found:\r\n");
  strcpy(name, fname(obj->name));
  j = level >> 1;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;
    
    if ( (i->carried_by) && (!(IS_OBJ_STAT(i, ITEM_ANTI_LOCATE)) ||
       (GET_LEVEL(ch) > LVL_IMMORT))) 
      sprintf(buf, "%s is being carried by %s.\n\r",
	      i->short_description, PERS(i->carried_by, ch));
    else if ((i->in_room != NOWHERE) && (!(IS_OBJ_STAT(i, ITEM_ANTI_LOCATE))
      || (GET_LEVEL(ch) > LVL_IMMORT))) 
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
	      world[i->in_room].name);
    else if ((i->in_obj) && (!(IS_OBJ_STAT(i, ITEM_ANTI_LOCATE))
      || (GET_LEVEL(ch) > LVL_IMMORT)))
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
	      i->in_obj->short_description);
    else if ((i->worn_by) && (!(IS_OBJ_STAT(i, ITEM_ANTI_LOCATE))
      || (GET_LEVEL(ch) > LVL_IMMORT))) 
      sprintf(buf, "%s is being worn by %s.\n\r",
	      i->short_description, PERS(i->worn_by, ch));
    else
      sprintf(buf, "%s's location is uncertain.\n\r",
	      i->short_description);
    
    if((strlen(buf) + strlen(output) + 12) < MAX_STRING_LENGTH)
      strcat(output, buf);      
    else
      overflow = 1;
    j--;
  }

  if(overflow == 1)
    strcat(output, "\r\nOVERFLOW\r\n");
  page_string(ch->desc, output, 1);
  if (j == level >> 1)
    send_to_char("You sense nothing.\n\r", ch);
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char("You like yourself even better!\r\n", ch);
  else if (!IS_NPC(victim))
    {
      if (charm_allowed == 0)
        if (!PRF_FLAGGED(victim, PRF_SUMMONABLE))
          send_to_char("You fail because SUMMON protection is on!\r\n", ch);
    }
  else if (IS_AFFECTED(victim, AFF_SANCTUARY))
    send_to_char("Your victim is protected by sanctuary!\r\n", ch);
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char("Your victim resists!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_CHARM))
    send_to_char("You can't have any followers of your own!\r\n", ch);
  else if (IS_AFFECTED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char("You fail.\r\n", ch);
  /* player charming another player - no legal reason for this */
  /* (FIDO) Changed from pk_allowed to charm_allowed */
  else if (!charm_allowed && !IS_NPC(victim))
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
  else if (circle_follow(victim, ch))
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
  else if (mag_savingthrow(victim, SAVING_PARA))
    send_to_char("Your victim resists!\r\n", ch);
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;

    if (GET_INT(victim))
      af.duration = 24 * 18 / GET_INT(victim);
    else
      af.duration = 24 * 18;

    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)) {
      REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
    }
  }
}



ASPELL(spell_identify)
{
  int i;
  int found;

  struct time_info_data age(struct char_data * ch);

  extern char *spells[];

  extern char *item_types[];
  extern char *extra_bits[];
  extern char *apply_types[];
  extern char *affected_bits[];

  if (obj) {
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
	    GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

      if (GET_OBJ_VAL(obj, 1) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 1)]);
      if (GET_OBJ_VAL(obj, 2) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 2)]);
      if (GET_OBJ_VAL(obj, 3) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%s\r\n", buf);
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf, "%s %s\r\n", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
	      GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
	      GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
	      GET_OBJ_VAL(obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
	      (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char("Can affect you as :\r\n", ch);
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, buf2);
	sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
	send_to_char(buf, ch);
      }
    }
  } else if (victim) {		/* victim */
    sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
	      GET_NAME(victim), age(victim).year, age(victim).month,
	      age(victim).day, age(victim).hours);
      send_to_char(buf, ch);
    }
    sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
	    GET_HEIGHT(victim), GET_WEIGHT(victim));
    sprintf(buf, "%sLevel: %d, Hits: %d, Mana: %d\r\n", buf,
	    GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    sprintf(buf, "%sAC: %d, Hitroll: %d, Damroll: %d\r\n", buf,
	    GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    sprintf(buf, "%sStr: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
	    buf, GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
    send_to_char(buf, ch);

  }
}



ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
      !IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
	return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1 + (level >= 18);

    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 1 + (level >= 20);

    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (IS_AFFECTED(victim, AFF_POISON))
        send_to_char("You can sense poison in your blood.\r\n", ch);
      else
        send_to_char("You feel healthy.\r\n", ch);
    } else {
      if (IS_AFFECTED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char("You sense that it should not be consumed.\r\n", ch);
    }
  }
}

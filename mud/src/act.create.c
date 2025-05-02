/* ************************************************************************
*   File: act.create.c					Part of CircleMUD *
*  Usage: Player-level object creation stuff				  *
*									  *
*  All rights reserved.	 See license.doc for complete information.	  *
*									  *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		  *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* struct for syls */
struct syllable {
  char *org;
  char *new;
};

/* extern variables */
extern char *spells[];
extern struct spell_info_type spell_info[];
extern struct syllable syls[];

/* extern procedures */
int mag_manacost(struct char_data * ch, int spellnum);


char *get_spell_name(char *argument)
{
  char *s;

  s = strtok(argument, "'");
  s = strtok(NULL, "'");
  
  return s;
}

void make_potion(struct char_data *ch, int potion, struct obj_data *container) {
	struct obj_data *final_potion;
	struct extra_descr_data *new_descr;
	int mana, dam;
    
	if ((number(1, 3) == 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
	  send_to_char("As you begin mixing the potion, it violently"
	 		     " explodes!\r\n",ch);
	  act("$n begins to mix a potion, but it suddenly explodes!",
		    FALSE, ch, 0,0, TO_ROOM);
	  extract_obj(container);
	  dam = number(15, mag_manacost(ch, potion) * 2);
	  GET_HIT(ch) -= dam;
	  update_pos(ch);
	  return;
	}

	mana = mag_manacost(ch, potion) * 9;
	if (GET_MANA(ch) - mana > 0) {
	    if (GET_LEVEL(ch) < LVL_IMMORT) GET_MANA(ch) -= mana;
		sprintf(buf, "You create a %s potion.\r\n",
		    spells[potion]);
		send_to_char(buf, ch);
		act("$n creates a potion!", FALSE, ch, 0, 0, TO_ROOM);
		extract_obj(container);
	}
	else {
		send_to_char("You don't have enough mana to mix"
			     " that potion!\r\n", ch);
		return;
	}
	
	final_potion = create_obj();

	final_potion->item_number = NOTHING;
	final_potion->in_room = NOWHERE;
	sprintf(buf2, "%s potion", spells[potion]);
	final_potion->name = str_dup(buf2);

	sprintf(buf2, "A %s potion lies here.",spells[potion]);
	final_potion->description = str_dup(buf2);

	sprintf(buf2, "a %s potion", spells[potion]);
	final_potion->short_description = str_dup(buf2);

	/* extra description coolness! */
	CREATE(new_descr, struct extra_descr_data, 1);
	new_descr->keyword = str_dup(final_potion->name);
	sprintf(buf2, "It appears to be a %s potion.", spells[potion]);
	new_descr->description = str_dup(buf2);
	new_descr->next = NULL;
	final_potion->ex_description = new_descr;
 
	GET_OBJ_TYPE(final_potion) = ITEM_POTION;
	GET_OBJ_WEAR(final_potion) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(final_potion) = ITEM_NORENT;
	GET_OBJ_VAL(final_potion, 0) = GET_LEVEL(ch);
	GET_OBJ_VAL(final_potion, 1) = potion;
	GET_OBJ_VAL(final_potion, 2) = potion;
	GET_OBJ_VAL(final_potion, 3) = potion;
	GET_OBJ_COST(final_potion) = GET_LEVEL(ch) * 15000 / 101;
	GET_OBJ_WEIGHT(final_potion) = 1;
	GET_OBJ_RENT(final_potion) = 0;
		
	obj_to_char(final_potion, ch);
}

ACMD(do_brew)
{
	struct obj_data *container = NULL;
	struct obj_data *obj, *next_obj;
	char bottle_name[MAX_STRING_LENGTH];
	char spell_name[MAX_STRING_LENGTH];
	char *temp1, *temp2;
	int potion, found = FALSE;

	temp1 = one_argument(argument, bottle_name);
	
	/* sanity check */
	if (temp1) {
	    temp2 = get_spell_name(temp1);
	    if (temp2)
		strcpy(spell_name, temp2);
	} else {
	    bottle_name[0] = '\0';
	    spell_name[0] = '\0';
	} 
	
     
	if (!(GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC
	    || GET_LEVEL(ch) >= LVL_IMMORT || GET_CLASS(ch) == CLASS_VAMPYRE
            || GET_CLASS(ch) == CLASS_PALADIN 
            || GET_CLASS(ch) == CLASS_NINJA || GET_CLASS(ch) == CLASS_MASTER)) {
		send_to_char("You have no idea how to mix potions!\r\n", ch);
		return;
	}
	if (!*bottle_name || !*spell_name) {
		send_to_char("Usage: brew <potion> '<spell>'\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(container = get_obj_in_list_vis(ch, bottle_name,
			ch->carrying))) 
			continue;
		else
			found = TRUE;
	}
	if (found != FALSE && (GET_OBJ_TYPE(container) != ITEM_DRINKCON)) {
		send_to_char("That item is not a drink container!\r\n", ch);
		return;
	}
	if (found == FALSE) {
		sprintf(buf, "You don't have %s in your inventory!\r\n",
			bottle_name);
		send_to_char(buf, ch);
		return;
	}

	if (!spell_name || !*spell_name) {
	    send_to_char("Spell names must be enclosed in single quotes!\r\n",
			 ch);
	    return;
	}
 
	potion = find_skill_num(spell_name);	

	if ((potion < 1) || (potion > MAX_SPELLS)) {
		send_to_char("Mix what spell?!?\r\n", ch);
		return;
	}
	if (GET_LEVEL(ch) < spell_info[potion].min_level[(int) GET_CLASS(ch)]) {
		 send_to_char("You do not know how to make that potion!\r\n",
			      ch);
		return;
	}
	if (GET_SKILL(ch, potion) == 0) {
		 send_to_char("You are unfamiliar with potion brewing.\r\n",
		 ch);
		return;
	}
	make_potion(ch, potion, container);
}


char *garble_spell(int spellnum)
{
  char lbuf[256];
  int j, ofs = 0;

  *buf = '\0';
  strcpy(lbuf, spells[spellnum]);

  while (*(lbuf + ofs)) {
    for (j = 0; *(syls[j].org); j++) {
      if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org))) {
	strcat(buf, syls[j].new);
	ofs += strlen(syls[j].org);
      }
    }
  }
  return buf;
}

void make_scroll(struct char_data *ch, int scroll, struct obj_data *paper)
{
	struct obj_data *final_scroll;
	struct extra_descr_data *new_descr;
	int can_make = TRUE, mana, dam = 0;
	
	/* add a case statement here for prohibited spells */
 
	if (can_make == FALSE) {
		send_to_char("That spell cannot be scribed into a"
			     " scroll.\r\n", ch);
		return;
	}
	else if ((number(1, 3) == 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
		send_to_char("As you begin inscribing the final rune, the"
			     " scroll violently explodes!\r\n",ch);
		act("$n tries to scribe a spell, but it explodes!",
		    FALSE, ch, 0,0, TO_ROOM);
		extract_obj(paper);
		dam = number(15, mag_manacost(ch, scroll) * 2);
		GET_HIT(ch) -= dam;
		update_pos(ch);
		return;
	}
	/* requires x3 mana to scribe a scroll than the spell */
	mana = mag_manacost(ch, scroll) * 3;

	if (GET_MANA(ch) - mana > 0) {
		if (GET_LEVEL(ch) < LVL_IMMORT) GET_MANA(ch) -= mana;
		sprintf(buf, "You create a scroll of %s.\r\n",
			spells[scroll]);
		send_to_char(buf, ch);
		act("$n creates a scroll!", FALSE, ch, 0, 0, TO_ROOM);
		extract_obj(paper);
	}
	else {
		send_to_char("You don't have enough mana to scribe such"
			     " a powerful spell!\r\n", ch);
		return;
	}
	
	final_scroll = create_obj();

	final_scroll->item_number = NOTHING;
	final_scroll->in_room = NOWHERE;
	sprintf(buf2, "%s %s scroll", 
		spells[scroll], garble_spell(scroll));
	final_scroll->name = str_dup(buf2);

	sprintf(buf2, "Some parchment inscribed with the runes '%s' lies here.",
		garble_spell(scroll));
	final_scroll->description = str_dup(buf2);

	sprintf(buf2, "a %s scroll", garble_spell(scroll));
	final_scroll->short_description = str_dup(buf2);

	/* extra description coolness! */
	CREATE(new_descr, struct extra_descr_data, 1);
	new_descr->keyword = str_dup(final_scroll->name);
	sprintf(buf2, "It appears to be a %s scroll.", spells[scroll]);
	new_descr->description = str_dup(buf2);
	new_descr->next = NULL;
	final_scroll->ex_description = new_descr;
 
	GET_OBJ_TYPE(final_scroll) = ITEM_SCROLL;
	GET_OBJ_WEAR(final_scroll) = ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(final_scroll) = ITEM_NORENT;
	GET_OBJ_VAL(final_scroll, 0) = GET_LEVEL(ch);
	GET_OBJ_VAL(final_scroll, 1) = scroll;
	GET_OBJ_VAL(final_scroll, 2) = -1;
	GET_OBJ_VAL(final_scroll, 3) = -1;
	GET_OBJ_COST(final_scroll) = GET_LEVEL(ch) * 500;
	GET_OBJ_WEIGHT(final_scroll) = 1;
	GET_OBJ_RENT(final_scroll) = 0;
	
	obj_to_char(final_scroll, ch);
}


ACMD(do_scribe)
{
	struct obj_data *paper = NULL;
	struct obj_data *obj, *next_obj;
	char paper_name[MAX_STRING_LENGTH];
	char spell_name[MAX_STRING_LENGTH];
	char *temp1, *temp2;
	int scroll = 0, found = FALSE;

	temp1 = one_argument(argument, paper_name);

	/* sanity check */
	if (temp1) {
	    temp2 = get_spell_name(temp1);
	    if (temp2)
		strcpy(spell_name, temp2);
	} else {
	    paper_name[0] = '\0';
	    spell_name[0] = '\0';
	}


	if (!(GET_CLASS(ch) == CLASS_BARD || GET_LEVEL(ch) >= LVL_IMMORT)) 
        {
		send_to_char("This skill is reserved for bards\r\n", ch);
		return;
	}
	if (!*paper_name || !*spell_name) {
		send_to_char("Usage: scribe <note> '<spell>'\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(paper = get_obj_in_list_vis(ch, paper_name,
			ch->carrying))) 
			continue;
		else
			found = TRUE;
	}
	if (found && (GET_OBJ_TYPE(paper) != ITEM_NOTE)) {
		send_to_char("You can't write on that!\r\n", ch);
		return;
	}
	if (found == FALSE) {
		sprintf(buf, "You don't have %s in your inventory!\r\n",
			paper_name);
		send_to_char(buf, ch);
		return;
	}

	if (!spell_name || !*spell_name) {
	    send_to_char("Spell names must be enclosed in single quotes!\r\n",
			 ch);
	    return;
	} 

	scroll = find_skill_num(spell_name);	

	if ((scroll < 1) || (scroll > MAX_SPELLS)) {
		send_to_char("Scribe what spell?!?\r\n", ch);
		return;
	}
	if (GET_LEVEL(ch) < spell_info[scroll].min_level[(int) GET_CLASS(ch)]) {
		 send_to_char("You are not schooled enough to cast that spell!\r\n",
			      ch);
		return;
	}
	if (GET_SKILL(ch, scroll) == 0) {
		 send_to_char("You don't know any spell like that!\r\n",
		 ch);
		return;
	}
	make_scroll(ch, scroll, paper);
}


ACMD(do_forge)
{
	/* PLEASE NOTE!!!  This command alters the object_values of the target
	   weapon, and this will save to the rent files.  It should not cause
	   a problem with stock Circle, but if your weapons use the first 
	   position [ GET_OBJ_VAL(weapon, 0); ], then you WILL have a problem.
	   This command stores the character's level in the first value to 
	   prevent the weapon from being "forged" more than once by mortals.
	   Install at your own risk.  You have been warned...
	*/
	
	struct obj_data *weapon = NULL;
	struct obj_data *obj, *next_obj;
	char weapon_name[MAX_STRING_LENGTH];
	int found = FALSE, prob = 0, dam = 0;

	one_argument(argument, weapon_name);

	if (!(GET_CLASS(ch) == CLASS_WARRIOR || GET_CLASS(ch) == CLASS_THIEF 
	    || GET_LEVEL(ch) >= LVL_IMMORT)) {
		send_to_char("You have no idea how to forge weapons!\r\n", ch);
		return;
	}
	if (!*weapon_name) {
		send_to_char("What do you wish to forge?\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(weapon = get_obj_in_list_vis(ch, weapon_name,
			ch->carrying))) 
			continue;
		else
			found = TRUE;
	}
	
	if (found == FALSE) {
		sprintf(buf, "You don't have %s in your inventory!\r\n",
			weapon_name);
		send_to_char(buf, ch);
		return;
	}

	if (found && (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)) {
		sprintf(buf, "It doesn't look like %s would make a"
			" good weapon...\r\n", weapon_name);
		send_to_char(buf, ch);
		return;
	}

	if ((GET_OBJ_VAL(weapon, 0) > 0) && (GET_LEVEL(ch) < LVL_IMMORT)) {
	    send_to_char("You cannot forge a weapon more than once!\r\n", ch);
	    return;
	}

	if (GET_OBJ_EXTRA(weapon) & ITEM_MAGIC) {
	    send_to_char("The weapon is imbued with magical powers beyond"
			 "your grasp.\r\nYou cannot further affect its form.\r\n", ch);
	    return;
	}

	/* determine success probability */
	prob += (GET_LEVEL(ch) << 1) + ((GET_DEX(ch) - 11) << 1);
	prob += ((GET_STR(ch) - 11) << 1) + (GET_ADD(ch) >> 3);

	if ((number(10, 100) > prob) && (GET_LEVEL(ch) < LVL_IMMORT)) {
		send_to_char("As you pound out the dents in the weapon,"
			     " you hit a weak spot and it explodes!\r\n", ch);
		act("$n tries to forge a weapon, but it explodes!",
		    FALSE, ch, 0,0, TO_ROOM);
		extract_obj(weapon);
		dam = number(20, 60);
		GET_HIT(ch) -= dam;
		update_pos(ch);
		return;
	}

	GET_OBJ_VAL(weapon, 0) = GET_LEVEL(ch);
	GET_OBJ_VAL(weapon, 1) += (GET_LEVEL(ch) % 3) + number(-1, 2); 
	GET_OBJ_VAL(weapon, 2) += (GET_LEVEL(ch) % 2) + number(-1, 2);
	GET_OBJ_RENT(weapon) += (GET_LEVEL(ch) << 3);

	send_to_char("You have forged new life into the weapon!\r\n", ch);
	act("$n vigorously pounds on a weapon!",
		    FALSE, ch, 0, 0, TO_ROOM);
}


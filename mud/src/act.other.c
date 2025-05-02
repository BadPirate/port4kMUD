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
#include "screen.h"
#include "house.h"

#define IS_GOD(ch)               (!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern char *class_abbrevs[];

/* extern procedures */
SPECIAL(shop_keeper);
void write_aliases(struct char_data *ch);

/* Towns Subroutine */
const struct townstart_struct townstart[] =
{
//abbrv	   Town Name	start	corpse	hell	minlv	x	donate
{ "&rD&n", "Daggerfall",3001,  	11,	101, 	0,   	"d",	3063 },
{ "&gE&n", "Elven City",19005,  19042,	19042, 	5,   	"e",	150  },
{ "&mR&n", "Rome",	145,  	12069,	12069, 	15,   	"r",	151  },
{ "&bM&n", "Mist",	1720,  	1733,	1733, 	25,   	"m",	1732 },
{ "&uI&n", "Immortals", 1204,  	1209,	1209,	101, 	"i",	1201 },
{ "USED", "RESERVED",   -1,    -1,    -1,    -1,  } // END STRUCT
};

ACMD(do_remort)
{
  int i, class, cb=0, total;
  int parse_class(char);
  extern int do_start(struct char_data *);
  extern char *class_menu;
  one_argument(argument, arg);

  if(IS_NPC(ch))
  {
    send_to_char("Mobs don't remort, go away",ch);
    return;
  }
  cb = ch->player_specials->saved.classes_been;
  if(GET_LEVEL(ch) < 101)
  {
    send_to_char("Your not high enough to remort.\r\n",ch);
    return;
  }
  for(i=0, total=1; i < NUM_CLASSES; i++)
    if(IS_SET(cb, (1 << i)))
      total++;
  if(*arg)
  {
    if(strcmp(arg, "master") == 0)
    {
      if (total >= 4)
      { 
        class = CLASS_MASTER;
      }
      else
      {
        send_to_char("You have to be four other classes before you can master\r\n",ch);
        return;
      }
    }
    else
    { 

      class = parse_class(arg[0]);
      if (class == CLASS_UNDEFINED || (class == CLASS_MASTER && total < 4))
      {
        send_to_char("Thats not an option\r\n",ch);
        send_to_char(class_menu,ch);
        return;
      }
    }
    if (IS_SET(cb, (1 << class)))
    {
      send_to_char("You've been that class!\r\n",ch);
      return;
    }
    if (GET_CLASS(ch) == class)
    {
      send_to_char("You are that class!\r\n",ch);
      return;
    }
    if (GET_CLASS(ch) == CLASS_MASTER)
    {
      send_to_char("You are already a master!\r\n",ch);
      return;
    }
    if (GET_LEVEL(ch) >= LVL_IMMORT)
    {
      send_to_char("You really don't want to do that :-)\r\n",ch);
      return;
    }
    if (class != CLASS_MASTER && total >= 4)
    {
      send_to_char("You already have your 4 remorts, you can go master now!\r\n",ch);
      class = CLASS_MASTER;
    }
    if (GET_CLASS(ch) == CLASS_NINJA && total == 1)
    {
      send_to_char("Your playing old rules, go on to master!\r\n",ch);
      class = CLASS_MASTER;
    }
    SET_BIT(ch->player_specials->saved.classes_been, (1 << GET_CLASS(ch)));
    GET_CLASS(ch) = class;  

    for (i = 0; i < NUM_WEARS; i++)
      if (ch->equipment[i])
        obj_to_char(unequip_char(ch, i), ch);
 
    GET_MAX_HIT(ch) = 1;
    GET_MAX_MOVE(ch) = 60;
    GET_MAX_MANA(ch) = 100;
    GET_LEVEL(ch) = 0;
    do_start(ch);
    sprintf(buf, "Succesfully Remorted #%d.  You're a newbie again and your old items are in your\r\ninventory! (WARNING: your invisible items are in your inventory too.)\r\n",(total));
    send_to_char(buf,ch);
  }
  else
  {
    send_to_char("Usage:  Remort <class letter>\r\n",ch);
    if(total >= 4)
      send_to_char("        Remort master\r\n",ch);
    send_to_char(class_menu,ch);
    return;
  }
}

ACMD(do_quote)
{
  FILE *quotefile;
  int randnum, counter, x;
  char line[90];

  strcpy(buf, "*");
  quotefile = fopen("../lib/quotes","r");
  randnum = number(0,340);

  for(counter=0; counter < randnum; counter++)
    do fgets(line, 80, quotefile); while ( line[0] != '-' );

  for(x=0; line[x]; x++)
    if (line[x] == '\r' || line[x] == '\n')
      line[x] = ' ';
  
  do {
    fgets(line, 80, quotefile);
    sprintf(buf,"%s%s\r", buf,line);
  } while ( line[0] != '-' );
  fclose(quotefile);
  strcat("\r\n\0", buf);  
  page_string(ch->desc, buf, 1);  
}

int find_rnum_object(char *obj)
{
  int nr=-1;
  extern int top_of_objt;
  extern struct obj_data *obj_proto;
  for (nr=0;!isname(obj,obj_proto[nr].name)&&nr<=top_of_objt;nr++) {
  }
  return(nr>top_of_objt?-1:nr);
}

ACMD(do_howl)
{
  int levelchance(struct char_data *);
  ACMD(do_flee);
  struct char_data *tch, *tch_next;

  send_to_char("You howl at the heavens.\n",ch);
  act("$n howls at the heavens.", TRUE, ch, 0, FALSE, TO_ROOM);
  if(IS_WOLF(ch)) {
    for (tch = world[ch->in_room].people; tch; tch = tch_next) {
      tch_next=tch->next_in_room;
      if ((tch!=ch) && IS_NPC(tch) && !MOB_FLAGGED(tch, MOB_SENTINEL))
        if ( levelchance(ch) )
          do_flee(tch,"",0,0);
    }
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }
}

ACMD(do_statue)
{
  int levelchance(struct char_data *);
  
  send_to_char("You try to stay still as a statue.\n",ch);
  act("$n tries to pose as a statue.", TRUE, ch, 0, FALSE, TO_ROOM);
  if (IS_GARGOYLE(ch)) {
    if(levelchance(ch)) {
      send_to_char("You feel yourself turn into stone!\n",ch);
      act("$n solidifies into solid stone!", TRUE, ch, 0, FALSE, TO_ROOM);
      SET_BIT(AFF_FLAGS(ch), AFF_STATUE);
      WAIT_STATE(ch, PULSE_VIOLENCE);
    }
  }
}

ACMD(do_food)
{
/* blc - elven food command */
  struct obj_data *obj;
  int onu;
  
  if(IS_ELF(ch)) {
    if(GET_MOVE(ch) >= 30) {
      onu=find_rnum_object("waybread");
      if(onu>=0) {
	obj=read_object(onu, REAL);
	obj_to_char(obj,ch);
	act("$n dances a little jig.", TRUE, ch, 0, FALSE, TO_ROOM);
	act("A waybread falls into $n's hands!", TRUE, ch, 0, FALSE, TO_ROOM);
	GET_MOVE(ch)-=30;
	send_to_char("You dance a little jig to please the food God.\r\n",ch);
	send_to_char("A waybread suddenly appears in your hands!\r\n",ch);
        WAIT_STATE(ch, PULSE_VIOLENCE);
      } else {
	send_to_char("An error occurred.  Waybread not found?!?!!.\r\n",ch);
      }
    } else {
      send_to_char("You're too tired to do the elven food dance.\r\n",ch);
    }
  } else {
    send_to_char("You do a little silly dance for the food God.\r\n",ch);
    act("$n dances a little jig but it makes a fool out of $mself.",
	TRUE, ch, 0, FALSE, TO_ROOM);
  }
}

ACMD(do_quit)
{
  void die(struct char_data * ch);
  void Crash_rentsave(struct char_data * ch, int cost);
  extern int free_rent;
  sh_int save_room;
  struct descriptor_data *d, *next_d;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if ( IS_SET(ROOM_FLAGS(ch->in_room), ROOM_NORECALL) &&
       GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char("The room holds you here.\r\n",ch);
    return;
  }
  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("You have to type quit--no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch);
  } else {
    if (GET_LEVEL(ch) >= LVL_IMMORT)
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    else
    {
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
      sprintf(buf, "%s has quit the game.\r\n", GET_NAME(ch));
      send_to_all(buf);
    }
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        close_socket(d);
    }

   save_room = ch->in_room;
   if (free_rent)
      Crash_rentsave(ch, 0);
    extract_char(ch);		/* Char is saved in extract char */

    /* If someone is quitting in their house, let them load back here */
    if (ROOM_FLAGGED(save_room, ROOM_HOUSE))
      save_char(ch, save_room);
  }
}



ACMD(do_save)
{
  struct char_data *wch;
  struct descriptor_data *d;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (cmd) {
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  for (d = descriptor_list; d; d = d->next) 
  {
    if (d->connected)
      continue;

    if (d->original)
      wch = d->original;
    else if (!(wch = d->character))
      continue;

    write_aliases(wch);
    save_char(wch, NOWHERE);
    Crash_crashsave(wch);
    if (ROOM_FLAGGED(wch->in_room, ROOM_HOUSE_CRASH))
      House_crashsave(world[wch->in_room].number);
  }
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  byte percent;

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_tstart)
{
  int x;
  char foo[MAX_INPUT_LENGTH];
  argument = one_argument(argument, foo);
  
  if (*foo)
  {
    for(x=0; townstart[x].minlevel != -1; x++)
    {
      if(strcmp(foo, townstart[x].entrychar) == 0)
      {
        if(GET_LEVEL(ch) < townstart[x].minlevel)
        {
          sprintf(buf, "You are not high enough level (%d) to live in %s\r\n", townstart[x].minlevel, townstart[x].longname);
          send_to_char(buf, ch);
          return;
        }
        if(PLR_FLAGGED(ch, PLR_LOADROOM) && GET_LEVEL(ch) <= LVL_IMMORT)
        {
          if(GET_GOLD(ch) < 1000000)
          {
            send_to_char("You don't have enough gold\r\n",ch);
            return;
          }
          else
           GET_GOLD(ch) -= 1000000;
        }
        SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);   
        GET_LOADROOM(ch) = townstart[x].loadroom;
        sprintf(buf, "You now start in %s, you recall to go there\r\n",
                townstart[x].longname);
        send_to_char(buf, ch);
        return;
      }
    }
  }
  send_to_char("&rAvailable starting rooms\r\n",ch);
  send_to_char("------------------------&c\r\n",ch);
  send_to_char("Num Name          MinLvl\r\n",ch);
  send_to_char("--- ------------- ------&n\r\n",ch);
  for(x=0; townstart[x].minlevel != -1 ; x++)
  {
    sprintf(buf, "%2s. %13s %6d\r\n", townstart[x].entrychar, 
            townstart[x].longname, townstart[x].minlevel);
    send_to_char(buf, ch);
  }
  send_to_char("Switching costs 1 million gold,\r\n",ch);      
  send_to_char("Unless you haven't switched before.\r\n", ch);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;
  extern int pt_allowed;


  ACMD(do_gen_comm);

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }

  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS */

  if (!pt_allowed && !IS_NPC(vict)) {
    pcsteal = 1;
  }

  if (pcsteal && !IS_GOD(ch)) {
    SET_BIT(PLR_FLAGS(ch), PLR_THIEF);
    sprintf(buf, "PC Thief bit set on %s for stealing from %s at %s.",
            GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_IMMORT, TRUE);
    act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_SLEEPING)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1) {
	  sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
	  send_to_char(buf, ch);
	} else {
	  send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
	}
      } else {
	send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  void list_skills(struct char_data * ch);

  one_argument(argument, arg);

  if (*arg)
    send_to_char("You can only practice skills in your guild.\r\n", ch);
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  void appear(struct char_data * ch);
  void perform_immort_vis(struct char_data *ch);

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if IS_AFFECTED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  int maxlength;
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    maxlength=80-24-strlen(GET_NAME(ch));
  else
    maxlength=80-18-strlen(GET_NAME(ch));
  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strstr(argument, "<") || strstr(argument, ">"))
    send_to_char("Titles can't contain the < or > characters.\r\n", ch);
  else if (strlen(argument) > maxlength)
  {
    sprintf(buf, "Sorry, your title can't be more than %d characters.\r\n",
	    maxlength);
    send_to_char(buf, ch);
  } else {
    set_title(ch, argument);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (IS_AFFECTED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return 0;

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return 1;
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("Your group consists of:\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP)) {
      sprintf(buf, "     [%4d/%4dH %4d/%4dM %3d/%3dV] [%2d %s] $N (Head of group)",
	      GET_HIT(k), GET_MAX_HIT(k), GET_MANA(k), GET_MAX_MANA(k),
	      GET_MOVE(k), GET_MAX_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!IS_AFFECTED(f->follower, AFF_GROUP))
	continue;
      sprintf(buf, "     [%4d/%4dH %4d/%4dM %3d/%3dV] [%2d %s] $N",
	      GET_HIT(f->follower), GET_MAX_HIT(f->follower), 
              GET_MANA(f->follower), GET_MAX_MANA(f->follower),
	      GET_MOVE(f->follower), GET_MAX_MOVE(f->follower), 
              GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}



ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!IS_AFFECTED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;
  void stop_follower(struct char_data * ch);

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (IS_AFFECTED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	send_to_char(buf2, f->follower);
        if (!IS_AFFECTED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!IS_AFFECTED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!IS_AFFECTED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && IS_AFFECTED(ch, AFF_GROUP))
      share = amount / num;
    else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
	      amount, share);
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
		amount, share);
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int equipped = 1, percent;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      equipped = 0;
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf2, ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
      break;
    default:
      log("SYSERR: Unknown subcmd passed to do_use");
      return;
      break;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.", ch);
      return;
    }
    percent=number(1, LVL_IMMORT);
    if (percent<GET_LEVEL(ch) && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char("Your body doesn't seem as affected as it used to.\r\n", ch);
      obj_from_char(mag_item);
      extract_obj(mag_item);
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.", ch);
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  one_argument(argument, arg);
 
  if (subcmd == SCMD_AWIMP)
  {
    send_to_char(
"You will now autowimp, when you get hit, you will flee if the\r\n"
"same amount of damage you were just hit with would kill you\r\n", ch);
    return;
  }
  if (!*arg) {
    if (GET_WIMP_LEV(ch) == -1)
    {
      send_to_char(
"You are currently autowimping, when you get hit, you will flee if the\r\n"
"same amount of damage you were just hit with would kill you\r\n", ch);
      return;
    } else if (GET_WIMP_LEV(ch) > 0) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev == -1)
      {
        send_to_char(
"You will now run away if you take a hit that if repeated would kill you\r\n"
"&b(autowimp)&n\r\n",ch);
        GET_WIMP_LEV(ch) = -1;
        return;
      } else if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
	send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

  return;

}


ACMD(do_display) {
   char arg[MAX_INPUT_LENGTH];
   int i, x;

   const char *def_prompts[][2] = {
     { "Normal"                 , "&n%hhp %mmp %vmv> " 	},
     { "Off"                    , "&n> " 		},
     { "Dymus", "&n%h/%HH %m/%MM %v/%VV> %t %px - %x/%Xexp "},
     { "Reju","[ &g%ph HP&n ] [ &r%pm mana&n ] [ &m%pv move&n ] [ &c%px exp&n ] [ &y%g gold&n ]%_(%o opp)(%t tank): "},
     { "Algernon","<&c%h&n/&c%H|&r%m&n/&r%M&n|&b%v&n/&b%V&n> <&rTANK: %t&n> "},
     { "\n"                       , "\n"                }
   };

   one_argument(argument, arg);

   if (!arg || !*arg) {
     send_to_char("The following pre-set prompts are availible...\r\n", ch);
     for (i = 0; *def_prompts[i][0] != '\n'; i++) {
       sprintf(buf, "  %d. %-25s \r\n", i, def_prompts[i][0]);
       send_to_char(buf, ch);
     }
     send_to_char("Usage: display <number>\r\n"
                  "To create your own prompt, use _prompt <str>_.\r\n", ch);
     return;
   } else if (!isdigit(*arg)) {
     send_to_char("Usage: display <number>\r\n", ch);
     send_to_char("Type _display_ without arguments for a list of preset prompts.\r\n", ch);
      return;
    }

   i = atoi(arg);

   if (i < 0) {
     send_to_char("The number cannot be negative.\r\n", ch);
      return;
    }

   for (x = 0; *def_prompts[x][0] != '\n'; x++);

   if (i >= x) {
     sprintf(buf, "The range for the prompt number is 0-%d.\r\n", x);
     send_to_char(buf, ch);
     return;
    }

   if (GET_PROMPT(ch))
     free(GET_PROMPT(ch));
   GET_PROMPT(ch) = str_dup(def_prompts[i][1]);

   sprintf(buf, "Set your prompt to the %s preset prompt.\r\n", def_prompts[i][0]);
   send_to_char(buf, ch);
}

ACMD(do_prompt) {
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "Your prompt is currently: %s\r\n", (GET_PROMPT(ch) ? GET_PROMPT(ch) : "n/a"));
    send_to_char(buf, ch);
    return;
  }

  delete_doubledollar(argument);

  if (GET_PROMPT(ch))
    free(GET_PROMPT(ch));
  GET_PROMPT(ch) = str_dup(argument);

  sprintf(buf, "Okay, set your prompt to: %s\r\n", GET_PROMPT(ch));
  send_to_char(buf, ch);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, *filename, buf[MAX_STRING_LENGTH];
  struct stat fbuf;
  extern int max_filesize;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  world[ch->in_room].number, argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;
  extern int nameserver_is_slow;

  char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"AFK flag is now off.\r\n",
     "AFK flag is now on.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
     "You will now Auto-Assist.\r\n"},
    {"AutoSplit disabled.\r\n",
     "AutoSplit enabled.\r\n"},
    {"AutoLooting disabled.\r\n",
     "AutoLooting enabled.\r\n"},
    {"You can hear the music.\r\n",
     "You can't hear the music anymore :'-(\r\n"},
    {"You can now hear the ouch channel\r\n",
     "You will no longer hear of others pain\r\n"},
    {"AutoGold diabled.\r\n",
     "AutoGold enabled.\r\n"},
    {"You are no longer a pkiller.\r\n",
     "You can now kill pkillers, (and be killed by them).\r\n"},
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
    {
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
      sprintf(ch->player_specials->afkmsgs, "none");
    }
    else
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_NOMUSIC:
    result = PRF_TOG_CHK(ch, PRF_NOMUSIC);
    break;
  case SCMD_NOOUCH:
    result = PRF_TOG_CHK(ch, PRF_NOOUCH);
    break;
  case SCMD_PKILL:
    result = -1;
    if(PRF_FLAGGED(ch, PRF_PKILL) && GET_LEVEL(ch) < LVL_IMMORT)
      send_to_char("You can't turn it off :-P\r\n", ch);
    else      
      result = PRF_TOG_CHK(ch, PRF_PKILL);
    break;
  default:
    log("SYSERR: Unknown subcmd in do_gen_toggle");
    return;
    break;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

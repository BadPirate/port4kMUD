/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"

extern int arena_status;
extern void diag_char_to_char(struct char_data *, struct char_data *);


/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct descriptor_data *descriptor_list;
extern struct obj_data *object_list;
extern int pk_allowed;		/* see config.c */
extern int summon_allowed;	/*       "      */
extern int sleep_allowed;       /*       "      */
extern int charm_allowed;       /*       "      */
extern int auto_save;		/* see config.c */
extern int max_exp_gain;	/* see config.c */

/* External procedures */
extern void arena_pulse(void);
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
void stop_follower(struct char_data * ch);
ACMD(do_flee);
void hit(struct char_data * ch, struct char_data * victim, int type);
void forget(struct char_data * ch, struct char_data * victim);
void remember(struct char_data * ch, struct char_data * victim);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);
void amount_exp_gain(int, struct char_data *, struct char_data *); 
void amount_exp_loss(int, struct char_data *, struct char_data *);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void amount_exp_gain(int dam, struct char_data *ch, struct char_data *victim)
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

  
  gain_int *= dam;

  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA) || arena_status == -2)
  {
    sprintf(buf, "[%d EXP] ", gain_int);
    send_to_char(buf, ch);
    gain_exp(ch, gain_int);
  }
  return;
}

void amount_exp_loss(int dam, struct char_data *ch, struct char_data *victim)
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

  
  gain_int *= dam;
  gain_int /= -2;

  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA) || arena_status == -2)
  {
    sprintf(buf, "[%d EXP] ", gain_int);
    send_to_char(buf, ch);
    gain_exp(ch, gain_int);
  }
  return;
}

void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}



void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{

  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)
      && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) &&
      (ch != vict) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) ) {
    char buf[256];

    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_IMMORT, TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  assert(!FIGHTING(ch));

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!pk_allowed && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
      !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(vict, PRF_PKILL))) 
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



void make_corpse(struct char_data * ch, int return_to)
{
  struct obj_data *corpse, *o, *t;
  struct obj_data *money;
  int i;
  extern int max_npc_corpse_time, max_pc_corpse_time;

  struct obj_data *create_money(int amount);

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;

  if (IS_NPC(ch)) strcpy(buf2,"corpse");
    else sprintf(buf2, "corpse pcorpse %s",GET_NAME(ch));
  corpse->name = str_dup(buf2);

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = return_to;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = 1;
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL;)
    if(GET_OBJ_TYPE(o) != ITEM_MOBSPELL)
    {
      o->in_obj = corpse;
      o = o->next_content;
    }
    else
    {
      t = o;
      o = o->next_content;
      t->in_obj = corpse;
      obj_from_obj(t);
      extract_obj(t);
    }
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0 && IS_NPC(ch)) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  int foo;
  foo=(-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 4;
  if(IS_HOBBIT(ch)) {
    foo/=3;
    if(foo<10)foo=0;
  }
  GET_ALIGNMENT(ch) += foo;
}



void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}



void raw_kill(struct char_data * ch)
{
  int x, return_to=0;
  extern struct townstart_struct townstart[];
  sh_int corpse, morgue; 

  corpse = real_room(townstart[0].corpseroom);  
  morgue = real_room(townstart[0].deathroom);

  if (PLR_FLAGGED(ch, PLR_LOADROOM))
  {
    corpse = real_room(GET_LOADROOM(ch));
    morgue = real_room(GET_LOADROOM(ch));
  }
  else
  {
    corpse = real_room(townstart[0].corpseroom);
    morgue = real_room(townstart[0].deathroom);
  }

  for(x=0; townstart[x].minlevel != -1; x++)
  {
    if(GET_LOADROOM(ch) == townstart[x].loadroom)
    {
      corpse = real_room(townstart[x].corpseroom);
      morgue = real_room(townstart[x].deathroom);
    }
  }
  
  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  death_cry(ch);
  
  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
  {
    if(!IS_NPC(ch))
    {
      return_to = world[ch->in_room].number;
      char_from_room(ch);
      char_to_room(ch, corpse);
    }
    else
      return_to = 1;
    make_corpse(ch, return_to);
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA) && arena_status==-2)
    GET_HIT(ch) = GET_MAX_HIT(ch);
  else
    GET_HIT(ch) = 1;
  if(!IS_NPC(ch))
  {
    char_from_room(ch);
    char_to_room(ch, morgue);
    if (GET_HIT(ch) != GET_MAX_HIT(ch))
      ch->points.hit = 1;
    GET_POS(ch) = POS_STANDING;
    look_at_room(ch, 0);
  }
  else
    extract_char(ch);
  arena_pulse();
}



void die(struct char_data * ch)
{
  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    gain_exp(ch, -(GET_EXP(ch) >> 1));
  if (!IS_NPC(ch))
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  raw_kill(ch);
}



void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));

/*  if (share > 1) {
    sprintf(buf2, "You receive your share of experience -- %d points.\r\n", share);
    send_to_char(buf2, ch);
  } else
    send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);
  if(!ROOM_FLAGGED(ch->in_room, ROOM_ARENA) || arena_status == -2)
    gain_exp(ch, share); */
  change_alignment(ch, victim); 
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      tot_members++;

  /* round up to the next highest tot_members */
  base = (GET_EXP(victim) / 3) + tot_members - 1;

  if (tot_members >= 1)
    base = MAX(1, GET_EXP(victim) / (3 * tot_members));
  else
    base = 0;

  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room)
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      perform_group_gain(f->follower, base, victim);
}



char *replace_string(char *str, char *weapon_singular, char *weapon_plural)
{
  static char buf[256];
  char *cp;

  cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    char *to_room;
    char *to_char;
    char *to_victim;
  }

dam_weapons[] = {

/* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */
{
"$n tries to #w $N, but misses.",   /* 0: 0     */
"You try to #w $N, but miss.",
"$n tries to #w you, but misses."
},
{
"$n tickles $N as $e #W $M.",   /* 1: 1..2  */
"You tickle $N as you #w $M.",
"$n tickles you as $e #W you."
},
{
"$n barely #W $N.",   /* 2: 3..4  */
"You barely #w $N.",
"$n barely #W you."
},
{
"$n #W $N.",   /* 3: 5..6  */
"You #w $N.",
"$n #W you."
},

{
"$n #W $N hard.",   /* 4: 7..10  */
"You #w $N hard.",
"$n #W you hard."
},
{
"$n #W $N very hard.",   /* 5: 11..14  */
"You #w $N very hard.",
"$n #W you very hard."
},
{
"$n #W $N extremely hard.",   /* 6: 15..19  */
"You #w $N extremely hard.",
"$n #W you extremely hard."
},
{
"$n massacres $N to small fragments with $s #w.",   /* 7: 19..23 */
"You massacre $N to small fragments with your #w.",
"$n massacres you to small fragments with $s #w."
},
{
"$n obliterates $N with $s deadly #w!!",   /* 8: 23..33 */
"You obliterate $N with your deadly #w!!",
"$n obliterates you with $s deadly #w!!"
},
{
"$n vaporizes $N with a swift #w!!",   /* 9: 33..55 */
"You vaporize $N with your swift #w!!",
"$n vaporizes you with $s swift #w!!"
},
{
"$n decimates $N with $s vicious #w!!",   /* 10: 55..70 */
"You decimate $N with your vicious #w!!",
"$n decimates you with $s vicious #w!!"
},
{
"$n PULVERIZES $N with $s righteous #w!!!",   /* 10: 70..85 */
"You PULVERIZE $N with your righteous #w!!!",
"$n PULVERIZES you with $s righteous #w!!!"
},
{
"$n gets M E D I E V A L on $N with $s vicious #w!!!", /* 11: > 85 */
"You get M E D I E V A L on $N with your vicious #w!!!",
"$n gets M E D I E V A L on you with $s vicious #w!!!"
}};
  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else if (dam <= 33)   msgnum = 8;
  else if (dam <= 33)   msgnum = 8;
  else if (dam <= 55)   msgnum = 9;
  else if (dam <= 70)   msgnum = 10;
  else if (dam <= 85)   msgnum = 11;
  else			msgnum = 12;

  amount_exp_gain(dam, ch, victim);

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  send_to_char(CCYEL(ch, C_CMP), ch);
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(CCNRM(ch, C_CMP), ch);

  /* damage message to damagee */
  send_to_char(CCRED(victim, C_CMP), victim);
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  amount_exp_gain(dam, ch, vict);
  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
	if (GET_POS(vict) == POS_DEAD) {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
	send_to_char(CCYEL(ch, C_CMP), ch);
	act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	send_to_char(CCRED(vict, C_CMP), vict);
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_CMP), vict);

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}


void damage(struct char_data * ch, struct char_data * victim, int dam,
	    int attacktype)
{
  extern void arena_kill(struct char_data *, struct char_data *);
  ACMD(do_get);
  ACMD(do_split);
  int exp;
  long local_gold = 0;
  char local_buf[256];

  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage a corpse.");
    return;			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }
  
  if(ch != victim && MOB_FLAGGED(victim, MOB_NOATTACK)) {
    send_to_char("It's not very nice to attack them :-P\r\n", ch);
    return;
  }

  if (ch->in_room != victim->in_room) {
    send_to_char("They are no longer in the room", ch);
    return;
  }
  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return;

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT))
    dam = 0;
  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED) {
      if (!(FIGHTING(ch)))
	set_fighting(ch, victim);

      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
	  !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
	  (victim->master->in_room == ch->in_room)) {
	if (FIGHTING(ch))
	  stop_fighting(ch);
        hit(ch, victim->master, TYPE_UNDEFINED);
	return;
      }
    }
    if (GET_POS(victim) > POS_STUNNED && !FIGHTING(victim)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) &&
	  (GET_LEVEL(ch) < LVL_IMMORT))
	remember(victim, ch);
    }
  }
  if (victim->master == ch)
    stop_follower(victim);

  if (IS_AFFECTED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  if (IS_AFFECTED(victim, AFF_SANCTUARY))
    dam >>= 1;		/* 1/2 damage when sanctuary */

  if (!pk_allowed && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA) &&
      !(PRF_FLAGGED(ch, PRF_PKILL) && PRF_FLAGGED(victim, PRF_PKILL)))
  {
    check_killer(ch, victim);

    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  if(IS_NPC(ch))
    dam = MAX(dam,0);
  else
    dam = MAX(MIN(dam, 250), 0);
  GET_HIT(victim) -= dam;
  
    
/*  if (ch != victim && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    gain_exp(ch, gain_int);*/

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (attacktype != -1) {
    if (!IS_WEAPON(attacktype))
      skill_message(dam, ch, victim, attacktype);
    else {
      if (GET_POS(victim) == POS_DEAD || dam == 0) {
        if (!skill_message(dam, ch, victim, attacktype))
  	  dam_message(dam, ch, victim, attacktype);
      } else
        dam_message(dam, ch, victim, attacktype);
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) 
  {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n", victim);
    break;
  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) >> 2))
    {
      act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);
      if ( GET_WIMP_LEV(victim) == -1 )
      {
        send_to_char("You wimp! Another shot like that would kill you!\r\n",victim);
        do_flee(victim, "", 0, 0);
      }
    }
    if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
      sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
	      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      send_to_char(buf2, victim);
      if (MOB_FLAGGED(victim, MOB_WIMPY) && (ch != victim))
	do_flee(victim, "", 0, 0);
      if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim)) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, "", 0, 0);
      }
    }
    break;
  }

  if (!IS_NPC(victim) && !(victim->desc)) {
    do_flee(victim, "", 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }
  if (!AWAKE(victim))
    if (FIGHTING(victim))
      stop_fighting(victim);

  if (GET_POS(victim) == POS_DEAD) {
    if (IS_NPC(victim) || victim->desc)
    {
      if (IS_AFFECTED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else {
	exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

	/* Calculate level-difference bonus */
	if (IS_NPC(ch))
	  exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	else
	  exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	exp = MAX(exp, 1);
/*        if(ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
          send_to_char("You have slain your adversary.",ch);
          exp = 0; }
  	else if (exp > 1) {
          sprintf(buf2, "You receive %d experience points.\r\n", exp);
	  send_to_char(buf2, ch);
	} else
	  send_to_char("You receive one lousy experience point.\r\n", ch);
          gain_exp(ch, exp); */
	change_alignment(ch, victim);
      }
    }
    if (!IS_NPC(victim)) 
    {
      if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA) && arena_status == -2)
        arena_kill(ch, victim);
      else
      {
        sprintf(buf2, "%s killed by %s at %s\r\n", GET_NAME(victim), 
                GET_NAME(ch), world[victim->in_room].name);
        send_to_all(buf2);
        sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), 
                GET_NAME(ch), world[victim->in_room].name);
        mudlog(buf2, BRF, LVL_IMMORT, TRUE);
        if (MOB_FLAGGED(ch, MOB_MEMORY))
          forget(ch, victim);
      }
    } 
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }
    die(victim);
    /* If Autoloot enabled, get all corpse */
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) 
      do_get(ch,"all corpse",0,0);
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOGOLD))
      do_get(ch,"coin corpse",0,0);
    /* If Autoloot AND AutoSplit AND we got money, split with group */
    if (IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
        PRF_FLAGGED(ch, PRF_AUTOSPLIT) && (PRF_FLAGGED(ch,PRF_AUTOLOOT) 
          || PRF_FLAGGED(ch,PRF_AUTOGOLD))) {
      do_split(ch,local_buf,0,0);
    }
  }
}



void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;

  extern int thaco[NUM_CLASSES][LVL_IMPL+1];
  extern struct str_app_type str_app[];
  extern struct dex_app_type dex_app[];

  int backstab_mult(int level);

  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate the raw armor including magic armor.  Lower AC is better. */

  if (!IS_NPC(ch))
    calc_thaco = thaco[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)];
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;

  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  calc_thaco -= (int) ((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
  calc_thaco -= (int) ((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */
  diceroll = number(1, 20);

  victim_ac = GET_AC(victim) / 10;

  if (AWAKE(victim))
    victim_ac += dex_app[GET_DEX(victim)].defensive;

  victim_ac = MAX(-10, victim_ac);	/* -10 is lowest */

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else
      damage(ch, victim, 0, w_type);
  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */
    dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    if (wielded)
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    else {
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else
	dam += number(0, 2);	/* Max. 2 dam with bare hands */
    }

    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
    /* Position  sitting  x 1.33 */
    /* Position  resting  x 1.66 */
    /* Position  sleeping x 2.00 */
    /* Position  stunned  x 2.33 */
    /* Position  incap    x 2.66 */
    /* Position  mortally x 3.00 */

    dam = MAX(1, dam);		/* at least 1 hp damage min per hit */

    if (type == SKILL_BACKSTAB) {
      dam *= backstab_mult(GET_LEVEL(ch));
      damage(ch, victim, dam, SKILL_BACKSTAB);
    } else
      damage(ch, victim, dam, w_type);
  }
}



/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  extern struct index_data *mob_index;
  int apr, togo;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;
    apr = 0;

    if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
      stop_fighting(ch);
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char("You can't fight while sitting!!\r\n", ch);
      continue;
    }

    if (GET_SKILL(ch, SKILL_SECOND_ATTACK) >= number(1, 101)) {
      if (GET_SKILL(ch, SKILL_THIRD_ATTACK) >= number(1, 201)) apr++;
      apr++;
    }
    
    if (IS_NPC(ch))
    {
       togo = GET_LEVEL(ch);
       while (togo > 0)
       {
         togo -= 33;
         apr++;
       }
       apr--;
    }

    
    // increment apr by one for every attack they are supposed to get,
    // for the multiple attack skills, you should make sure they only
    // get a subsequent attack if they properly got the previous one.
    // For instance, you only get third attack if you are getting a
    // second attack.  This doesn't need to be skill based, you can
    // easily make it based upon class/level... see the second example
    // below.
    //
    //   if (AFF_FLAGGED(ch, AFF_HASTE))
    //     apr += number(0, 2);
    //    
    //   if (GET_CLASS(ch) == CLASS_WARRIOR && GET_LEVEL(ch) >= 10)
    //     apr++;
    //
    // If apr is negative they get no attacks, if apr is 0 they get
    // one attack.  APR has a range of -1 to 4, giving a minimum of
    // no attacks, to a maximum of 4.  See the below line for changing
    // that (eg., MAX(-1, MIN(apr, 6)) means a max of 6).
    
    apr = MAX(-1, MIN(apr, 4));
 
    if (apr >= 0) {
      for (; apr >= 0 && FIGHTING(ch); apr--)
        hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
      if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
        (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
    }
    if(FIGHTING(ch) != NULL)
      diag_char_to_char(FIGHTING(ch), ch);    
  }
}

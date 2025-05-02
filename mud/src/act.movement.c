/*************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "house.h"

/* external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern int rev_dir[];
extern char *dirs[];
extern int movement_loss[];

/* external functs */
int special(struct char_data *ch, int cmd, char *arg);
void death_cry(struct char_data *ch);
int find_eq_pos(struct char_data * ch, struct obj_data * obj, char *arg);
void dismount_char(struct char_data * ch);
void mount_char(struct char_data *ch, struct char_data *mount);

/* simple function to determine if char can walk on water */
int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return 1;
/*
  if (ROOM_IDENTITY(ch->in_room) == DEAD_SEA)
    return 1;
*/
  if (IS_AFFECTED(ch, AFF_WATERWALK))
    return 1;

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return 1;

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return 1;

  return 0;
}

  

/*  do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If success.
 *   0 : If fail
 */
int move_modify(struct char_data *ch, int base)
{
    int val;
    if(IS_DWARF(ch) && (SECT(ch->in_room) == SECT_MOUNTAIN)) {
         if(GET_LEVEL(ch)<20) val=2*base/3;
         else val=base/2;
    } else val=base;
    return(val);
}

// Fun with falling!
void falling(struct char_data *ch)
{
  int speed=0, ouch=0;
  sh_int down_room;

  send_to_char("You lose your grip!\r\n",ch);
  if (number(1,20) < GET_DEX(ch) && GET_MOVE(ch) > 20)
  {
    send_to_char("But you manage to grab hold of something, whew!\r\n",ch);
    GET_MOVE(ch) -= 20;
    return;
  }
  send_to_char("You've lost your footing and have begun to fall!\r\n",ch);
  if (!EXIT(ch,DOWN))
  {
    send_to_char("You fall harmlessly to the floor\r\n",ch);
    return;
  }
  while(EXIT(ch,DOWN))
  {
    speed++;
    ouch+=15*speed;
    send_to_char("You fall farther down!  This is gonna hurt!\r\n",ch);  
    
    down_room = EXIT(ch, DOWN)->to_room;
    char_from_room(ch);
    char_to_room(ch, down_room);
    if((number(1,20)*(speed+1)) < GET_DEX(ch) && GET_MOVE(ch) > 
       ((speed+1)*20))
    {
      send_to_char("You miraculously grab onto something and stop your plummet!\r\n",ch);
      GET_MOVE(ch) -= (speed+1)*20;
      return;
    }
  }
  send_to_char("&rYour body makes a sickening crunch as it hits the ground&n\r\n",ch);
  damage(ch, ch, ouch, -1);
  return;
}

int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  extern int arena_status;
  void falling(struct char_data *);
  int same_room = 0, riding = 0, ridden_by = 0;
  int was_in, need_movement;
  int vnum;
  void handle_dt_items(struct char_data *);

//  struct obj_data *obj;
 
/*  int check_dir, is_exit; */

  int special(struct char_data *ch, int cmd, char *arg);

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, ""))
    return 0;

  // check if they're mounted
  if (RIDING(ch))    riding = 1;
  if (RIDDEN_BY(ch)) ridden_by = 1;
  
  // if they're mounted, are they in the same room w/ their mount(ee)?
  if (riding && RIDING(ch)->in_room == ch->in_room)
    same_room = 1;
  else if (ridden_by && RIDDEN_BY(ch)->in_room == ch->in_room)
    same_room = 1;

  // tamed mobiles cannot move about (DAK)
  if (ridden_by && same_room && AFF_FLAGGED(ch, AFF_TAMED)) {
    send_to_char("You've been tamed.  Now act it!\r\n", ch);
    return 0;
  }

  // charmed?
  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) {
    send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((SECT(ch->in_room) == SECT_WATER_NOSWIM) ||
      (SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) {
    if ((riding && !has_boat(RIDING(ch))) || !has_boat(ch)) {
      send_to_char("You need a boat to go there.\r\n", ch);
      return 0;
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(ch->in_room)] +
		   movement_loss[SECT(EXIT(ch, dir)->to_room)]) >> 1;

  if (riding) {
    if (GET_MOVE(RIDING(ch)) < need_movement) {
      send_to_char("Your mount is too exhausted.\r\n", ch);
      return 0;
    }
  } else {
    if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
      if (need_specials_check && ch->master)
        send_to_char("You are too exhausted to follow.\r\n", ch);
      else
        send_to_char("You are too exhausted.\r\n", ch);
      return 0;
    }
  }
  
  if (riding && GET_SKILL(ch, SKILL_RIDING) < number(1, 101)-number(-4,need_movement)) {
    act("$N rears backwards, throwing you to the ground.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
    act("You rear backwards, throwing $n to the ground.", FALSE, ch, 0, RIDING(ch), TO_VICT);
    act("$N rears backwards, throwing $n to the ground.", FALSE, ch, 0, RIDING(ch), TO_NOTVICT);
    dismount_char(ch);
    damage(ch, ch, dice(1,6), -1);
    return 0;
  }
  
  vnum = world[EXIT(ch, dir)->to_room].number;
  
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_FALL) && number(1,20) >
      GET_DEX(ch))
  {
    falling(ch);
    return(0);
  }

  if (arena_status != -1)
  {
    if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_ARENA)  && 
        !IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ARENA))
    {
      send_to_char("There is an arena going on, I'm afraid you can't enter\r\n", ch);
      return 0;
    }
    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ARENA)  &&
        !IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_ARENA))
    {
      send_to_char("You have entered, and now you can't leave :-P\r\n",ch);
      return 0;
    }
  }
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, vnum)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return 0;
    }
  }
  
  if ((riding || ridden_by) && IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_TUNNEL)) {
    send_to_char("There isn't enough room there, while mounted.\r\n", ch);
    return 0;
  } else {
    if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_TUNNEL) &&
        num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) > 1) {
      send_to_char("There isn't enough room there for more than one person!\r\n", ch);
      return 0;
    }
  }
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch) && !(riding || ridden_by))
    GET_MOVE(ch) -= move_modify(ch,need_movement);
  else if (riding)
    GET_MOVE(RIDING(ch)) -= move_modify(RIDING(ch),need_movement/2);
  else if (ridden_by)
    GET_MOVE(RIDDEN_BY(ch)) -= move_modify(RIDDEN_BY(ch),need_movement);

  if (riding) 
  {
    if (!IS_AFFECTED(RIDING(ch), AFF_SNEAK))
    {
      if (IS_AFFECTED(ch, AFF_SNEAK)) 
      {
        sprintf(buf2,  "$n leaves %s.", dirs[dir]);
        act(buf2, TRUE, RIDING(ch), 0, 0, TO_ROOM);
      } 
      else 
      {
        sprintf(buf2, "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
      }
    }
  }  
  else if (ridden_by) 
  {
    if (!IS_AFFECTED(ch, AFF_SNEAK))
    {
      if (IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK)) 
      {
        sprintf(buf2, "$n leaves %s.", dirs[dir]);
        act(buf2, TRUE, ch, 0, 0, TO_ROOM);
      } 
      else 
      {
        sprintf(buf2, "$n rides $N %s.", dirs[dir]);
        act(buf2, TRUE, RIDDEN_BY(ch), 0, ch, TO_NOTVICT);
      }
    }
  } else if (!IS_AFFECTED(ch, AFF_SNEAK)) {
    sprintf(buf2, "$n leaves %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }
  
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (riding && same_room && RIDING(ch)->in_room != ch->in_room) {
    char_from_room(RIDING(ch));
    char_to_room(RIDING(ch), ch->in_room);
  } else if (ridden_by && same_room && RIDDEN_BY(ch)->in_room != ch->in_room) {
    char_from_room(RIDDEN_BY(ch));
    char_to_room(RIDDEN_BY(ch), ch->in_room);
  }

  if (!IS_AFFECTED(ch, AFF_SNEAK)) {
    if (riding && same_room && !IS_AFFECTED(RIDING(ch), AFF_SNEAK)) {
      sprintf(buf2, "$n arrives from %s%s, riding $N.",
              (dir < UP  ? "the " : ""),
              (dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
      act(buf2, TRUE, ch, 0, RIDING(ch), TO_ROOM);
    } else if (ridden_by && same_room && !IS_AFFECTED(RIDDEN_BY(ch), AFF_SNEAK)) {
      sprintf(buf2, "$n arrives from %s%s, ridden by $N.",
      	      (dir < UP  ? "the " : ""),
      	      (dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
      act(buf2, TRUE, ch, 0, RIDDEN_BY(ch), TO_ROOM);
    } else if (!riding || (riding && !same_room)) {
      act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);
  }
}

  if (ch->desc != NULL)
    look_at_room(ch, 0);

  // DT! (Hopefully these are rare in your MUD) -dak
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH)) {
    if (GET_LEVEL(ch) < LVL_IMMORT) {
      log_death_trap(ch);
      death_cry(ch);
      handle_dt_items(ch);
    }
    
    if (riding && GET_LEVEL(RIDING(ch)) < LVL_IMMORT) {
      log_death_trap(ch);
      death_cry(ch);
      extract_char(ch);
    }
    
    if (ridden_by && GET_LEVEL(RIDDEN_BY(ch)) < LVL_IMMORT) {
      log_death_trap(ch);
      death_cry(ch);
      extract_char(ch);
    }
    return 0;
  }
  return 1;
}


int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  int was_in;
  struct follow_type *k, *next;

  if (ch == NULL || dir < 0 || dir > NUM_OF_DIRS) {
    sprintf(buf2, "Invalid direction: &r%d&w contact implementor.\r\n", dir);
    send_to_char(buf2 ,ch);
    return 0; }
  else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE)
    send_to_char("Alas, you cannot go that way...\r\n", ch);
  else if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword) {
      sprintf(buf2, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf2, ch);
    } else
      send_to_char("It seems to be closed.\r\n", ch);
  } else {
    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = ch->in_room;
    if (!do_simple_move(ch, dir, need_specials_check))
      return 0;

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((k->follower->in_room == was_in) &&
	  (GET_POS(k->follower) >= POS_STANDING)) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return 1;
  }
  return 0;
}

ACMD(do_recall)
{
  extern struct townstart_struct townstart[];
  struct follow_type *k, *next;
  sh_int was_in;

  was_in = ch->in_room;
  if (ch == NULL || IS_NPC(ch))
    return;

  if ( IS_SET(ROOM_FLAGS(ch->in_room), ROOM_NORECALL) && 
       GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char("You wave your hands, but feel powerless\r\n",ch);
    return;
  }

  act("$n disappears.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  if (GET_LOADROOM(ch) == -1)
    char_to_room(ch, real_room(townstart[0].loadroom));
  else
    char_to_room(ch, real_room(GET_LOADROOM(ch)));
  act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  if(ch->followers)
    for (k = ch->followers; k; k = next) 
    {
      next = k->next;
      if ((k->follower->in_room == was_in) &&
          (GET_POS(k->follower) >= POS_STANDING)) 
      {
        if(IS_NPC(k->follower))
        {
          act("$n disappears.", TRUE, k->follower, 0, 0, TO_ROOM);
          char_from_room(k->follower);
          if (GET_LOADROOM(ch) == -1)
            char_to_room(k->follower, real_room(townstart[0].loadroom));
          else
            char_to_room(k->follower, real_room(GET_LOADROOM(ch)));
          act("$n appears in the middle of the room.", TRUE, k->follower, 0,
              0, TO_ROOM);
          look_at_room(k->follower, 0);
          send_to_char("You recall with your master.\r\n",k->follower);
        }
        else
        {
          if (GET_LOADROOM(k->follower) != GET_LOADROOM(ch))
            send_to_char("You may not recall to that city.\r\n",k->follower);
          else
          {
            act("$n disappears.", TRUE, k->follower, 0, 0, TO_ROOM);
            char_from_room(k->follower);
            if (GET_LOADROOM(ch) == -1)
              char_to_room(k->follower, real_room(townstart[0].loadroom));
            else
              char_to_room(k->follower, real_room(GET_LOADROOM(ch)));
            act("$n appears in the middle of the room.", TRUE, k->follower,
              0, 0, TO_ROOM);
            look_at_room(k->follower, 0);
            send_to_char("You recall with the leader.\r\n",k->follower);
          }
        } 
      } 
    }  
}

ACMD(do_move)
{
  int good_dirs[NUM_OF_DIRS], i, j;

  if ( IS_SET(ROOM_FLAGS(ch->in_room), ROOM_MAZE) 
       && GET_INT(ch) < number(1, 20)) {
    if (!CAN_GO(ch, cmd - 1))
      perform_move(ch, cmd - 1, 0);
    else {
      /* gotta gather all the good directions to go to */
      for (i = 0, j = 0; i < NUM_OF_DIRS; i++)
        if (CAN_GO(ch, i))
          good_dirs[j++] = i;

      /* now choose a random slot from our harvested directions and go there */
      send_to_char("You feel lost\r\n",ch);
      perform_move(ch, good_dirs[number(0, j-1)], 0);
    }
  } else
    perform_move(ch, subcmd - 1, 0);
}


int find_door(struct char_data *ch, char *type, char *dir, char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) {	/* Partial Match */
      send_to_char("That's not a direction.\r\n", ch);
      return -1;
    }
    if (EXIT(ch, door))
      if (EXIT(ch, door)->keyword)
	if (isname(type, EXIT(ch, door)->keyword))
	  return door;
	else {
	  sprintf(buf2, "I see no %s there.\r\n", type);
	  send_to_char(buf2, ch);
	  return -1;
      } else
	return door;
    else {
      send_to_char("I really don't see how you can close anything there.\r\n", ch);
      return -1;
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
      send_to_char(buf2, ch);
      return -1;
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (isname(type, EXIT(ch, door)->keyword))
	    return door;

    sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    send_to_char(buf2, ch);
    return -1;
  }
}


int has_key(struct char_data *ch, int key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return 1;

  if (GET_EQ(ch, WEAR_HOLD))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
      return 1;

  return 0;
}



#define NEED_OPEN	1
#define NEED_CLOSED	2
#define NEED_UNLOCKED	4
#define NEED_LOCKED	8

char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  int other_room = 0;
  struct room_direction_data *back = 0;

  sprintf(buf, "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]))
      if (back->to_room != ch->in_room)
	back = 0;

  switch (scmd) {
  case SCMD_OPEN:
  case SCMD_CLOSE:
    OPEN_DOOR(ch->in_room, obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(OK, ch);
    break;
  case SCMD_UNLOCK:
  case SCMD_LOCK:
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("*Click*\r\n", ch);
    break;
  case SCMD_PICK:
    LOCK_DOOR(ch->in_room, obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char("The lock quickly yields to your skills.\r\n", ch);
    strcpy(buf, "$n skillfully picks the lock on ");
    break;
  }

  /* Notify the room */
  sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "), (obj) ? "$p" :
	  (EXIT(ch, door)->keyword ? "$F" : "door"));
  if (!(obj) || (obj->in_room != NOWHERE))
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back) {
    sprintf(buf, "The %s is %s%s from the other side.\r\n",
	 (back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
	    (scmd == SCMD_CLOSE) ? "d" : "ed");
    if (world[EXIT(ch, door)->to_room].people) {
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_ROOM);
      act(buf, FALSE, world[EXIT(ch, door)->to_room].people, 0, 0, TO_CHAR);
    }
  }
}


int ok_pick(struct char_data *ch, int keynum, int pickproof, int scmd)
{
  int percent;

  percent = number(1, 101);

  if (scmd == SCMD_PICK) {
    if (keynum < 0)
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    else if (pickproof)
      send_to_char("It resists your attempts to pick it.\r\n", ch);
    else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
      send_to_char("You failed to pick the lock.\r\n", ch);
    else
      return (1);
    return (0);
  }
  return (1);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			(IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE))) :\
			(IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
			(!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
			(!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF)) : \
			(IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(EXIT(ch, door)->key))
#define DOOR_LOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 1)) : \
					(EXIT(ch, door)->exit_info))

ACMD(do_gen_door)
{
  int door = -1, keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
    send_to_char(CAP(buf), ch);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char("But it's already closed!\r\n", ch);
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char("But it's currently open!\r\n", ch);
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char("It seems to be locked.\r\n", ch);
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}


/*
ACMD(do_enter)
{
  int door;

  one_argument(argument, buf);

  if (*buf) {			
				
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    sprintf(buf2, "There is no %s here.\r\n", buf);
    send_to_char(buf2, ch);
  } else if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are already indoors.\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	      IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("You can't seem to find anything to enter.\r\n", ch);
  }
}
*/

ACMD(do_leave)
{
  int door;

  if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_INDOORS))
    send_to_char("You are outside.. where do you want to go?\r\n", ch);
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) &&
	    !IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char("I see no obvious exits to the outside.\r\n", ch);
  }
}


ACMD(do_stand)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SITTING:
    act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_RESTING:
    act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and put your feet on the ground.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char("You're sitting already.\r\n", ch);
    break;
  case POS_RESTING:
    act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Sit down while fighting? are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
    act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_SLEEPING:
    act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
    break;
  case POS_FIGHTING:
    act("Rest while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
    break;
  default:
    act("You stop floating around, and stop to rest your tired bones.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char("You go to sleep.\r\n", ch);
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char("You are already sound asleep.\r\n", ch);
    break;
  case POS_FIGHTING:
    send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
    break;
  default:
    act("You stop floating around, and lie down to sleep.",
	FALSE, ch, 0, 0, TO_CHAR);
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}


ACMD(do_wake)
{
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("Maybe you should wake yourself up first.\r\n", ch);
    else if ((vict = get_char_room_vis(ch, arg)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (GET_POS(vict) > POS_SLEEPING)
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (IS_AFFECTED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (IS_AFFECTED(ch, AFF_SLEEP))
    send_to_char("You can't wake up!\r\n", ch);
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char("You are already awake...\r\n", ch);
  else {
    send_to_char("You awaken, and sit up.\r\n", ch);
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
  }
}


ACMD(do_follow)
{
  struct char_data *leader;

  void stop_follower(struct char_data *ch);
  void add_follower(struct char_data *ch, struct char_data *leader);

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char("You are already following yourself.\r\n", ch);
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}


// Mounts (DAK)
ACMD(do_mount) {
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  
  one_argument(argument, arg);
  
  if (!arg || !*arg) {
    send_to_char("Mount who?\r\n", ch);
    return;
  } else if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("There is no-one by that name here.\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("Ehh... no.\r\n", ch);
    return;
  } else if (RIDING(ch) || RIDDEN_BY(ch)) {
    send_to_char("You are already mounted.\r\n", ch);
    return;
  } else if (RIDING(vict) || RIDDEN_BY(vict)) {
    send_to_char("It is already mounted.\r\n", ch);
    return;
  } else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED)) {
    send_to_char("You can't mount that!\r\n", ch);
    return;
  } else if (!GET_SKILL(ch, SKILL_MOUNT)) {
    send_to_char("First you need to learn *how* to mount.\r\n", ch);
    return;
  } else if (GET_SKILL(ch, SKILL_MOUNT) <= number(1, 101)) {
    act("You try to mount $N, but slip and fall off.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n tries to mount you, but slips and falls off.", FALSE, ch, 0, vict, TO_VICT);
    act("$n tries to mount $N, but slips and falls off.", TRUE, ch, 0, vict, TO_NOTVICT);
    damage(ch, ch, dice(1, 2), -1);
    return;
  }
  
  act("You mount $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n mounts you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n mounts $N.", TRUE, ch, 0, vict, TO_NOTVICT);
  mount_char(ch, vict);
  
  if (IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_TAMED) && GET_SKILL(ch, SKILL_MOUNT) <= number(1, 101)) {
    act("$N suddenly bucks upwards, throwing you violently to the ground!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You buck violently and throw $n to the ground.", FALSE, ch, 0, vict, TO_VICT);
    dismount_char(ch);
    damage(vict, ch, dice(1,3), -1);
  }
}


ACMD(do_dismount) {
  if (!RIDING(ch)) {
    send_to_char("You aren't even riding anything.\r\n", ch);
    return;
  } else if (SECT(ch->in_room) == SECT_WATER_NOSWIM && !has_boat(ch)) {
    send_to_char("Yah, right, and then drown...\r\n", ch);
    return;
  }
  
  act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
  act("$n dismounts from you.", FALSE, ch, 0, RIDING(ch), TO_VICT);
  act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_NOTVICT);
  dismount_char(ch);
}


ACMD(do_buck) {
  if (!RIDDEN_BY(ch)) {
    send_to_char("You're not even being ridden!\r\n", ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_TAMED)) {
    send_to_char("But you're tamed!\r\n", ch);
    return;
  }
  
  act("You quickly buck, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_CHAR);
  act("$n quickly bucks, throwing you to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_VICT);
  act("$n quickly bucks, throwing $N to the ground.", FALSE, ch, 0, RIDDEN_BY(ch), TO_NOTVICT);
  GET_POS(RIDDEN_BY(ch)) = POS_SITTING;
  if (number(0, 4)) {
    send_to_char("You hit the ground hard!\r\n", RIDDEN_BY(ch));
    damage(RIDDEN_BY(ch), RIDDEN_BY(ch), dice(2,4), -1);
  }
  dismount_char(ch);
  
  
  // you might want to call set_fighting() or some non-sense here if you
  // want the mount to attack the unseated rider or vice-versa.
}


ACMD(do_tame) {
  char arg[MAX_INPUT_LENGTH];
  struct affected_type af;
  struct char_data *vict;
  
  one_argument(argument, arg);
  
  if (!arg || !*arg) {
    send_to_char("Tame who?\r\n", ch);
    return;
  } else if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("They're not here.\r\n", ch);
    return;
  } else if (GET_LEVEL(ch) < LVL_IMMORT && IS_NPC(vict) && !MOB_FLAGGED(vict, MOB_MOUNTABLE)) {
    send_to_char("You can't do that to them.\r\n", ch);
    return;
  } else if (!GET_SKILL(ch, SKILL_TAME)) {
    send_to_char("You don't even know how to tame something.\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("You can't do that.\r\n", ch);
    return;
  } else if (GET_SKILL(ch, SKILL_TAME) <= number(1, 101)) {
    send_to_char("You fail to tame it.\r\n", ch);
    return;
  }
  
  af.type = SKILL_TAME;
  af.duration = 24;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_TAMED;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
  
  act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n tames you.", FALSE, ch, 0, vict, TO_VICT);
  act("$n tames $N.", FALSE, ch, 0, vict, TO_NOTVICT);
}

#define DTSHOP 1460
#define DTMOB 161

void handle_dt_items(struct char_data *ch)
{
  /*
     OK.  DF has been really ugly with object handling so we should clean
     this up while we're at it.  Call this function with the hapless person and
     this should handle everything.  Just Raw Kill and dont extract.
  */

  struct obj_data *obj,*one;
  struct char_data *dtguy=0;
  int junk_stuff_anyway=0, dtguyroom, eq, rmob, cnum;
  
  
  if ((rmob=real_mobile(DTMOB))>0) {
    dtguyroom=real_room(DTSHOP);
    for(dtguy=world[dtguyroom].people; dtguy;
	dtguy=dtguy->next) {
      if(!IS_NPC(dtguy)) continue;
      if(dtguy->nr == rmob) break;
    }
  } else junk_stuff_anyway=1;

  if (!dtguy) junk_stuff_anyway=1;

  for(obj=ch->carrying; obj; obj=one) {
    one=obj->next_content;
    obj_from_char(obj);
    if(junk_stuff_anyway) obj_to_room(obj,ch->in_room);
    else 
    {
      if(GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      {
        cnum = GET_OBJ_RNUM(obj);
        extract_obj(obj);
        obj = read_object(cnum, REAL); 
      }
      obj_to_char(obj, dtguy);
    }
  }

  for(eq=0;eq<NUM_WEARS;eq++) {
    if(GET_EQ(ch,eq)) {
      obj=unequip_char(ch, eq);
      if(junk_stuff_anyway) obj_to_room(obj,ch->in_room);
      else 
      {
        if(GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
        {
          cnum = GET_OBJ_RNUM(obj);
          extract_obj(obj);
          obj = read_object(cnum, REAL); 
        }
        obj_to_char(obj,dtguy);
      }
    }
  }
  
  gain_exp(ch, -(GET_EXP(ch) >> 1));  
  char_from_room(ch);
  char_to_room(ch, real_room(DTSHOP));
  GET_HIT(ch) = 1;
  look_at_room(ch, 0);
}


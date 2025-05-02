/* A file to hold me arena code.. Jaxom */

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

// Global Variables
int arena_status = -1;
int arena_prize = 154;
int arena_zone = 1026;
int arena_beat = 5;

// Local Function Prototypes
void arena_kill(struct char_data *, struct char_data *);
void arena_pulse(void);
void arena_end();
int arena_users();

// Externals
extern struct room_data *world;
extern struct townstart_struct townstart[];
extern struct descriptor_data *descriptor_list;

int arena_users(void)
{
  int users=0;
  struct char_data *ach;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->connected)
      continue;

    if (d->original)
      ach = d->original;
    else if (!(ach = d->character))
      continue;

    if(ROOM_FLAGGED(ach->in_room, ROOM_ARENA))
      users++;
  }
  return users;
}

void arena_pulse(void)
{
  struct char_data *i;
  struct char_data *j;
  struct char_data *list;

  if (arena_status >= 0)
  {
    if(arena_beat < 0)
    {
      arena_status = -2;
      send_to_all("&n[&rArena&n] Time's up, the games started.\r\n");
      list = world[real_room(arena_zone)].people;
      if(list != NULL)
        for (i = list; i; i = j)
        {
          j = i->next_in_room;
          char_from_room(i);
          char_to_room(i, real_room(arena_zone+1));
          look_at_room(i, i->in_room);
          send_to_char("[&rArena&n] Welcome to the arena!\r\n", i);
        }
    }
    else
    {
      sprintf(buf, "&n[&rArena&n] Arena is still open to level %d and below, &rarena join&n to join\r\n",
              arena_status);
      sprintf(buf, "%s&n[&rArena&n] You have %d pulses remaining before start\r\n", 
              buf, arena_beat);
      send_to_all(buf);
      arena_beat--;
    }
    return;
  }

  if(arena_status == -2 && arena_users() <= 1)
    arena_end();
}


void arena_end()
{
  struct descriptor_data *d;
  struct char_data *ach;
  
  if(arena_status == -1)
    log("Arena_end called with no arena in progress\r\n");
  else
  {
    send_to_all("&n[&rArena&n] Thats all folks, its over.\r\n");
    arena_status = -1;
  }
  for (d = descriptor_list; d; d = d->next) 
  {
    if (d->connected)
      continue;

    if (d->original)
      ach = d->original;
    else if (!(ach = d->character))
      continue;
 
    if(ROOM_FLAGGED(ach->in_room, ROOM_ARENA))
    {
      act("$n disappears.", TRUE, ach, 0, 0, TO_ROOM);
      char_from_room(ach);
      if (GET_LOADROOM(ach) == -1)
        char_to_room(ach, real_room(townstart[0].loadroom));
      else
        char_to_room(ach, real_room(GET_LOADROOM(ach)));
      act("$n appears in the middle of the room.", TRUE, ach, 0, 0, TO_ROOM);
      look_at_room(ach, 0);
      send_to_char("The arena is over, you are returned home\r\n", ach);
    }
  }
}

void arena_kill(struct char_data *ch, struct char_data *victim)
{
  struct obj_data *obj = 0;
  char buf[200];
  int r_num;

  if(arena_users() == 2)
  {
    sprintf(buf, "&r[&nArena&r]&n %s has won the arena by killing %s!\r\n", 
            GET_NAME(ch), GET_NAME(victim));
    send_to_all(buf);
    if ((r_num = real_object(arena_prize)) < 0) {
      send_to_char("No prize for you.\r\n", ch);
    }
    else
    {
      send_to_char("You have recieved the prize!\r\n",ch);
      obj = read_object(r_num, REAL);
      obj_to_char(obj, ch);
    }
    if ((r_num = real_object(154)) < 0) {
      send_to_char("No ear for you, tell gods.\r\n", ch);
    }
    else
    {
      send_to_char("You have recieved the ear of your victim\r\n",ch);
      obj = read_object(r_num, REAL);
      obj_to_char(obj, ch);
    }
    send_to_char("Congratulations, you won!\r\n",ch);
    GET_HIT(ch) = GET_MAX_HIT(ch);
  }
  else
  {
    sprintf(buf, "[&rArena&n] %s has slain %s.\r\n", GET_NAME(ch), 
            GET_NAME(victim));
    send_to_all(buf);
    if ((r_num = real_object(154)) < 0) {
      send_to_char("No ear for you, tell gods.\r\n", ch);
    }
    else
    {
      send_to_char("You have recieved the ear of your victim\r\n",ch);
      obj = read_object(r_num, REAL);
      obj_to_char(obj, ch);
    }
    GET_HIT(victim) = GET_MAX_HIT(victim);
  }
}    

ACMD(do_arena)
{
  struct descriptor_data *d, *multi;
  struct char_data *i, *j;
  struct char_data *list, *ach;
  char argparam[10];
  int level;

  half_chop(argument, arg, argparam);

  if (GET_LEVEL(ch) >= LVL_IMMORT)  
  {
    if(is_abbrev(arg, "list"))
    {
      sprintf(buf, "You can do the following:\r\n"
                   "zone-     Set the current arena field.  The number you input\r\n"
                   "          is the number of the arena prep area, and the room number\r\n"
                   "          right after that must be the drop area, see 2900 for\r\n"
                   "          an example.\r\n"
                   "start-    Open up the arena to characters of level <= number\r\n"
                   "begin-    After starting the game, this the allows no one else\r\n"
                   "          to join the competition.\r\n"
                   "end-      Resets the battlefield.\r\n"
                   "join-     Lets you join an arena game in progress.\r\n"
                   "status-   Displays the current arena's status.\r\n"
                   "prize-    Lets you set the arena's prize, use an object number\r\n"
                   "extend-   Extend the arena count <number> pulses\r\n");

      send_to_char(buf,ch);
      return;
    }
    if (is_abbrev(arg, "extend"))
    {
      if(arena_status < 1)
      {
        send_to_char("There must be an arena pending.\r\n",ch);
        return;
      }
      if (atoi(argparam)+arena_beat > 100 || atoi(argparam)+arena_beat < 1)
      {
        send_to_char("The ending result must be between 1 and 100\r\n", ch);
        return;
      }
      arena_beat+=atoi(argparam);
      arena_beat++;
      arena_pulse();
      return;
    }
    if (is_abbrev(arg, "prize"))
    {
      arena_prize = atoi(argparam);
      sprintf(buf, "Prize set to %d.\r\n", arena_prize);
      send_to_char(buf,ch);
      return;
    }
    if (is_abbrev(arg, "zone"))
    {
      arena_zone = atoi(argparam);
      sprintf(buf, "Zone set to %d.\r\n", arena_zone);
      send_to_char(buf,ch);
      return;
    }
    if(is_abbrev(arg, "begin"))
    {
      if (arena_status)
      {
        arena_status = -2;
        send_to_all("&n[&rArena&n] Time's up, the games started.\r\n");
        list = world[real_room(arena_zone)].people;
        if(list != NULL)
          for (i = list; i; i = j)
          {
            j = i->next_in_room;
            char_from_room(i);
            char_to_room(i, real_room(arena_zone+1));
            look_at_room(i, i->in_room);
            send_to_char("[&rArena&n] Welcome to the arena!\r\n", i);
          }
      }
      else
        send_to_char("There is no game pending.\r\n",ch);
      return;
    }
    else if(is_abbrev(arg, "end"))
    {
      arena_end();
      return;
    }
    if(is_abbrev("start", arg))
    {
      level = atoi(argparam);
      if (arena_status == -2)
      {
        send_to_char("There is already a game in progress.\r\n", ch);
        return;
      }
      else if (arena_status != -1)
      {
        sprintf(buf, "The arena is already open to level %d characters.\r\n",
                arena_status);
        send_to_char(buf, ch);
        return;
      }
      if (level >= LVL_IMMORT || level < 0)
      {
        sprintf(buf,"Invalid level (%d)\r\n", level);
        send_to_char(buf, ch);
        return;
      }

      arena_beat = 5;
      arena_status = level;
      sprintf(buf, "[&rArena&n] Arena opened to all characters below %d\r\n",
              level+1);
      for (d = descriptor_list; d; d = d->next)
      {
        if (d->connected)
          continue;

        if (d->original)
          ach = d->original;
        else if (!(ach = d->character))
          continue;

        if(ROOM_FLAGGED(ach->in_room, ROOM_ARENA))
        {
          act("$n disappears.", TRUE, ach, 0, 0, TO_ROOM);
          char_from_room(ach);
          if (GET_LOADROOM(ach) == -1)
            char_to_room(ach, real_room(townstart[0].loadroom));
          else
            char_to_room(ach, real_room(GET_LOADROOM(ach)));
            look_at_room(ach, 0);
            send_to_char("You have been returned to make way for ARENA!\r\n", ach);
        }
      }
      send_to_all(buf);
      return;
    }
  }
  if(is_abbrev(arg, "list"))
    send_to_char("Arena commands available to you are:\r\n\r\n"
                 "join-     join a currently running game.\r\n"
                 "status-   check the status on a currently running game.\r\n"
                 "list-     this help menu\r\n", ch);
  
  if(is_abbrev(arg, "join"))
  {
    if(arena_status < 0)
    {
      if (arena_status == -1)
        send_to_char("There is no game going on now.\r\n", ch);
      else if (arena_status == -2)
        send_to_char("The game has already started now.\r\n", ch);
      else
      {
        send_to_char("Ooops.. Bug, mail Imp.\r\n",ch);
        arena_status = -1;
      }
      return;
    }
    for ( multi = descriptor_list; multi; multi = multi->next)
      if (strcmp(ch->desc->host,multi->host) == 0)
        if(world[multi->character->in_room].number == arena_zone)
        {
          send_to_char("Only one char per IP may join\r\n",ch);
          return;
        }
    if (GET_LEVEL(ch) <= arena_status)
    {
      char_from_room(ch);
      char_to_room(ch, real_room(arena_zone));
      look_at_room(ch, ch->in_room);
      sprintf(buf, "&n[&rArena&n] %s has joined the battle!\r\n",
              GET_NAME(ch));
      send_to_all(buf);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      return;
    }
    else
    {
      send_to_char("Your too high a level for this battle.\r\n",ch);
      return;
    }
  }
  if (strcmp("status", arg) == 0)
  {
    if(arena_status == -1)
      send_to_char("Current status is: [OFF                -1]\r\n",ch);
    else if(arena_status == -2)
      send_to_char("Current status is: [In progress        -2]\r\n",ch);
    else
    {
      sprintf(buf, "Current status is: [Open for level %5d]\r\n",
              arena_status);
      send_to_char(buf,ch);
    }
    sprintf(buf,   "Current # Players: %d\r\n", arena_users());
    sprintf(buf, "%sCurrent Prize    : %d\r\n", buf, arena_prize);
    sprintf(buf, "%sCurrent Arena    : %d\r\n", buf, arena_zone);
    send_to_char(buf,ch);
    return;
  }
  if (GET_LEVEL(ch) < LVL_IMMORT)  
  {
    sprintf(buf, "You can do the following:\r\n"
                 "join-     Lets you join an arena game in progress.\r\n"
                 "status-   Displays the current arena's status.\r\n");
    send_to_char(buf,ch);
    return;
  }
  else
  {
    sprintf(buf, "You can do the following:\r\n"
                 "zone-     Set the current arena field.  The number you input\r\n"
                 "          is the number of the arena prep area, and the room number\r\n"
                 "          right after that must be the drop area, see 2900 for\r\n"
                 "          an example.\r\n"
                 "start-    Open up the arena to characters of level <= number\r\n"
                 "begin-    After starting the game, this the allows no one else\r\n"
                 "          to join the competition.\r\n"
                 "end-      Resets the battlefield.\r\n"
                 "join-     Lets you join an arena game in progress.\r\n"
                 "status-   Displays the current arena's status.\r\n"
                 "prize-    Lets you set the arena's prize, use an object number\r\n"
                 "extend-   Extend the arena count <number> pulses\r\n");
    send_to_char(buf,ch);
    return;
  }
}

/*************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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


/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct command_info cmd_info[];
extern int top_of_world;

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);

#define BTV_NORTH  (1 << 0)
#define BTV_EAST   (1 << 1)
#define BTV_SOUTH  (1 << 2)
#define BTV_WEST   (1 << 3)
#define BTV_UP     (1 << 4)
#define BTV_DOWN   (1 << 5)


#define NUM_OBJ_SENTINELS 	4

struct gamble_items_struct
{
  int vnum;
  int odds;
  int prize_vnum;
  char *name;
  char *prize_name;
};

struct obj_sentinel_struct 
{
  int vnum;
  int direction;
  char *can_pass;
  char *cannot_pass;
};

struct arena_sentinels_struct
{
  int vnum;
  int to_room;
  char push_message[MAX_STRING_LENGTH];
};

struct gamble_items_struct gamble_items[] =
{
  { 
    154,      
    5,        
    100,
    "Arena Token",
    "Quest Token"
  },
  { 
    100,      
    1,        
    144,
    "&rRed&n Token",
    "&cCyan&n Token"
  },
  { 
    144,      
    1,        
    143,
    "&cCyan&n Token",
    "&bBlue&n Token" 
  },
  { 
    143,      
    1,        
    142,
    "&bBlue&n Token",
    "PLATINUM Token" 
  },
  { -1, -1, -1, "bug", "bug"  }  // This must be last
};
    
struct arena_sentinels_struct arena_sentinels[] =
{
  /*  First must be battlemaster */
  { 159,
    112,
    "Hey, not while we're fighting.\r\n"
  },
  { 154,
    3059,
    "Gotta keep you outta there, sorry.\r\n"
  },
  { 155,
    113,
    "Enjoy your stay in the arena!\r\n"
  },
  { -1,
    -1,
    "MUST BE LAST"
  }
};

struct obj_sentinel_struct obj_sentinel_data[NUM_OBJ_SENTINELS] = 
{
  { 702,
    NORTH,
    "Right this way!",
    "You don't have a ticket!  It must be in your inventory."
  },
  { 703,
    EAST,
    "Hope you've got a strong stomach.",
    "You need a freak show ticket in your inventory or no go."
  },
  { 102,
    SOUTH,
    "You may pass.",
    "You must have an Arena Club entry pass in your inventory."
  },
  { 118,
    -1,
    "&rB&gr&ce&ma&bk&n on through to the other side.",
    "You have to have a Mystic Portal pass in your inventory"
  }
};

struct social_type {
  char *cmd;
  int next_line;
};


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[MAX_SKILLS+1];

extern char *spells[];

void sort_spells(void)
{
  int a, b, tmp;

  /* initialize array */
  for (a = 1; a < MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < MAX_SKILLS - 1; a++)
    for (b = a + 1; b < MAX_SKILLS; b++)
      if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
	tmp = spell_sort_info[a];
	spell_sort_info[a] = spell_sort_info[b];
	spell_sort_info[b] = tmp;
      }
}


char *how_good(int percent)
{
  static char buf[256];

  if (percent == 0)
    strcpy(buf, " (not learned)");
  else if (percent <= 10)
    strcpy(buf, " (awful)");
  else if (percent <= 20)
    strcpy(buf, " (bad)");
  else if (percent <= 40)
    strcpy(buf, " (poor)");
  else if (percent <= 55)
    strcpy(buf, " (average)");
  else if (percent <= 70)
    strcpy(buf, " (fair)");
  else if (percent <= 80)
    strcpy(buf, " (good)");
  else if (percent <= 85)
    strcpy(buf, " (very good)");
  else
    strcpy(buf, " (superb)");

  return (buf);
}

char *prac_types[] = {
  "spell",
  "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills(struct char_data * ch)
{
  extern char *spells[];
  extern struct spell_info_type spell_info[];
  int i, sortpos, linepos, class, found=0;

  if (!GET_PRACTICES(ch))
    strcpy(buf, "You have no practice sessions remaining.\r\n");
  else
    sprintf(buf, "You have %d practice session%s remaining.\r\n",
	    GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));

  sprintf(buf, "%sYou know of the following %ss:\r\n", buf, SPLSKL(ch));

  strcpy(buf2, buf);

  for (sortpos = 1, linepos = 1; sortpos < MAX_SKILLS; sortpos++) 
  {
    i = spell_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    for(class=0, found=0; found != 2 && class <= NUM_CLASSES; class++)
      if(IS_SET(ch->player_specials->saved.classes_been, (1 << class)) || 
         GET_CLASS(ch) == class || GET_LEVEL(ch) >= LVL_IMMORT)
        if ((spell_info[i].min_level[(int) class] < LVL_IMMORT ||
            GET_LEVEL(ch) >= LVL_IMMORT) && strcmp(spells[i],"!UNUSED!")
            != 0) 
        {
          if (GET_LEVEL(ch) < spell_info[i].min_level[(int) class] &&
              GET_CLASS(ch) == class)
          {
            found = 1;
            sprintf(buf, "&r< %-20s - %-2d%% - %-3dlvl >&n", spells[i],
                    GET_SKILL(ch, i),
                    spell_info[i].min_level[(int) class]);
          }
          else
          {
            found = 2;
            sprintf(buf, "[ %-20s - %-2d%% - %-3dlvl ]&n", spells[i],
                    GET_SKILL(ch, i),
                    spell_info[i].min_level[(int) class]);
          }
        }
    if (found != 0)
    {
      strcat(buf2, buf);
      switch(linepos)
      {
        case 1: linepos++; break;
        case 2: linepos = 1; strcat(buf2, "\r\n"); break;
      }
    }
  }
  if(linepos==2)
    strcat(buf2,"\r\n");
  page_string(ch->desc, buf2, 1);
}

SPECIAL(corpse_guy)
{
  ACMD(do_say);
  int price=0, stop = FALSE;
  struct char_data *self;
  struct obj_data *i, *j;

  self=(struct char_data *) me;

  if(GET_LEVEL(ch) > 10)
    price = (GET_LEVEL(ch)-10)*1000;

  for (i = world[ch->in_room].contents; i;) 
    if(GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i,3))
    {
      j = i;
      i = i->next_content;
      obj_from_room(j);
      obj_to_char(j,self);
      act("$n greedily snags a corpse.",FALSE,self,0,0,TO_ROOM);
    }
    else
      i = i->next_content;

  if(IS_NPC(ch))
    return FALSE;

  if(IS_MOVE(cmd) || CMD_IS("corpse"))
  {
    if(IS_MOVE(cmd))
      do_say(self,"Hold on, I may have something for ya",0,0);
    act("$n begins rummaging through $s stuff", FALSE, self, 0, 0, TO_ROOM);
    for(i = self->carrying, stop = FALSE; i && !(stop); )
    {
      if(GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i,3))
      {
        act("$n holds up a corpse and compares the face.",FALSE,self,0,0,TO_ROOM);
        if(isname(GET_NAME(ch), i->name))
        {
          if(IS_MOVE(cmd))
          {
            do_say(self,"Yup this one's yours, so do go runnin' off",0,0);
            return TRUE;
          }
          stop = TRUE;
        }
        else
        {
          act("$n doesn't see a likeness, shakes his head and continues",
              FALSE,self,0,0,TO_ROOM);
          i = i->next_content;
        }
      }
      else
      {
        act("$n looks at something and throws it over his shoulder",FALSE,
            self,0,0,TO_ROOM);
        do_say(self,"Wonder how that got in there",0,0);
        j = i;
        i = i->next_content;
        obj_from_char(j);
        obj_to_room(j,self->in_room);
      }
    }
    if (!(stop))
    {
      do_say(self,"Guess not, see ya!",0,0);
      return FALSE;
    }
  }
  if (CMD_IS("corpse"))
  {
    if(!(stop))
    {
      do_say(self,"I don't have your corpse",0,0);
      return TRUE;
    }
    two_arguments(argument, buf1, buf2);
    if(strcmp("price",buf1) == 0)
    {
      sprintf(buf,"For the notorious %s? %d coins.\r\n",GET_NAME(ch),price);
      send_to_char(buf,ch);
      return TRUE;
    }
    else if(strcmp("buy",buf1) == 0)
    {
      if(GET_GOLD(ch) < price)
      {
        send_to_char("Your too broke, perhaps you should have me put it &bback&n.\r\n",ch);
        return TRUE;
      }
      GET_GOLD(ch) -= price;
      act("$n hands some money to the man.",FALSE,ch,0,0,TO_ROOM);
      obj_from_char(i);
      obj_to_char(i,ch);
      do_say(self,"There ya go.",0,0);
      return TRUE;
    }
    else if(strcmp(buf1,"back") == 0)
    {
      do_say(self,"Your loss.",0,0);
      obj_from_char(i);
      obj_to_room(i, real_room(GET_OBJ_VAL(i,3)));
      return TRUE;
    }
  }
  if(CMD_IS("corpse") || CMD_IS("list") || CMD_IS("help"))
  {
    send_to_char("&rA list of my services-&n\r\n"
                 "&bcorpse buy&n   - buy your corpse back from me\r\n"
                 "&bcorpse back&n  - Have me put your corpse back to your point of death\r\n"
                 "&bcorpse price&n - Find out how much it costs for you to buy a corpse\r\n",
                 ch);
    return TRUE;
  }
  return FALSE;
}

SPECIAL(gamble_master)
{
  extern void death_cry(struct char_data *);
  extern void raw_kill(struct char_data *);
  struct obj_data *obj;
  int x = 0;

  if(!CMD_IS("gamble") && !CMD_IS("list"))
    return FALSE;
  if(CMD_IS("gamble"))
  {
    two_arguments(argument, buf1, buf2);
    if(strcmp(buf1, "money") == 0)
    {
      if(GET_GOLD(ch) < atoi(buf2))
      {
        sprintf(buf, "&rSorry your funds, %d are less than %d&n\r\n",
                GET_GOLD(ch), atoi(buf2));
        send_to_char(buf, ch);
        return TRUE;
      }
      if(number(0,1) == 0)
      {
        if (atoi(buf2) <= 10000)
        {
          GET_GOLD(ch) -= atoi(buf2);
          sprintf(buf, "&rYOU LOST&n %d coins. You now have %d.\r\n", 
                  atoi(buf2), GET_GOLD(ch));
          send_to_char(buf, ch);
        }
        else
        {
          send_to_char("&rWhoa, too rich for my blood, but I'll do 10,000&n\r\n",ch);
          GET_GOLD(ch) -= 10000;
          sprintf(buf, "&rYOU LOST&n 10,000 coins. You now have %d.\r\n", 
                  GET_GOLD(ch));
          send_to_char(buf, ch);
        }
        WAIT_STATE(ch, PULSE_VIOLENCE);
        return TRUE;
      }
      if (atoi(buf2) <= 10000)
      {
        GET_GOLD(ch) += atoi(buf2);
        sprintf(buf, "&yYOU WON!&n %d coins. You now have %d.\r\n", 
                atoi(buf2), GET_GOLD(ch));
        send_to_char(buf, ch);
      }
      else
      {
        send_to_char("&rWhoa, too rich for my blood, but I'll do 10,000&n\r\n",ch);
        GET_GOLD(ch) += 10000;
        sprintf(buf, "&yYOU WON!&n 10,000 coins. You now have %d.\r\n", 
                GET_GOLD(ch));
        send_to_char(buf, ch);
      }
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return TRUE;
    }
    if(strcmp(buf1, "token") == 0)
    {
      if(GET_GOLD(ch) < 1500000)
      {
        sprintf(buf, "&rYou need 1.5 million coins to use this, you have %d&n\r\n",
                GET_GOLD(ch));
        send_to_char(buf, ch);
        return TRUE;
      }
      GET_GOLD(ch) -= 1500000;
      if(number(0,1) == 0)
      {
        sprintf(buf, "&rYOU LOST.&n You now have %d coins.\r\n", 
                GET_GOLD(ch));
        send_to_char(buf, ch);
        return TRUE;
      }
      sprintf(buf, "&yYOU WON!&n You now have %d coins and a Quest Token.\r\n",
              GET_GOLD(ch));
      send_to_char(buf, ch);
      obj = read_object(real_object(100), REAL);
      obj_to_char(obj, ch);
      return TRUE;
    }
    if(strcmp(buf1, "item") == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, buf2, ch->carrying)))
      {
        sprintf(buf, "You don't seem to be carrying a %s", buf2);
        send_to_char(buf, ch);
        return TRUE;
      }
      for(x=0; gamble_items[x].vnum != -1; x++)
        if(gamble_items[x].vnum == GET_OBJ_VNUM(obj))
        {
          extract_obj(obj);
          if(number(0, gamble_items[x].odds) != 0)
          {
            sprintf(buf, "&rYOU LOST&n a %s.\r\n", gamble_items[x].name);
            send_to_char(buf,ch);
            return TRUE;
          }
          sprintf(buf, "&yYOU WON&n a %s.\r\n", gamble_items[x].prize_name);
          send_to_char(buf,ch);
          obj = read_object(real_object(gamble_items[x].prize_vnum), REAL);
          obj_to_char(obj, ch);
          return TRUE;
        }
      send_to_char("&rThat item has no gamble capability.&n\r\n",ch);
      return TRUE;
    }
    if(strcmp(buf1, "odds") == 0)
    {
      sprintf(buf, "&bGame odds:&n for gamble item\r\n");
      for(x=0; gamble_items[x].vnum != -1; x++)
        sprintf(buf, "%s[1 in %d odds of turning %s into %s]\r\n",
                buf, (gamble_items[x].odds+1), gamble_items[x].name,
                gamble_items[x].prize_name);
      send_to_char(buf,ch);
      return TRUE;
    }         
  }
  send_to_char("&bGambling Options:&n\r\n"
               "Command-             Usage-\r\n"
               "- Money              gamble money x\r\n"
               "  Gamble x coins, if you win it's doubled, otherwise you lose it\r\n\r\n"
               "- token              gamble token\r\n"
               "  Pay 1.5 million coins for a 1 out of 2 chance at a Quest Token\r\n\r\n"
               "- item               gamble item <item name>\r\n"
               "  put whatever <item> up to a gamble, if it is a gamble able item you\r\n"
               "  have a chance at getting the prize for that item, otherwise you\r\n"
               "  lose it.\r\n\r\n"
               "- odds               gamble odds\r\n"
               "  Displays the items accepted by gamble item, and their odds of winning\r\n"
               ,ch);
  return TRUE;
}

SPECIAL(quest_token_master)
{
  int r_num, item_state = 0, x;
  struct obj_data *obj, *obj2;

  if (!CMD_IS("quest") && !CMD_IS("list"))
    return FALSE;
  if (CMD_IS("quest"))
  {
    two_arguments(argument, buf1, buf2);
    sprintf(buf, "%s used quest%s",GET_NAME(ch),argument);
    mudlog(buf, NRM, LVL_IMMORT,TRUE);
    if (strcmp("improve",buf1) == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, "cyan", ch->carrying)))
      {
        send_to_char("&rYou don't have a &cCyan&r quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 144)
      {
        send_to_char("&rDrop your other &cCyan&r objects first&n\r\n",ch);
        return TRUE;
      }
      if(!(obj2 = get_obj_in_list_vis(ch, buf2, ch->carrying)))
      {
        send_to_char("You have to be carrying the object\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_TYPE(obj2) != ITEM_WEAPON)
      {
        send_to_char("I can only improve weapons\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      for(x=0; x < MAX_OBJ_AFFECT && item_state < 3; x++)
        if (obj2->affected[x].location == APPLY_NONE)
        {
          if ( number(1,20) < x )
          {
            extract_obj(obj2);
            send_to_char("The Questmaster eats your item, Yummie!\r\n",ch);
            return(TRUE);
          }
          if (item_state == 0)
          {
            item_state++;
            obj2->affected[x].location = APPLY_DAMROLL;
            obj2->affected[x].modifier = 1;
            send_to_char("Added 1 to damroll.\r\n",ch);
          }
          else if (item_state == 1)
          {
            item_state++;
            obj2->affected[x].location = APPLY_HITROLL;
            obj2->affected[x].modifier = 1;
            send_to_char("Added 1 to hitroll.\r\n",ch);
          }
        }
        else if(obj2->affected[x].location == APPLY_DAMROLL && 
                item_state == 0 && obj2->affected[x].modifier <= 1)
        {
          item_state++;
          obj2->affected[x].modifier++;
          send_to_char("Added 1 to damroll.\r\n",ch);
        }
        else if(obj2->affected[x].location == APPLY_HITROLL &&
                item_state == 1 && obj2->affected[x].modifier <= 1)
        {
          item_state++;
          obj2->affected[x].modifier++;
          send_to_char("Added 1 to hitroll.\r\n",ch);
        }
      if (x == MAX_OBJ_AFFECT && item_state == 0)
      {
        send_to_char("&rTHE OBJECT OVERLOADS! And EXPLODES!&n\r\n", ch);
        extract_obj(obj2);
        return TRUE;
      }
      return TRUE;
    }
    if (strcmp("str",buf1) == 0)
    {
      if (GET_STR(ch) >= 18)
      {
        send_to_char("Your strength is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.str += 1;
      send_to_char("You feel stronger\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("int",buf1) == 0)
    {
      if (GET_INT(ch) >= 18)
      {
        send_to_char("Your intelligence is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.intel += 1;
      send_to_char("You feel smarter.\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("wis",buf1) == 0)
    {
      if (GET_WIS(ch) >= 18)
      {
        send_to_char("Your wisdom is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.wis += 1;
      send_to_char("You feel wiser\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("dex",buf1) == 0)
    {
      if (GET_DEX(ch) >= 18)
      {
        send_to_char("Your dexterity is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.dex += 1;
      send_to_char("You feel more agile\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("con",buf1) == 0)
    {
      if (GET_CON(ch) >= 18)
      {
        send_to_char("Your constitution is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.con += 1;
      send_to_char("You feel tougher\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("cha",buf1) == 0)
    {
      if (GET_CHA(ch) >= 18)
      {
        send_to_char("Your charisma is at max right now\r\n",ch);
        return TRUE;
      }
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou don't have a RED quest token&n\r\n",ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other RED items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      ch->real_abils.cha += 1;
      send_to_char("You feel more charismatic\r\n",ch);
      affect_total(ch);
      return TRUE;
    }
    if (strcmp("money",buf1) == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, buf2, ch->carrying)))
      {
        sprintf(buf, "&rYou don't have a %s&n\r\n", buf2);
        send_to_char(buf,ch);
        return TRUE;
      }
      switch(GET_OBJ_VNUM(obj))
      {
        case 154: GET_GOLD(ch) += 500000; break;
        case 100: GET_GOLD(ch) += 3000000; break;
        case 144: GET_GOLD(ch) += 5000000; break;
        case 143: GET_GOLD(ch) += 7000000; break;
        case 142: GET_GOLD(ch) += 9000000; break;
        default:
          sprintf(buf, "&r%s is not something that I will pay for&n", buf2);
          send_to_char(buf,ch);
          return TRUE;
          break;
      };
      extract_obj(obj);
      send_to_char("You feel richer.\r\n",ch);
      return TRUE;
    }
    if( strcmp("hunger",buf1) == 0 )
    {
      if(!(obj = get_obj_in_list_vis(ch, "blue", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a &bBlue&r token&n\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 143)
      {
        send_to_char("&rYou have to drop all your other Blue items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      send_to_char("You are no longer hungry\r\n",ch);
      GET_COND(ch, FULL) = -1;
      return TRUE;
    }

    if( strcmp("thirst",buf1) == 0 )
    {
      if(!(obj = get_obj_in_list_vis(ch, "blue", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a &bBlue&r token\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 143)
      {
        send_to_char("&rYou have to drop all your other Blue items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      GET_COND(ch, THIRST) = -1;
      send_to_char("You will no longer be thirsty\r\n",ch);
      return TRUE;
    }

    if ( strcmp("exchange",buf1) == 0 )
    {
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a RED token&n\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other red items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      r_num = real_object(104);
      obj = read_object(r_num, REAL);
      obj_to_char(obj, ch);
      r_num = real_object(118);
      obj = read_object(r_num, REAL);
      obj_to_char(obj, ch);
      send_to_char("You now have an arena and mystical portal pass\r\n",ch);
      return TRUE;
    }

    if (strcmp(buf1, "hit") == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a RED token&n\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other red items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      sprintf(buf, "You gained %d more hitpoints!",
              ((5000-GET_HIT(ch))/250));
      GET_MAX_HIT(ch) += (5000-GET_HIT(ch))/250;
      GET_HIT(ch) = GET_MAX_HIT(ch);
      send_to_char(buf,ch);
      return TRUE;
    }

    if (strcmp(buf1, "mana") == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a RED token&n\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other red items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      sprintf(buf, "You gained %d more max mana!",
              ((5000-GET_MANA(ch))/250));
      GET_MAX_MANA(ch) += (5000-GET_MAX_MANA(ch))/250;
      GET_MANA(ch) = GET_MAX_MANA(ch);
      send_to_char(buf,ch);
      return TRUE;
    }

    if (strcmp(buf1, "move") == 0)
    {
      if(!(obj = get_obj_in_list_vis(ch, "red", ch->carrying)))
      {
        send_to_char("&rYou have to be holding a RED token&n\r\n", ch);
        return TRUE;
      }
      if(GET_OBJ_VNUM(obj) != 100)
      {
        send_to_char("&rYou have to drop all your other red items&n\r\n",ch);
        return TRUE;
      }
      extract_obj(obj);
      sprintf(buf, "You gained %d more movepoints!",
              ((5000-GET_MOVE(ch))/250));
      GET_MAX_MOVE(ch) += (5000-GET_MOVE(ch))/250;
      GET_MOVE(ch) = GET_MAX_MOVE(ch);
      send_to_char(buf,ch);
      return TRUE;
    }
  }
    send_to_char(
"Quest Options:\r\n"
"* quest improve, 1 cyan token, give an item +1/+1, careful I eat items!\r\n"  
"* quest exchange, 1 red token, get 1 arena and 1 mystical portal pass.\r\n"
"* quest <stat>, 1 red token, improve your &rstr&n, &rint&n, &rwis&n, &rdex&n, &rcha&n by 1\r\n"
"* quest <stat>, 1 red token, improve your maximum &rhit&n, &rmove&n, &rmana&n by a few\r\n"
"* quest money <item>, 1 token, get money for your token, equal to token level\r\n"
"* quest <stat>, 1 blue token permantely cure your &rhunger&n or &rthirst&n\r\n",ch);
  return TRUE;
}

SPECIAL(arena_sentinel)
{
  extern int arena_status;
  int x;
  struct char_data *self;

  self=(struct char_data *) me;

  if ( IS_NPC(ch) )
    return FALSE;

  if(arena_status > 0 && GET_MOB_VNUM(self) == arena_sentinels[0].vnum)
    if(IS_MOVE(cmd))
    {    
      send_to_char("Hold your horses, it will start soon.\r\n",ch);
      return TRUE;
    }

  if(arena_status == -2)
  {
    for(x=0; arena_sentinels[x].vnum != -1; x++)
      if(arena_sentinels[x].vnum == GET_MOB_VNUM(self))
      {
        send_to_char(arena_sentinels[x].push_message, ch);
        char_from_room(ch);
        char_to_room(ch, real_room(arena_sentinels[x].to_room));
        return TRUE;
      }
    log("Non-exsistant arena mob call in spec_procs.c arena_sentinel()");
    return FALSE;
  }
  else
    return FALSE;
}



SPECIAL(obj_sentinel)
{
  ACMD(do_say);
  struct char_data *self;
  struct obj_data *obj;
  int x, vnum = -1;

  self=(struct char_data *) me;

  for(x=0; x <= NUM_OBJ_SENTINELS; x++)
    if ( obj_sentinel_data[x].vnum == GET_MOB_VNUM(self) )  
    {
      vnum = GET_MOB_VNUM(self);
      break;
    }
  
  if ( vnum == -1 )
  {
    send_to_char("Mob doesn't exist!\r\n",ch);
    return FALSE;
  }

  if ( IS_MOVE(cmd) )
  {
    if ( (cmd-1) != obj_sentinel_data[x].direction &&
        obj_sentinel_data[x].direction != -1)
    {
//      send_to_char("Direction not blocked!\r\n", ch);
      return FALSE;
    }
    sprintf(buf, "%d", vnum);
    if ((obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
    {
      extract_obj(obj);
      do_say(self, obj_sentinel_data[x].can_pass, 0, 0);
      return FALSE;
    }
    else
    {
      do_say(self, obj_sentinel_data[x].cannot_pass, 0, 0);
      return TRUE;
    }
  }
  else
    return FALSE;
}      

SPECIAL(monkey) {
  struct char_data *vict;
  struct obj_data *obj, *list;
  int to_take, obj_count = 0, found = 0;

  if ((cmd) || (GET_POS(ch) != POS_STANDING))
    return FALSE;

  /* Pick a victim */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!IS_NPC(vict) && (GET_LEVEL(vict) < LVL_IMMORT) && (!number(0,4)))
    {
      if (AWAKE(vict) && (number(0, GET_LEVEL(ch)) == 0))
      {
        act("You gasp as you realize $n is attempting to take something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n just tried to take something from $N!", TRUE, ch, 0, vict, TO_NOTVICT);
        return TRUE;
      } else
      {
        /* Decide whether to take armor or object */
        switch(number(0,3)) {
        case 0:
        case 2:
          break;
        case 1:
          log ("in monkey - case 1");
          to_take = number(0, 25);
          if (GET_EQ(vict, to_take) && CAN_SEE_OBJ(ch, GET_EQ(vict,to_take)))
          {
            obj = GET_EQ(vict, to_take);
            act("$n has just stolen $p from $N!!",
                FALSE, ch, obj, vict, TO_NOTVICT);
            act("$n has just stolen $p from you!!",
                FALSE, ch, obj, vict, TO_VICT);

            obj_to_char(unequip_char(vict, to_take), vict);
            obj_from_char(obj);
            obj_to_room(obj, ch->in_room);
            act("$n has just exploded into little bitty bits!",
                FALSE, ch, 0, 0, TO_ROOM);
            extract_char(ch);
            return TRUE;
          }
          break;
        case 3: /* object */
          list = vict->carrying;
          for (obj = list; obj; obj = obj->next_content) {
            if (CAN_SEE_OBJ(vict, obj)) {
              obj_count += 1;
              found = 1;
            }
          }
          if (found == 1) /* Ok, they're carrying something, so pick one*/
          {
            to_take = number(0, obj_count);
            obj_count = 0;
            for (obj = list; obj; obj = obj->next_content)
            {
              obj_count += 1;
              if (obj_count == to_take)
              {
                act("$n has just stolen $p from $N!!",
                    FALSE, ch, obj, vict, TO_NOTVICT);
                act("$n has just stolen $p from you!!",
                    FALSE, ch, obj, vict, TO_VICT);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
                act("$n has just exploded into little bitty bits!",
                    FALSE, ch, 0, 0, TO_ROOM);
                extract_char(ch);
                return TRUE;
              }
            }
            break;
          }
        } /* end case 2 */
      }
    }
  return FALSE;
}

SPECIAL(guild)
{
  int skill_num, percent;

  extern struct spell_info_type spell_info[];
  extern struct int_app_type int_app[];

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    list_skills(ch);
    return 1;
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to practice now.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\r\n", ch);
    return 1;
  }
  send_to_char("You practice for a while...\r\n", ch);
  GET_PRACTICES(ch)--;

  percent = GET_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return 1;
}

SPECIAL(temple_cleric)
{
  struct char_data *vict;
  struct char_data *hitme = NULL;
  static int this_hour;
  int temp1 = 100;
  int temp2 = 100;

  if (cmd) return FALSE;

  if (time_info.hours != 0) {

  this_hour = time_info.hours;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
        if (IS_AFFECTED(vict,AFF_POISON)) hitme = vict;
    }
    if (hitme != NULL) {
          cast_spell(ch, hitme, NULL, SPELL_REMOVE_POISON);
          return TRUE;
         }

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
        if (IS_AFFECTED(vict,AFF_BLIND)) hitme = vict;
    }
    if (hitme != NULL) {
          cast_spell(ch, hitme, NULL, SPELL_CURE_BLIND);
          return TRUE;
         }

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    {
       temp1 = (100*GET_HIT(vict)) / GET_MAX_HIT(vict);
       if (temp1 < temp2) {
             temp2 = temp1;
             hitme = vict;
            }
    }
    if (hitme != NULL) {
          cast_spell(ch, hitme, NULL, SPELL_CURE_LIGHT);
          return TRUE;
         }
  }
  return 0;
}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  ACMD(do_drop);
  char *fname(char *namelist);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return 0;

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return 1;
}


SPECIAL(mayor)
{
  ACMD(do_gen_door);

  static char open_path[] =
  "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

  static char close_path[] =
  "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static char *path;
  static int index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return FALSE;

  switch (path[index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return FALSE;
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}


SPECIAL(snake)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return TRUE;
    }
  return FALSE;
}


SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return TRUE;

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return TRUE;

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  extern int guild_info[][3];
  struct char_data *guard = (struct char_data *) me;
  char *buf = "The guard humiliates you, and blocks your way.\r\n";
  char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	world[ch->in_room].number == guild_info[i][1] &&
	cmd == guild_info[i][2] && guild_info[i][0] != -1) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}



SPECIAL(puff)
{
  ACMD(do_say);

  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very hermaphroditic dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  case 4:
    do_say(ch, "I love Lord Jaxom, He's real cute.",0,0);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return TRUE;
  }

  return FALSE;
}


SPECIAL(cityguard)
{
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return FALSE;

  max_evil = 1000;
  evil = 0;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_THIEF)){
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }
  return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}



/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(spread)
{
  struct obj_data *obj = (struct obj_data *) me;

  if (obj->in_room == real_room(106))
  {  
    obj_from_room(obj);
    obj_to_room(obj, number(1, top_of_world));
    send_to_char("Something disappears.\r\n", ch); 
    return 0;
  }
  else
    return 0;
}

SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is %d coins.\r\n",
	      GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    GET_BANK_GOLD(ch) += amount;
    sprintf(buf, "You deposit %d coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) += amount;
    GET_BANK_GOLD(ch) -= amount;
    sprintf(buf, "You withdraw %d coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}

ACMD(do_enter)
{
  struct obj_data *portal;
  char obj_name[MAX_STRING_LENGTH];
  sh_int roomto;

  argument = one_argument(argument,obj_name);
  if (!(portal = get_obj_in_list_vis(ch,obj_name,world[ch->in_room].contents))) 
  {
    send_to_char("There is no portal by that name.\r\n",ch);
    return;
  }
  if(GET_OBJ_TYPE(portal) != ITEM_PORTAL)
  {
    send_to_char("Thats not a portal.\r\n",ch);
    return;
  }
  if (ch->points.move < 5)
  {
    send_to_char("Your too tired.\r\n", ch);
    return;
  }
  if (real_room(GET_OBJ_VAL(portal, 0)) == NOWHERE)
  {
    send_to_char("The portal seems to go nowhere..\r\n",ch);
    send_to_room("A portal vanishes as it loses bounds with reality.\r\n",ch->in_room);
    log("Portal with non-existant room attached. (spec_procs.c)");
    extract_obj(portal);
    return;
  }
  roomto = real_room(GET_OBJ_VAL(portal,0));
  sprintf(buf, "%s enters the portal.\r\n", GET_NAME(ch));
  send_to_room(buf, ch->in_room);
  char_from_room(ch);
  sprintf(buf, "%s pulls open a portal and steps through.\r\n", GET_NAME(ch));
  send_to_room(buf, roomto);
  char_to_room(ch, roomto);
  look_at_room(ch, 0);
  ch->points.move -= 5;
  return;
}

SPECIAL (redbutton)
{
  struct obj_data *obj = (struct obj_data *) me;
  struct obj_data *port;
  char obj_name[MAX_STRING_LENGTH];
  int roomto;

    if (!CMD_IS("push")) return FALSE;

    argument = one_argument(argument,obj_name);
    if (!(port = get_obj_in_list_vis(ch,obj_name,world[ch->in_room].contents))) 
    {
      return(FALSE);
    }

    if (GET_LEVEL(ch) == 1)
    {
      send_to_char("The button is dangerous, maybe next level.\r\n",ch);
      return(TRUE);
    }
    if (port != obj)
      return(FALSE);
    if (ch->points.move <= 0)
    {
      send_to_char("Your too tired.\r\n", ch);
      return(TRUE);
    }
        roomto = number(0, top_of_world);
        if (ROOM_FLAGGED(roomto, ROOM_NOTELEPORT) || 
	    ROOM_FLAGGED(roomto, ROOM_HOUSE) || /* Galvanon */
            ROOM_FLAGGED(roomto, ROOM_DEATH))
        {
          send_to_char("BOING!!  The button pops back and nothing happens.\r\n", ch);
          return(TRUE);
        }
        act("$n pushes $p and fades away.", FALSE, ch, port, 0, TO_ROOM);
        act("You push $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
        char_from_room(ch);
        ch->points.move -= 20;
        char_to_room(ch, roomto);
        look_at_room(ch, 0);
        act("$n slowly materializes from nowhere...", FALSE, ch, 0, 0, TO_ROOM);
        return(TRUE);
}


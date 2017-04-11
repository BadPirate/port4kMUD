/*************************************************************************
*   File: clan.c				    Addition to CircleMUD *
*  Usage: Source file for clan-specific code                              *
*                                                                         *
*  Copyright (c) to Daniel Muller					  *
*									  *
************************************************************************ */

/*
 * This file attempts to concentrate all of the clan specific code for the
 * implementation of clans in CircleMUD.  I've tried to mimic the code 
 * style of the rest of CircleMUD's code, to make it easier for you to
 * modify as needed.
*/



#include "conf.h"
#include "sysdep.h"
#include "string.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "clan.h"
#include "handler.h"

#define YES 	1
#define NO 	0

/*   external vars  */
extern struct room_data *world;
extern struct clan_info *clan_index;
extern struct descriptor_data *descriptor_list;
extern char *class_abbrevs[];
extern FILE *player_fl;
extern int clan_top;
extern int mini_mud;
extern struct player_index_element *player_table;
extern int top_of_p_table;

/* Local functions */
void clan_clean(int);
void remove_m(char *);
void prepare_colour_table(struct char_data *ch);
void update_clan_cross_reference();
void clan_save(int system, struct char_data *ch, int);
void write_clan(FILE *fl, int cnum, int);
void display_menu(struct char_data *ch, int cnum);
int is_clannum_already_used(int cnum);
void update_cnum(int oldcnum, int newcnum);
void setup_new_clan(struct char_data *ch, int cnum);
void setup_clan_edit(struct char_data *ch, int cnum);
void cleanup_clan_edit(struct descriptor_data *d, int method);
void refresh_clan_index();
void display_rank_menu(struct descriptor_data *d, int num);
void display_colour_menu(struct descriptor_data *d);
void fix_illegal_cnum();
int debug_clan(char *what, struct char_data *ch);
int already_being_edited(int cnum);

/* Define cross reference table */
int cross_reference[9999];

/* Define quick reference for colours (global) */
struct clan_colours clan_colour[8];

/* Define clan editing table */
struct clan_editing clan_edit[10];

/*
 * 			General functions go here
*/




/*
 * The general player commands go here
*/
ACMD(do_clan)
{
  extern int true_clan(int);
  char subbcmd[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];
  int  players_clan = 0, players_clannum = 0, ccmd = 0, value = 0, found = 0, temp = 0, temp2 = 0;
  struct char_data *vict = NULL;
  struct descriptor_data *pt;
//  struct char_data *clanchar = 0;
  *buf = 0;

  half_chop(argument, subbcmd, buf);
  strcpy(val_arg, buf);

  /* if (ch) belongs to a clan, find its number in the index array */
  if (!((PLAYERCLANNUM(ch) == 0) && (CLANRANK(ch) == 0)))
    for (ccmd = 0; ccmd <= clan_top; ccmd++)
      if (CLANNUM(clan_index[ccmd]) == PLAYERCLAN(ch))
      {
        players_clannum = ccmd;
	players_clan = CLANNUM(clan_index[ccmd]);
      }

  if (players_clan == 0)
    players_clan = -1;

  /* Prepare the colour table for this player */
  prepare_colour_table(ch);

  /* Update clan cross reference */
  update_clan_cross_reference();

  if (strcasecmp(subbcmd, "debug") == 0)
  {

    if (GET_LEVEL(ch) != LVL_IMPL)
    {
      send_to_char("Huh?!\r\n", ch);
      return;
    }

    /* Debug clan variables */
    if (debug_clan(val_arg, ch) == NO)
      send_to_char("Debug dump not written to disk.\r\n", ch);
    else
      send_to_char("Debug dump complete.\r\n", ch);
    return;
  }

  if (strcasecmp(subbcmd, "say") != 0 && GET_POS(ch) != POS_STANDING)
  {
    send_to_char("Your in the wrong position.\r\n",ch);
    return;
  }
  if (strcasecmp(subbcmd, "save") == 0)
  {
    /* Save clans */
    if (!(GET_LEVEL(ch) >= LVL_GOD))
    {
      send_to_char("Huh?!", ch);
      return;
    }
    clan_save(NO, ch, YES);
    return;
  }

  if (strcasecmp(subbcmd, "edit") == 0)
  {
    if (!(GET_LEVEL(ch) == LVL_IMPL))
    {
      send_to_char("Huh?!", ch);
      return;
    }
  
    value = atoi(val_arg);
    if (value == 0)
    {
      send_to_char("Zero is an illegal clan number.\r\n", ch);
      return;
    }

    if (already_being_edited(value) == YES)
    {
      send_to_char("Someone is already editing that clan number.\r\n", ch);
      return;
    }

    if (is_clannum_already_used(value) == YES)
      setup_clan_edit(ch, value);
    else
      setup_new_clan(ch, value);

    STATE(ch->desc) = CON_CLAN_EDIT;
    
    act("$n starts editing clans.", YES, ch, 0, 0, TO_ROOM);
    SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

    return;
  }

  if (strcasecmp(subbcmd, "list") == 0)
  {
    /* Display title for list */
    send_to_char("[   # Clan Title                Clan Owner     ]\r\n", ch);
    /* List all clans and owners */
    for (ccmd = 0; ccmd < clan_top; ccmd++)
    {
      sprintf(buf, " %4d %s%-25s%s %-15s\r\n", 
        CLANNUM(clan_index[ccmd]), CLANCOLOUR(ccmd), 
	CLANNAME(clan_index[ccmd]), 
        CCNRM(ch, C_NRM), CLANOWNER(clan_index[ccmd]));
      send_to_char(buf, ch);
    }
  sprintf(buf, "\r\n[%d clans displayed.]\r\n", (ccmd));
  send_to_char(buf, ch);

  return;
  }

  if (strcasecmp(subbcmd, "info") == 0)
  {
    /* Use index to retrieve information on clan */
    value = atoi(val_arg);
    if (value != 0)
    {
      for (ccmd = 0; ccmd < clan_top; ccmd++)
	if (CLANNUM(clan_index[ccmd]) == value)
	{
	  found = 1;
	  temp2 = 0;
	  
	  /* Find number of members in clan */
	  temp = CLANPLAYERS(clan_index[ccmd]);

	  temp2 = 0;
	  /* Find number of clan members currently online */
  	  for (pt = descriptor_list; pt; pt = pt->next)
    	    if (!pt->connected && pt->character && 
                pt->character->player_specials->saved.clannum == value)
	      temp2++;

	  /* Display info header */
	  sprintf(buf, "[%s", CLANCOLOUR(ccmd));
	  
	  sprintf(buf, "%s%-25s&n]\r\n \r\n", buf, CLANNAME(clan_index[ccmd]));

	  sprintf(buf, "%sOwner: %-15s\r\n",buf,CLANOWNER(clan_index[ccmd]));
	  sprintf(buf, "%sClan Gold: %d\r\n",buf,CLANGOLD(clan_index[ccmd]));
	  sprintf(buf, "%sMembers currently online: %-3d\r\nTotal number of members: %-3d\r\n \r\n&n", buf, temp2, temp);

          sprintf(buf, "%s%s&n\r\n",buf, CLANDESC(clan_index[ccmd]));
          page_string(ch->desc, buf, 1);
	}
    
      if (found == 0)
      {
	send_to_char("Unable to find a clan with that number.\r\n", ch);
	return;
      }
    }
    else
    {
      send_to_char("Unable to find a clan with that number.\r\n", ch);
    }
  return;
  }

  if (strcasecmp(subbcmd, "resign") == 0)
  {
    temp2 = 0;

    /* This handles clan resignations */
    if ((PLAYERCLAN(ch) == 0) && (CLANRANK(ch) == 0))
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    if ( true_clan(PLAYERCLAN(ch)) != -1)
    {
      if (strcasecmp(GET_NAME(ch), CLANOWNER(clan_index[PLAYERCLANNUM(ch)])) == 0)
      {
        send_to_char("You can't resign from a clan you own!\r\n", ch);
        return;
      }

      /* Update number of members in clan */
      if (CLANRANK(ch) > CLAN_APPLY)
        CLANPLAYERS(clan_index[players_clannum]) = CLANPLAYERS(clan_index[players_clannum]) --;  
    }
    
    PLAYERCLAN(ch) = 0;
    
CLANRANK(ch) = 0;
    
    send_to_char("You have resigned from your clan.\r\n", ch);

  return;
  }

  if (strcasecmp(subbcmd, "apply") == 0)
  {
    /* This handles clan applications */
    if (CLANRANK(ch) != 0)
    {
      send_to_char("You must resign from the clan you currently belong to, first.\r\n", ch);
      return;
    }

    value = 0;
    found = 0;
    value = atoi(val_arg);
    if (value >= 0)
    {
      for (ccmd = 0; ((ccmd < clan_top) && (found != 1)); ccmd++)
        if (CLANNUM(clan_index[ccmd]) == value)
	{
	  found = 1;

	  PLAYERCLAN(ch) = CLANNUM(clan_index[ccmd]);
	  CLANRANK(ch) = CLAN_APPLY;

	  sprintf(buf, "You have applied to the clan '%s'.\r\n", 
		CLANNAME(clan_index[ccmd]));
	  send_to_char(buf, ch);
	}

      if (found != 1)
      {
	send_to_char("Unable to find that clan.\r\n", ch);
	return;
      }
    }
    else
    {
      send_to_char("Which clan number?!\r\n", ch);
      return;
    }

  return;
  }

  if (strcasecmp(subbcmd, "enlist") == 0)
  {
    /* This handles the enlistment of characters who have applied to your
     * clan.
     *
     * Implementors can force an enlistment.  So, when creating a new clan,
     * make sure your owner applies to join it.  Then the implementor must
     * do a 'clan enlist <character>' so that the owner belongs to the clan.
    */
    
    if (!(GET_LEVEL(ch) == LVL_IMPL))
    {
      if ((CLANRANK(ch) == 0) && (PLAYERCLANNUM(ch) == 0))
      {
        send_to_char("But you don't belong to any clan!\r\n", ch);
        return;
      }

      if (!(CLANRANK(ch) >= CLAN_SARGEANT))
      {
	send_to_char("You have insufficent rank in your clan to do this.\r\n", ch);
	return;
      }

      if ((vict = get_player_vis(ch, val_arg, 0)))
      {
	if (CLANRANK(vict) != CLAN_APPLY)
	{
	  send_to_char("But that person isn't applying to any clan!\r\n", ch);
	  return;
	}

	if (PLAYERCLAN(vict) != PLAYERCLAN(ch))
	{
	  send_to_char("But that person isn't applying to your clan!\r\n", ch);
	  return;
	}

	CLANRANK(vict) = CLAN_SOLDIER;
	sprintf(buf, "You have been enlisted into the ranks of the '%s' clan.\r\n", 
		CLANNAME(clan_index[players_clannum]));
	send_to_char(buf, vict);

	sprintf(buf, "You have enlisted '%s' into your clan.\r\n", 
		GET_NAME(vict));
	send_to_char(buf, ch);

	SET_BIT(PRF_FLAGS(vict), PRF_CLANTALK);

	/* Update number of clan members */
	
	CLANPLAYERS(clan_index[players_clannum])++;
	
      }
      else
      {
	send_to_char("But there is noone here by that name!\r\n", ch);
	return;
      }
    }
    else
    {

      if ((vict = get_player_vis(ch, val_arg, 0)))
      {
        if (CLANRANK(vict) != CLAN_APPLY)
        {
	  send_to_char("But that person isn't applying to any clan!\r\n", ch);
	  return;
      	}

	if (strcasecmp(GET_NAME(vict), 
		CLANOWNER(clan_index[PLAYERCLANNUM(vict)])) == 0)
	  CLANRANK(vict) = CLAN_RULER;
	else
	  CLANRANK(vict) = CLAN_SOLDIER;
	log("Done forcing enlistment");

	sprintf(buf, "You have forced the enlistment of '%s' into the clan '%s'.\r\n", 
		GET_NAME(vict), CLANNAME(clan_index[PLAYERCLANNUM(vict)]));
	send_to_char(buf, ch);

	sprintf(buf, "You have been enlisted into the clan '%s'.\r\n", 
		CLANNAME(clan_index[PLAYERCLANNUM(vict)]));
	send_to_char(buf, vict);

	/* Update number of clan members */
	CLANPLAYERS(clan_index[PLAYERCLANNUM(vict)])++;
      }
      else
      {
        send_to_char("Unable to find a character of that name.\r\n", ch);
        return;
      }

    }
  return;
  }

  if (strcasecmp(subbcmd, "promote") == 0)
  {

    /* Clan promotions */
    if (CLANRANK(ch) < CLAN_RULER && GET_LEVEL(ch) < LVL_IMMORT)
    {
      send_to_char("You have insufficent rank to do that.\r\n", ch);
      return;
    }

    if ((vict = get_player_vis(ch, val_arg, 0)))
    {
      if (CLANRANK(vict) > CLANRANK(ch) && GET_LEVEL(ch) < LVL_IMMORT)
      {
	send_to_char("You can't promote someone who ranks higher than you!\r\n", ch);
	return;
      }
  
      if (((CLANRANK(vict) < CLAN_SOLDIER) || (PLAYERCLAN(vict) != PLAYERCLAN(ch))) && GET_LEVEL(ch) < LVL_IMMORT)
      {
	send_to_char("You can't promote someone who doesn't belong to your clan!\r\n", ch);
	return;
      }

      if (CLANRANK(vict) == CLAN_RULER)
      {
	send_to_char("You can't promote this person any higher in rank!\r\n", ch);
	return;
      }

      CLANRANK(vict)++;
      sprintf(buf, "%sYou have been promoted to ", CCRED(vict, C_NRM));
      if (CLANRANK(vict) == CLAN_SARGEANT)
        strcat(buf, SARGEANTTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_CAPTAIN) strcat(buf, CAPTAINTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_RULER) strcat(buf, RULERTITLE(clan_index[players_clannum]));

      sprintf(buf, "%s in your clan.%s\r\n", buf, CCNRM(ch, C_NRM));
      send_to_char(buf, vict);

      sprintf(buf, "%sYou have promoted '%s' to the rank of ", 
		CCRED(vict, C_NRM), GET_NAME(vict));
      if (CLANRANK(vict) == CLAN_SARGEANT) strcat(buf, SARGEANTTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_CAPTAIN) strcat(buf, CAPTAINTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_RULER) strcat(buf, RULERTITLE(clan_index[players_clannum]));

      sprintf(buf, "%s in your clan.%s\r\n", buf, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
    }
    else
    {
      send_to_char("Promote who?!\r\n", ch);
      return;
    }

  return;
  }

  if (strcasecmp(subbcmd, "demote") == 0)
  {
    /* Clan demotions */
    if (CLANRANK(ch) == 0)
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    if (CLANRANK(ch) < CLAN_CAPTAIN)
    {
      send_to_char("You have insufficent rank to do that.\r\n", ch);
      return;
    }

    if ((vict = get_player_vis(ch, val_arg, 0)))
    {
      if (CLANRANK(vict) > CLANRANK(ch))
      {
	send_to_char("You can't demote someone who ranks higher than you!\r\n", ch);
	return;
      }
  
      if ((CLANRANK(vict) < CLAN_SOLDIER) || 
		(PLAYERCLAN(vict) != PLAYERCLAN(ch)))
      {
	send_to_char("You can't demote someone who doesn't belong to your clan!\r\n", ch);
	return;
      }

      if (CLANRANK(vict) == CLAN_SOLDIER)
      {
	send_to_char("You can't demote this person any lower! Try clan boot <person>!\r\n", ch);
	return;
      }

      if (strcasecmp(GET_NAME(vict), CLANOWNER(clan_index[players_clannum])) == 0)
      {
	send_to_char("You can't demote the clan owner!\r\n", ch);
	return;
      }

      CLANRANK(vict)--;
      sprintf(buf, "%sYou have been demoted to ", CCRED(vict, C_NRM));
      if (CLANRANK(vict) == CLAN_SOLDIER) strcat(buf, SOLDIERTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_SARGEANT) strcat(buf, SARGEANTTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_CAPTAIN) strcat(buf, CAPTAINTITLE(clan_index[players_clannum]));

      sprintf(buf, "%s in your clan.%s\r\n", buf, CCNRM(ch, C_NRM));
      send_to_char(buf, vict);

      sprintf(buf, "%sYou have demoted '%s' to the rank of ", 
		CCRED(vict, C_NRM), GET_NAME(vict));
      if (CLANRANK(vict) == CLAN_SOLDIER) strcat(buf, SOLDIERTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_CAPTAIN) strcat(buf, CAPTAINTITLE(clan_index[players_clannum]));
      if (CLANRANK(vict) == CLAN_SARGEANT) strcat(buf, SARGEANTTITLE(clan_index[players_clannum]));

      sprintf(buf, "%s in your clan.%s\r\n", buf, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
    }
    else
    {
      send_to_char("Demote who?!\r\n", ch);
      return;
    }

  return;
  }

  if (strcasecmp(subbcmd, "recall") == 0)
  {
    if ((PLAYERCLAN(ch) == 0) || (CLANRANK(ch) <= 1))
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }
   
    if (CLANROOM(clan_index[players_clannum]) == 0)
    {
      send_to_char("Your clan does not have a room.\r\n",ch);
      return;
    }

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
    char_to_room(ch, real_room(CLANROOM(clan_index[players_clannum])));
    act("$n appears in the middle of the room.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    return;
  }
    

  if (strcasecmp(subbcmd, "who") == 0)
  {  

  /* List all clan members online, along with their clan rank 	*/
  /* Then list all the members of that clan 		      	*/

  if ((PLAYERCLAN(ch) == 0) || (CLANRANK(ch) == 0))
  {
    send_to_char("But you don't belong to any clan!\r\n", ch);
    return;
  }

  sprintf(buf, "Members of the clan '%s' currently online:\r\n", 
    CLANNAME(clan_index[players_clannum]));
  send_to_char(buf, ch);

  for (pt = descriptor_list; pt; pt = pt->next) 
    if (!pt->connected && pt->character && 
       PLAYERCLAN(pt->character) == PLAYERCLAN(ch))
    { 
      sprintf(buf, "%s[%2d %s ",  
              (GET_LEVEL(pt->character) >= LVL_IMMORT ? CCYEL(ch, C_SPR) : ""),
              GET_LEVEL(pt->character), CLASS_ABBR(pt->character));
	  if (CLANRANK(pt->character) == CLAN_APPLY) sprintf(buf, "%s%-10s", buf, "(Applying)");
	  if (CLANRANK(pt->character) == CLAN_SOLDIER) 
		sprintf(buf, "%s%-10s", buf, SOLDIERTITLE(clan_index[players_clannum])); 
	  if (CLANRANK(pt->character) == CLAN_SARGEANT) 
		sprintf(buf, "%s%-10s", buf, SARGEANTTITLE(clan_index[players_clannum])); 
	  if (CLANRANK(pt->character) == CLAN_CAPTAIN) 
		sprintf(buf, "%s%-10s", buf, CAPTAINTITLE(clan_index[players_clannum])); 
	  if (CLANRANK(pt->character) == CLAN_RULER) 
		sprintf(buf, "%s%-10s", buf, RULERTITLE(clan_index[players_clannum])); 
	  sprintf(buf, "%s] %s %s%s\r\n", buf, GET_NAME(pt->character), GET_TITLE(pt->character), CCNRM(ch, C_NRM));
	  send_to_char(buf, ch);
    }

/*
  sprintf(buf, "\r\nMembers of the clan '%s':\r\n", 
    CLANNAME(clan_index[players_clannum]));
  send_to_char(buf, ch);

 Removed cause of baaaaad memory leak
  for (i = 0; i <= top_of_p_table; i++) 
  {
    CREATE(clanchar, struct char_data, 1);
    clear_char(clanchar);
    if (load_char((player_table + i)->name, &tmp_store) > -1)
      store_to_char(&tmp_store, clanchar);
    if (!PLR_FLAGGED(clanchar, PLR_DELETED) &&
        PLAYERCLAN(clanchar) == PLAYERCLAN(ch))
    {
      sprintf(buf, "%s[%2d %s Rank: %d",  
          (GET_LEVEL(clanchar) >= LVL_IMMORT ? CCYEL(ch, C_SPR) : ""),
           GET_LEVEL(clanchar), CLASS_ABBR(clanchar), CLANRANK(clanchar));
       
      sprintf(buf, "%s] %s %s%s\r\n", buf, GET_NAME(clanchar), 
              GET_TITLE(clanchar), CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
      free_char(clanchar);
    }
    } */
    return;
  }

  if (strcasecmp(subbcmd, "say") == 0)
  {
    /* Clan say */
    if (CLANRANK(ch) == 0)
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    if (CLANRANK(ch) < CLAN_SOLDIER)
    {
      send_to_char("You have insufficent rank to do that!\r\n", ch);
      return;
    }

    if (!PRF_FLAGGED(ch, PRF_CLANTALK))
    {
      send_to_char("Try turning your clan talk channel on first, dork!\r\n", ch);
      return;
    }    

    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && 
          PLAYERCLAN(pt->character) == PLAYERCLAN(ch) &&
	  pt->character != ch && PRF_FLAGGED(pt->character, PRF_CLANTALK))
	{
	  sprintf(buf, "%s%s says to the clan, '%s'%s\r\n", CCRED(pt->character, C_NRM),
			GET_NAME(ch), val_arg, CCNRM(pt->character, C_NRM));
	  send_to_char(buf, pt->character);
        }
    sprintf(buf, "%sYou say to the clan, '%s'%s\r\n", CCRED(ch, C_NRM),
		val_arg, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

  return;
  }

  if (strcasecmp(subbcmd, "channel") == 0)
  {
    if (PRF_FLAGGED(ch, PRF_CLANTALK))
    {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_CLANTALK);
      send_to_char("Clan talk channel turned off.\r\n", ch);
    }
    else
    {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_CLANTALK);
      send_to_char("Clan talk channel turned on.\r\n", ch);
    }
  return;
  }

  if (strcasecmp(subbcmd, "deposit") == 0)
  {
    /* This handles the process of depositing gold into the clan account */
    /* This may only be done in the clan room */
    
    if ((PLAYERCLANNUM(ch) == 0) && (CLANRANK(ch) == 0))
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    if (CLANRANK(ch) < CLAN_SOLDIER)
    {
      send_to_char("You have insufficent rank to do that!\r\n", ch);
      return;
    }

    if (ch->in_room != real_room(CLANROOM(clan_index[players_clannum])))
    {
      send_to_char("You can only do that in clan rooms!\r\n", ch);
      return;
    }

    value = atoi(val_arg);
    if (value != 0)
    {
      if (ch->points.gold < value)
      {
   	send_to_char("But you don't have that much gold!\r\n", ch);
	return;
      }

      ch->points.gold = ch->points.gold - value;

      CLANGOLD(clan_index[players_clannum]) = CLANGOLD(clan_index[players_clannum]) + value;      

      clan_save(NO,ch,NO);
      sprintf(buf, "%d gold deposited into clan account.\r\n", value);
      send_to_char(buf, ch);

    }
    else
    {
      send_to_char("Huh?\r\n", ch);
    }

  return;
  }

  if (strcasecmp(subbcmd, "withdraw") == 0)
  {
    /* This handles withdrawals from the clan account */
    if ((CLANRANK(ch) == 0) && (PLAYERCLANNUM(ch) == 0))
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    value = atoi(val_arg);
    if (value > -1)
    {
      if (CLANRANK(ch) < CLAN_RULER)
      {
	send_to_char("You have insufficent rank to do that!\r\n", ch);
	return;
      }

      if (ch->in_room != real_room(CLANROOM(clan_index[players_clannum])))
      {
	send_to_char("But you can only do that in your clan room!\r\n", ch);
	return;
      }

      if (CLANGOLD(clan_index[players_clannum]) < value)
      {
	send_to_char("But your clan doesn't have that much money!\r\n", ch);
	return;
      }
      else
      {
        CLANGOLD(clan_index[players_clannum]) = CLANGOLD(clan_index[players_clannum]) - value;    
	ch->points.gold = ch->points.gold + value;
	sprintf(buf, "%d gold coins withdrawn from the clan account.\r\n",  value);
	send_to_char(buf, ch);
        clan_save(NO,ch,NO);
      }

    }
    else
    {
      send_to_char("Huh?\r\n", ch);
      return;
    }


  return;
  }

  if (strcasecmp(subbcmd, "kick") == 0)
  {
    if ((CLANRANK(ch) == 0) && (PLAYERCLANNUM(ch) == 0))
    {
      send_to_char("But you do not belong to any clan!\r\n", ch);
      return;
    }

    if (CLANRANK(ch) < CLAN_RULER)
    {
      send_to_char("You have insufficent rank to do that!\r\n", ch);
      return;
    }

    if ((vict = get_player_vis(ch, val_arg, 0)))
    {
      if (PLAYERCLAN(vict) != PLAYERCLAN(ch))
      {
	send_to_char("But that person doesn't belong to your clan!\r\n", ch);
	return;
      }

      if (strcasecmp(CLANOWNER(clan_index[players_clannum]), GET_NAME(vict)) == 0)
      {
        send_to_char("You can't kick the clan owner out!\r\n", ch);
        return;
      }

      CLANRANK(vict) = 0;
      PLAYERCLAN(vict) = 0;

      sprintf(buf, "You have kicked '%s' from your clan.\r\n", GET_NAME(vict));
      send_to_char(buf, ch);
      sprintf(buf, "You have been kicked from the clan, '%s'.\r\n", CLANNAME(clan_index[players_clannum]));
      send_to_char(buf, vict);

      /* Update number of members in clan */
      if (CLANRANK(vict) != CLAN_APPLY)
	CLANPLAYERS(clan_index[players_clannum])--;

    }
    else
    {
      send_to_char("Can't find that person.\r\n", ch);
      return;
    }
    return;
  }


  /* If no recognised subcommand given, display help */
  if (strcasecmp(subbcmd, "help") != 0)
  {
    send_to_char("Clan help:\r\n", ch);
    send_to_char("\r\n", ch);
    if (CLANRANK(ch) == 0)
      send_to_char("apply, info, list.\r\n", ch);
    if (CLANRANK(ch) == CLAN_APPLY)
      send_to_char("info, list, resign, who.\r\n", ch);
    if (CLANRANK(ch) == CLAN_SOLDIER)
      send_to_char("channel, deposit, info, list, recall, resign, say, who.\r\n", ch);
    if (CLANRANK(ch) == CLAN_SARGEANT)
      send_to_char("channel, deposit, enlist, info, list, recall, resign, say, who.\r\n", ch);
    if (CLANRANK(ch) == CLAN_CAPTAIN)
      send_to_char("channel, demote, deposit, enlist, info, list, promote, recall, resign, say, who.\r\n", ch);
    if (CLANRANK(ch) == CLAN_RULER)
    {
      send_to_char("channel, demote, deposit, enlist, info, kick, list, promote, resign, say,\r\n", ch);
      send_to_char("withdraw, who, recall.\r\n", ch);
    }
    if (GET_LEVEL(ch) == LVL_IMPL)
    {
      send_to_char("\r\nImplementor commands:\r\n", ch);
      send_to_char("debug, edit, save.\r\n", ch);
    }
    send_to_char("\r\n", ch);
    send_to_char("For further help type: clan help <command>\r\n", ch);
    return;
  }
  else
  {
    if (strcasecmp(val_arg, "debug") == 0)
    {
      send_to_char("CLAN DEBUG <type>\r\n", ch);
      send_to_char("\r\nDump debug information to debug file.\r\n", ch);
      send_to_char("Type 'clan debug' for more information.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "save") == 0)
    {
      send_to_char("CLAN SAVE\r\n", ch);
      send_to_char("\r\nSave clan data to clan files.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "recall") == 0)
    {
      send_to_char("CLAN RECALL\r\n", ch);
      send_to_char("\r\nGo to your own Clan Room.  Where you can deposit and\r\n",ch);
      send_to_char("withdraw.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "edit") == 0)
    {
      send_to_char("CLAN EDIT <num>\r\n", ch);
      send_to_char("\r\nEnter full screen clan editor for clan <num>.\r\n", ch);
      send_to_char("If <num> doesn't exist, a clan will be created.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "apply") == 0)
    {
      send_to_char("CLAN APPLY <num>\r\n", ch);
      send_to_char("\r\n", ch);
      send_to_char("Apply to the clan with the number <num>.  To find out the number of a\r\n", ch);
      send_to_char("clan, use the CLAN INFO command.\r\n", ch);
      send_to_char("\r\nRelated topics: resign, enlist.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "kick") == 0)
    {
      send_to_char("CLAN KICK <player>\r\n", ch);
      send_to_char("\r\nThis kicks <player> from your clan.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "channel") == 0)
    {
      send_to_char("CLAN CHANNEL\r\n", ch);
      send_to_char("\r\nUsed to toggle the clan talk channel.  Similar to the gossip channel command.\r\n", ch);
      send_to_char("\r\nRelated topics: say.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "demote") == 0)
    {
      send_to_char("CLAN DEMOTE <player>\r\n", ch);
      send_to_char("\r\nThis command demotes <player> one rank in your clan.\r\n", ch);
      send_to_char("\r\nRelated topics: promote.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "deposit") == 0)
    {
      send_to_char("CLAN DEPOSIT <amount>\r\n", ch);
      send_to_char("\r\nThis command deposits <amount> of gold into the clan account.\r\n", ch);
      send_to_char("This can only be done in the clan room.\r\n", ch);
      send_to_char("\r\nRelated topics: withdraw.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "enlist") == 0)
    {
      send_to_char("CLAN ENLIST <player>\r\n", ch);
      send_to_char("\r\nThis command enlists <player> into your clan.\r\n", ch);
      send_to_char("<player> must be applying to your clan first, of course.\r\n", ch);
      if (GET_LEVEL(ch) == LVL_IMPL)
        send_to_char("\r\nAs an implementor you can force the enlisting of <player> into\r\nthe clan they are applying for whether you are in that clan or not.\r\n", ch);

      send_to_char("\r\nRelated topics: apply, resign.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "info") == 0)
    {
      send_to_char("CLAN INFO <num>\r\n", ch);
      send_to_char("\r\nThis command gives info on the clan <num>.  To get the number of a\r\n", ch);
      send_to_char("clan, use the CLAN LIST command.\r\n", ch);
      send_to_char("\r\nRelated topics: list.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "list") == 0)
    {
      send_to_char("CLAN LIST\r\n", ch);
      send_to_char("\r\nThis command lists all the existing clans.\r\n", ch);
      send_to_char("\r\nRelated topics: info.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "promote") == 0)
    {
      send_to_char("CLAN PROMOTE <player>\r\n", ch);
      send_to_char("\r\nThis command promotes <player> a level in your clan.  Provided\r\n", ch);
      send_to_char("they already belong to your clan.\r\n", ch);
      send_to_char("\r\nRelated topics: demote, enlist.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "resign") == 0)
    {
      send_to_char("CLAN RESIGN\r\n", ch);
      send_to_char("\r\nThis command resigns you from your clan.\r\n", ch);
      send_to_char("\r\nRelated topics: apply, enlist.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "say") == 0)
    {
      send_to_char("CLAN SAY <message>\r\n", ch);
      send_to_char("\r\nThis command passes a message onto your fellow clan members.\r\n", ch);
      send_to_char("Much the same as GOSSIP.\r\n", ch);
      send_to_char("Abbreviation: | <message>\r\n", ch);
      send_to_char("\r\nRelated topics: channel.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "withdraw") == 0)
    {
      send_to_char("CLAN WITHDRAW <amount>\r\n", ch);
      send_to_char("\r\nThis withdraws <amount> of gold from the clan account.\r\n", ch);
      send_to_char("You must be in the clan room to do this.\r\n", ch);
      send_to_char("\r\nRelated topics: deposit.\r\n", ch);
      return;
    }
    if (strcasecmp(val_arg, "who") == 0)
    {
      send_to_char("CLAN WHO\r\n", ch);
      send_to_char("\r\nThis command is similar to WHO.  It displays all the clan members\r\n", ch);
      send_to_char("currently online.  As well as their rank in the clan.\r\n", ch);
      send_to_char("it now lists offline members as well.\r\n", ch);
      send_to_char("\r\nRelated topics: list, info.\r\n", ch);
      return;
    }
    send_to_char("Huh?\r\n", ch);
  }

};

ACMD(do_clansay)
{
  struct descriptor_data *pt;

    /* Clan say */
    if (CLANRANK(ch) == 0)
    {
      send_to_char("But you don't belong to any clan!\r\n", ch);
      return;
    }

    if (!PRF_FLAGGED(ch, PRF_CLANTALK))
    {
      send_to_char("Try turning your clan talk channel on first, dork!\r\n", ch);
      return;
    }    

    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && 
          PLAYERCLAN(pt->character) == PLAYERCLAN(ch) &&
	  pt->character != ch && PRF_FLAGGED(pt->character, PRF_CLANTALK))
	{
	  sprintf(buf, "%s%s says to the clan, '%s'%s\r\n", CCRED(pt->character, C_NRM),
			GET_NAME(ch), argument, CCNRM(pt->character, C_NRM));
	  send_to_char(buf, pt->character);
        }
    sprintf(buf, "%sYou say to the clan, '%s'%s\r\n", CCRED(ch, C_NRM),
		argument, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

};

/*
 * The following procedures attempt to fit into the db.c load procedures
 * to allow the dynamic creation, editing, and removal of clans whilst
 * in game play.
 *
 * Most of these functions will be almost direct copies of the db.c
 * procedures.
 *
*/

void parse_clan(FILE *fl, int virtual_nr)
{
  static int clan_nr = 0;
  int t[10];
  char line[256];

  clan_index[clan_nr].number = virtual_nr;
  clan_index[clan_nr].title = fread_string(fl, buf2);
  clan_index[clan_nr].description = fread_string(fl, buf2);
  clan_index[clan_nr].owner = fread_string(fl, buf2);
  clan_index[clan_nr].ranksoldiertitle = fread_string(fl, buf2);
  clan_index[clan_nr].ranksargeanttitle = fread_string(fl, buf2);
  clan_index[clan_nr].rankcaptaintitle = fread_string(fl, buf2);
  clan_index[clan_nr].rankrulertitle = fread_string(fl, buf2);

  if (!get_line(fl,line) || sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in clan #%d\n", virtual_nr);
    exit(1);
  }

  clan_index[clan_nr].clanroom = t[0];
  clan_index[clan_nr].clangold = t[1];
  clan_index[clan_nr].nummembers = t[2];
  clan_index[clan_nr].colour = t[3];
  clan_index[clan_nr].status = CLAN_OLD;
  clan_nr++;
}

void prepare_colour_table(struct char_data *ch)
{
  clan_colour[0].colour_string = CCWHT(ch, C_NRM);
  clan_colour[1].colour_string = CCGRN(ch, C_NRM);
  clan_colour[2].colour_string = CCRED(ch, C_NRM);
  clan_colour[3].colour_string = CCBLU(ch, C_NRM);
  clan_colour[4].colour_string = CCMAG(ch, C_NRM);
  clan_colour[5].colour_string = CCYEL(ch, C_NRM);
  clan_colour[6].colour_string = CCCYN(ch, C_NRM);
  clan_colour[7].colour_string = CCNRM(ch, C_NRM);
}

void update_clan_cross_reference()
{
  int i = 0;

  for (i = 0; i < 9999; i++)
    cross_reference[i] = -1;

  for (i = 0; i < clan_top; i++)
    cross_reference[CLANNUM(clan_index[i])] = i;

}

void clan_save(int system, struct char_data *ch, int fix)
{
  int i = 0, found = 0, last = 0, rec_count = 0, files = 0;
  FILE *fl, *index;
  char *index_file;
  char line[256];
  int reference[9999];

  if (mini_mud)
    index_file = MINDEX_FILE;
  else
    index_file = INDEX_FILE;

  sprintf(buf, "%s/%s", CLAN_PREFIX, index_file);

  if (!(index = fopen(buf, "r")))
  {
    log("FUNCTION(clan_save): Unable to open clan index file");
    if (system == NO)
      send_to_char("ClanSave: Unable to open index file (ERROR)\r\n", ch);
    return;
  }

  fscanf(index, "%s\n", buf);
  while (*buf != '$')
  {
    sprintf(buf2, "%s/%s", CLAN_PREFIX, buf);
    if (!(fl = fopen(buf2, "r")))
    {
      log("FUNCTION(clan_save): Unable to open sub file from index file");
      if (system == NO)
      {
	sprintf(buf, "ClanSave: Unable to open '%s' (ERROR)\r\n", buf2);
        send_to_char(buf, ch);
      }
      return;
    }
    else
    {
      rec_count = 0;

      while (found == 0)
      {
        if (!get_line(fl, line))
        {
          log("FUNCTION(clan_save): Format error encountered");
          if (system == NO);
          {
  	    sprintf(buf, "ClanSave: Format error after clan #%d (ERROR)\r\n", i);
            send_to_char(buf, ch);
          }
          exit(1);
        }

        if (*line == '$')
	  found = 1;

	if (*line == '#')
	{
	  last = i;

	  if (sscanf(line, "#%d", &i) != 1)
	  {
	    log("FUNCTION(clan_save): Format error in clan file");
	    if (system == NO)
	    {
	      sprintf(buf, "ClanSave: Format error after clan #%d\r\n", last);
	      send_to_char(buf, ch);
	    }
	    exit(1);
	  }
	  
	  if (i >= 99999)
	    return;

	  reference[rec_count] = i;
	  rec_count++;
	}	  
      }

      /* Now that we have a list of the clan #'s in this clan file */
      /* We replace them with the updated records */
      fclose(fl);
      
      sprintf(buf2, "%s/%s", CLAN_PREFIX, buf);
      if (!(fl = fopen(buf2, "w")))
      {
        log("FUNCTION(clan_save): Error truncuating clan file for rewrite");
	if (system == NO)
	{
	  sprintf(buf, "ClanSave: Unable to truncuate '%s' (ERROR)\r\n", buf2);
	  send_to_char(buf, ch);
	}
        return;
      }
      
      for (i = 0; i < rec_count; i++)
	write_clan(fl, reference[i], fix);

      /* New clan records go at the end of the first file in the index */
      if (files == 0)
        for (i = 0; i < clan_top; i++)
	  if (CLANSTATUS(i) == CLAN_NEW)
	  {
	    write_clan(fl, CLANNUM(clan_index[i]), YES);
	    CLANSTATUS(i) = CLAN_OLD;
	  }

      fputs("$~\n", fl);
      fclose(fl);
    }  

  fscanf(index, "%s\n", buf);
  files++;
  }

//  log("Successfully saved clan details");

  if (system == NO)
    send_to_char("Clans saved.\r\n", ch);
  fclose(index);
}

void write_clan(FILE *fl, int cnum, int fix)
{
  int j = 0;


  update_clan_cross_reference();
  j = cross_reference[cnum];

  if (j == -1) return;

  if(fix)
    clan_clean(j);
  fprintf(fl, "#%d\n", clan_index[j].number);
  fprintf(fl, "%s~\n", clan_index[j].title);
  fprintf(fl, "%s~\n", clan_index[j].description);
  fprintf(fl, "%s~\n", clan_index[j].owner);
  fprintf(fl, "%s~\n", clan_index[j].ranksoldiertitle);
  fprintf(fl, "%s~\n", clan_index[j].ranksargeanttitle);
  fprintf(fl, "%s~\n", clan_index[j].rankcaptaintitle);
  fprintf(fl, "%s~\n", clan_index[j].rankrulertitle);
  fprintf(fl, "%d %d %d %d\n", clan_index[j].clanroom, 
          clan_index[j].clangold, clan_index[j].nummembers, 
	  clan_index[j].colour);

}

void clan_clean(int clanpos)
{
  struct char_data *is_deleted = 0;
  struct char_file_u tmp_store;
  int i, playercount = 0;
  *buf = 0;
  
  remove_m(clan_index[clanpos].description); // Remove ^M from the descript

  for (i = 0; i <= top_of_p_table; i++) 
  {
    CREATE(is_deleted, struct char_data, 1);
    clear_char(is_deleted);
    if (load_char((player_table + i)->name, &tmp_store) > -1)
      store_to_char(&tmp_store, is_deleted);
    if (!PLR_FLAGGED(is_deleted, PLR_DELETED))
    {
      if (PLAYERCLANNUM(is_deleted) == clanpos)
        playercount++;
      free_char(is_deleted);
    }
  }
  clan_index[clanpos].nummembers = playercount;
}
  
void display_menu(struct char_data *ch, int cnum)
{

  sprintf(buf, "\x1B[H\x1B[J"
               "%s[Editing clan record]%s\r\n\r\n", 
	CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf2, "%sClan #%s%4d%s", CCYEL(ch, C_NRM), CCGRN(ch, C_NRM),
		 cnum, CCNRM(ch, C_NRM));
  sprintf(buf, "%54s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s'%s'%s", 
	CLANCOLOUR(cross_reference[cnum]), 
	CLANNAME(clan_index[cross_reference[cnum]]), 
	CCNRM(ch, C_NRM));
  sprintf(buf, "%54s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%sClan Gold : %s%d", CCYEL(ch, C_NRM), 
		CCGRN(ch, C_NRM),
		CLANGOLD(clan_index[cross_reference[cnum]]));
  sprintf(buf, "\r\n%50s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%sClan Room : %s%d", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM),
		CLANROOM(clan_index[cross_reference[cnum]]));
  sprintf(buf, "%50s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%sClan Owner : %s%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM),
		CLANOWNER(clan_index[cross_reference[cnum]]));
  sprintf(buf, "%56s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s1. %sChange clan number%s", CCYEL(ch, C_NRM), 
	CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "\r\n%59s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s2. %sChange clan name%s", CCYEL(ch, C_NRM), 
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%57s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s3. %sChange clan owner%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%58s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s4. %sChange clan description%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%64s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s5. %sChange clan colour%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%59s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s6. %sChange clan gold%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%57s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s7. %sChange clan room%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%57s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%s8. %sChange clan ranks%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%58s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%sQ. %sQuit editing clan%s", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf, "%58s\r\n", buf2);
  send_to_char(buf, ch);

  sprintf(buf2, "%sD. %sDelete clan%s", CCYEL(ch, C_NRM), CCGRN(ch, C_NRM),
		CCNRM(ch, C_NRM));
  sprintf(buf, "\r\n%52s\r\n", buf2);
  send_to_char(buf, ch);

}

/* Some generic tests */
int is_clannum_already_used(int cnum)
{
  update_clan_cross_reference();

  if (cross_reference[cnum] >= 0)
    return YES;


  return NO;

}

int already_being_edited(int cnum)
{
  int i = 0;

  for (i = 0; i < 10; i ++)
    if (CLAN_EDIT_CLAN(i) == cnum) return YES;

  return NO;
}

void update_cnum(int oldcnum, int newcnum)
{
  struct descriptor_data *pt;

  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && 
          PLAYERCLAN(pt->character) == oldcnum)
      PLAYERCLAN(pt->character) = newcnum;

  clan_index[cross_reference[oldcnum]].number = newcnum;

  update_clan_cross_reference();
}

void setup_new_clan(struct char_data *ch, int cnum)
{
  int i = 0;
  int node_assigned = 0;

  for (i = 0; i < 10; i++)
    if ((CLAN_EDIT_DESC(i) == NULL) && (node_assigned == NO))
    {
      CLAN_EDIT_DESC(i) = ch->desc;
      CLAN_EDIT_CLAN(i) = cnum;
      CLAN_EDIT_MODE(i) = CLAN_EDIT_MAIN_MENU;
      node_assigned = YES;
    }

  if (node_assigned == NO)
  {
    send_to_char("Sorry, but all editing slots are full.  Try again later.\r\n", ch);
    cleanup_clan_edit(ch->desc, CLEAN_ALL);
    return;
  }
  
  clan_index[clan_top].number = cnum;
  clan_index[clan_top].title = str_dup("New Clan");
  clan_index[clan_top].owner = str_dup(GET_NAME(ch));
  clan_index[clan_top].description = str_dup("A new clan");
  clan_index[clan_top].clangold = 0;
  clan_index[clan_top].clanroom = 0;
  clan_index[clan_top].colour = CLAN_NORMAL;
  clan_index[clan_top].ranksoldiertitle = str_dup("Soldier");
  clan_index[clan_top].ranksargeanttitle = str_dup("Sargeant");
  clan_index[clan_top].rankcaptaintitle = str_dup("Captain");
  clan_index[clan_top].rankrulertitle = str_dup("Ruler");
  clan_index[clan_top].status = CLAN_NEW;

  clan_top++;

  update_clan_cross_reference();

  display_menu(ch, cnum);

}

void setup_clan_edit(struct char_data *ch, int cnum)
{
  int i = 0;
  int node_assigned = 0;

  for (i = 0; i < 10; i++)
    if ((CLAN_EDIT_DESC(i) == NULL) && (node_assigned == NO))
    {
      CLAN_EDIT_DESC(i) = ch->desc;
      CLAN_EDIT_CLAN(i) = cnum;
      CLAN_EDIT_MODE(i) = CLAN_EDIT_MAIN_MENU;
      node_assigned = YES;
    }

  if (node_assigned == NO)
  {
    send_to_char("Sorry, but all editing slots are full.  Try again later.\r\n", ch);
    cleanup_clan_edit(ch->desc, CLEAN_ALL);
    return;
  }
  
  display_menu(ch, cnum);

}

void cleanup_clan_edit(struct descriptor_data *d, int method)
{
  int i = 0;

  for (i = 0; i < 10; i++)
    if (CLAN_EDIT_DESC(i) == d)
    {
      CLAN_EDIT_DESC(i) = NULL;
      CLAN_EDIT_MODE(i) = 0;
      if (method == CLEAN_ALL)
        CLANSTATUS(cross_reference[CLAN_EDIT_CLAN(i)]) = CLAN_DELETE;
      CLAN_EDIT_CLAN(i) = 0;
    }

  refresh_clan_index();

  if (d->character)
  {
    STATE(d) = CON_PLAYING;
    REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
    act("$n stops editing clans.", YES, d->character, 0, 0, TO_ROOM);
  }

  if (method == CLEAN_ALL)
    fix_illegal_cnum();

}

void fix_illegal_cnum()
{
  /* Search out and destroy cnum's in player's records which no longer */
  /* exist in the clan_index array */
  int i = 0, valid = 0;
  struct descriptor_data *pt;

  for (pt = descriptor_list; pt; pt = pt->next)
  {
    for (i = 0; i < clan_top; i++)
      if (clan_index[i].number == PLAYERCLAN(pt->character)) valid = YES;
    if (valid == NO)
    {
      sprintf(buf, "(ILLEGAL CLAN NUMBER FOUND) belonging to '%s' - #%d\n",
	GET_NAME(pt->character), PLAYERCLAN(pt->character));
      fprintf(stderr, "%s", buf);
      PLAYERCLAN(pt->character) = -1;
      CLANRANK(pt->character) = 0;
    }
    valid = NO;
  }

}

void refresh_clan_index()
{
  int i = 0;
  int j = 0;
  int k = 0;

  for (i = 0; i < clan_top; i++)
    if (CLANSTATUS(i) == CLAN_DELETE)
    {

      k = clan_index[i].number;
      clan_index[i].number = -1;
      update_cnum(k, -1);
      if (i != (clan_top - 1))
        for (j = i; j < (clan_top - 1); j++)
	  clan_index[j] = clan_index[j + 1];
      clan_top--;
    }

  update_clan_cross_reference();

}

void parse_clan_edit(struct descriptor_data *d, char *arg)
{
  int i = 0;
  int num = -1;

  for (i = 0; i < 10; i++)
    if (CLAN_EDIT_DESC(i) == d) num = i;

  if (num == -1) return;

  switch (CLAN_EDIT_MODE(num))
  {
    case CLAN_EDIT_MAIN_MENU:
      switch (*arg)
      {
        case 'd':
	case 'D':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_CONFIRM_DELETE;
	  send_to_char("\r\nAre you sure you wish to delete this clan? [Yy/Nn]", d->character);
	  break;
	case 'q':
	case 'Q':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_CONFIRM_SAVE;
	  send_to_char("\r\nDo you wish to save this clan? [Yy/Nn]", d->character);
	  break;
	case '1':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_NUMBER;
	  send_to_char("\r\nEnter new clan number: ", d->character);
	  break;
	case '2':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_NAME;
	  send_to_char("\r\nEnter clan name: \r\n", d->character);
	  break;
	case '3':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_OWNER;
	  send_to_char("\r\nEnter clan owners name: \r\n", d->character);
	  break;
	case '4':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_DESCRIPTION;
	  SEND_TO_Q("\x1B[H\x1B[J", d);
	  /*
	   * Attempting the impossible - Using the inbuilt text editor */
       	  clan_index[cross_reference[CLAN_EDIT_CLAN(num)]].old_description = NULL;
	  clan_index[cross_reference[CLAN_EDIT_CLAN(num)]].old_description =
		str_dup(CLANDESC(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]));
          SEND_TO_Q("Old clan description:\r\n\r\n", d);
	  SEND_TO_Q(CLANDESC(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]), d);
	  SEND_TO_Q("\r\n\r\nEnter new description: \r\n\r\n", d);
	  clan_index[cross_reference[CLAN_EDIT_CLAN(num)]].description = NULL;
	  d->str = &clan_index[cross_reference[CLAN_EDIT_CLAN(num)]].description;
	  d->max_str = 1000;
	  break;
	case '5':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_COLOUR;
	  display_colour_menu(d);
	  break;
	case '6':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_GOLD;
	  send_to_char("\r\nEnter new gold value: ", d->character);
	  break;
	case '7':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_CLANROOM;
	  send_to_char("\r\nEnter new clan room: ", d->character);
	  break;
	case '8':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_RANK_MENU;
	  display_rank_menu(d, num);
	  break;
	default:
	  send_to_char("r\nInvalid command!", d->character);
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
      }
      return;

    case CLAN_EDIT_DESCRIPTION_FINISHED:
      switch (*arg)
      {
	case 'S':
        case 's':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
	default:
	  CLANDESC(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) =
		str_dup(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]].old_description);
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
      }
      break;

    case CLAN_EDIT_CONFIRM_DELETE:
      switch (*arg)
      {
	case 'Y':
	case 'y':
	  CLANSTATUS(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_DELETE;
	  refresh_clan_index();
	  cleanup_clan_edit(d, CLEAN_NODE);
	  send_to_char("\r\nClan deleted.\r\n", d->character);
	  break;
	case 'N':
	case 'n':
	  send_to_char("\r\nNOT deleted.\r\n", d->character);
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
      }
      break;

    case CLAN_EDIT_CONFIRM_SAVE:
      switch (*arg)
      {
	case 'Y':
	case 'y':
	  cleanup_clan_edit(d, CLEAN_NODE);
	  send_to_char("\r\nSaved.\r\n", d->character);
	  break;
	case 'N':
	case 'n':
	  cleanup_clan_edit(d, CLEAN_ALL);
	  send_to_char("\r\nNOT saved.\r\n", d->character);
	  break;
	default:
	  send_to_char("\r\nInvalid command!", d->character);
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
      }
      return;

    case CLAN_EDIT_NUMBER:
      i = 0;
      i = atoi(arg);
      if (!(i <= 0))
      {
	if (is_clannum_already_used(i) == NO)
	{
	  update_cnum(CLAN_EDIT_CLAN(num), i);
	  CLAN_EDIT_CLAN(num) = i;
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	}
	else
	{
	  send_to_char("\r\nThat number already in use!\r\n", d->character);
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	}
      }
      else
      {
        send_to_char("\r\nInvalid number!\r\n", d->character);
	CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	display_menu(d->character, CLAN_EDIT_CLAN(num));
      }
      break;

    case CLAN_EDIT_NAME:
      CLANNAME(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) = str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
      display_menu(d->character, CLAN_EDIT_CLAN(num));
      break;

    case CLAN_EDIT_OWNER:
      /* No error checking here */
      CLANOWNER(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) = str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
      display_menu(d->character, CLAN_EDIT_CLAN(num));
      break;

    case CLAN_EDIT_GOLD:
      i = 0;
      i = atoi(arg);
      if (!(i <= 0))
      {
	CLANGOLD(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) = i;
	CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
        display_menu(d->character, CLAN_EDIT_CLAN(num));
      }
      else
      {
	send_to_char("\r\nInvalid number!\r\n", d->character);
	CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	display_menu(d->character, CLAN_EDIT_CLAN(num));
      }
      break;

    case CLAN_EDIT_CLANROOM:
	/* No error checking */
      i = 0;
      i = atoi(arg);
      if (!(i <= 0))
      {
	CLANROOM(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) = i;
	CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
        display_menu(d->character, CLAN_EDIT_CLAN(num));
      }
      else
      {
	send_to_char("\r\nInvalid number!\r\n", d->character);
	CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	display_menu(d->character, CLAN_EDIT_CLAN(num));
      }
      break;

    case CLAN_EDIT_COLOUR:
      switch (*arg)
      {
	case 'w':
	case 'W':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_WHITE;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'n':
	case 'N':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_NORMAL;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'r':
	case 'R':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_RED;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'g':
	case 'G':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_GREEN;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'b':
	case 'B':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_BLUE;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'y':
	case 'Y':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_YELLOW;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'm':
	case 'M':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_MAGENTA;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'c':
	case 'C':
	  CLANCOLOR(cross_reference[CLAN_EDIT_CLAN(num)]) = CLAN_CYAN;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
	default:
	  send_to_char("Invalid colour!\r\n", d->character);
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
          CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  break;
      }
      display_menu(d->character, CLAN_EDIT_CLAN(num));
      CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
      break;

    case CLAN_EDIT_SOLDIER:
      SOLDIERTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) =
	str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;

    case CLAN_EDIT_SARGEANT:
      SARGEANTTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) =
	str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;
  
    case CLAN_EDIT_CAPTAIN:
      CAPTAINTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) =
	str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;

    case CLAN_EDIT_RULER:
      RULERTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]) =
	str_dup(arg);
      CLAN_EDIT_MODE(num) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;

    case CLAN_EDIT_RANK_MENU:
      switch (*arg)
      {
	case '1':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_SOLDIER;
	  send_to_char("\r\nEnter new soldier rank: \r\n", d->character);
	  break;
	case '2':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_SARGEANT;
	  send_to_char("\r\nEnter new sargeant rank: \r\n", d->character);
	  break;
	case '3':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_CAPTAIN;
	  send_to_char("\r\nEnter new captain rank: \r\n", d->character);
	  break;
	case '4':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_RULER;
	  send_to_char("\r\nEnter new ruler rank: \r\n", d->character);
	  break;
	case 'q':
        case 'Q':
	  CLAN_EDIT_MODE(num) = CLAN_EDIT_MAIN_MENU;
	  display_menu(d->character, CLAN_EDIT_CLAN(num));
	  break;
	default:
	  send_to_char("\r\nInvalid command!\r\n", d->character);
	  display_rank_menu(d, num);
	  break;
      }
      break;

    }

}

void display_colour_menu(struct descriptor_data *d)
{
  struct char_data *ch;

  ch = d->character;

  send_to_char("\r\n\x1B[H\x1B[J", ch);
  sprintf(buf, "%s(W)hite\r\n", CCWHT(ch, C_NRM));
  sprintf(buf, "%s%s(R)ed\r\n", buf, CCRED(ch, C_NRM));
  sprintf(buf, "%s%s(G)reen\r\n", buf, CCGRN(ch, C_NRM));
  sprintf(buf, "%s%s(B)lue\r\n", buf, CCBLU(ch, C_NRM));
  sprintf(buf, "%s%s(Y)ellow\r\n", buf, CCYEL(ch, C_NRM));
  sprintf(buf, "%s%s(M)agenta\r\n", buf, CCMAG(ch, C_NRM));
  sprintf(buf, "%s%s(C)yan\r\n", buf, CCCYN(ch, C_NRM));
  sprintf(buf, "%s%s(N)ormal\r\n", buf, CCNRM(ch, C_NRM));
  sprintf(buf, "%s\r\nSelect a colour: ", buf);
  send_to_char(buf, ch);

}

void display_rank_menu(struct descriptor_data *d, int num)
{
  struct char_data *ch;

  ch = d->character;

  sprintf(buf, "\x1B[H\x1B[J\r\n%sSoldier Rank : %s%s\r\n", CCYEL(ch, C_NRM), 
		CCGRN(ch, C_NRM), 
	SOLDIERTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]));
  sprintf(buf, "%s%sSargeant Rank: %s%s\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	SARGEANTTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]));
  sprintf(buf, "%s%sCaptain Rank : %s%s\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	CAPTAINTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]));
  sprintf(buf, "%s%sRuler Rank: %s%s\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	RULERTITLE(clan_index[cross_reference[CLAN_EDIT_CLAN(num)]]));
  sprintf(buf, "%s\r\n%s1. %sEdit Soldier Rank\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf, "%s%s2. %sEdit Sargeant Rank\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf, "%s%s3. %sEdit Captain Rank\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf, "%s%s4. %sEdit Ruler Rank\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf, "%s\r\n%sQ. %sReturn to main menu\r\n", buf, CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  send_to_char(buf, ch);


}

/* Debug stuff goes here */
int debug_clan(char *what, struct char_data *ch)
{
  int i = 0, count = 0, debug_type = 0;
  time_t ct;
  char *tmstr;
  FILE *fl = NULL;

  /* Find type of debug request */
  if (strcasecmp(what, "cref") == 0)
    debug_type = CLAN_DEBUG_CREF;
  if (strcasecmp(what, "node") == 0)
    debug_type = CLAN_DEBUG_NODE;
  if (strcasecmp(what, "var") == 0)
    debug_type = CLAN_DEBUG_VAR;
  if (strcasecmp(what, "all") == 0)
    debug_type = CLAN_DEBUG_ALL;

  if (debug_type != CLAN_DEBUG_UNKNOWN)
  {
    /* Open debug file */
    sprintf(buf, "%s/DEBUG", CLAN_PREFIX);
    if (!(fl = fopen(buf, "a")))
    {
      fprintf(stderr, "Error opening clan debug file '%s'!\n", buf);
      return NO;
    }

    /* DateTime stamp entry */
    ct = time(0);
    tmstr = asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1 ) = '\0';
    fprintf(fl, "%-19.19s :: Debug requested by user\n", tmstr);
    fprintf(fl, "\nDebug requested by: %s\n", GET_NAME(ch));
  }

  if ((debug_type == CLAN_DEBUG_VAR) || (debug_type == CLAN_DEBUG_ALL))
  {
    /* Dump relevant global variables */
    fprintf(fl, "Debug request type: -GLOBAL VARIABLES-\n\n");
    
    fprintf(fl, "-CLAN STATUS-\n\n");
    fprintf(fl, "Clan # | Status\n");
    fprintf(fl, "-------|-------\n");

    for (i = 0; i < clan_top; i++)
      fprintf(fl, "  %-4d | %d    \n", clan_index[i].number, CLANSTATUS(i));


    fprintf(fl, "\n\n");
  }

  if ((debug_type == CLAN_DEBUG_CREF) || (debug_type == CLAN_DEBUG_ALL))
  {
    i = 0;
    /* Dump cross reference variable to debug file */
    fprintf(fl, "Debug request type: -CROSS REFERENCE-\n");

    fprintf(fl, "\nClan # | Index\n");
    fprintf(fl, "-------|------\n");
    for (i = 0; i < 9999; i++)
      if (cross_reference[i] != -1) 
	fprintf(fl, "%-6d | %d\n", i, cross_reference[i]);
      else
  	count++;
    fprintf(fl, "\n%d valid records, %d empty records.\n\n", 
	(9999 - count), count);
  }

  if ((debug_type == CLAN_DEBUG_NODE) || (debug_type == CLAN_DEBUG_ALL))
  {
    count = 0;
    /* Dump clan editing nodes to debug file */
    fprintf(fl, "Debug request type: -CLAN EDIT NODES-\n");

    fprintf(fl, "\nNode # | Clan # | Mode   | Owner\n");
    fprintf(fl, "-------|--------|--------|-------------------\n");
    for (i = 0; i < 10; i++)
      if (CLAN_EDIT_DESC(i) != NULL)
      {
	sprintf(buf2, "    %-2d |   %-4d | ", i, CLAN_EDIT_CLAN(i));
        switch (CLAN_EDIT_MODE(i))
    	{
	  case CLAN_EDIT_DESCRIPTION_FINISHED:
	    fprintf(fl, "%s-Desc- | %s\n", buf2,
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_CONFIRM_SAVE:
	    fprintf(fl, "%sCon-Sv | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_MAIN_MENU:
	    fprintf(fl, "%s-Main- | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_NAME:
	    fprintf(fl, "%sClnNam | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_NUMBER:
	    fprintf(fl, "%sClnNum | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_GOLD:
	    fprintf(fl, "%sClnGld | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_OWNER:
	    fprintf(fl, "%sClnOwn | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_DESCRIPTION:
	    fprintf(fl, "%sClnDsc | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_CLANROOM:
	    fprintf(fl, "%sCln-Rm | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_COLOUR:
	    fprintf(fl, "%sClnClr | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_RANK_MENU:
	    fprintf(fl, "%sRnkMnu | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_SELECT_COLOUR:
	    fprintf(fl, "%sSltClr | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_SOLDIER:
	    fprintf(fl, "%sRnkSld | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_SARGEANT:
	    fprintf(fl, "%sRnkSgt | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_CAPTAIN:
	    fprintf(fl, "%sRnkCpt | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  case CLAN_EDIT_RULER:
	    fprintf(fl, "%sRnkRlr | %s\n", buf2, 
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	  default:
	    fprintf(fl, "%s%d | %s\n", buf2, CLAN_EDIT_MODE(i),
		GET_NAME(CLAN_EDIT_DESC(i)->character));
	    break;
	}
      }
      else count++;

    fprintf(fl, "\n%d nodes used, %d free nodes.\n\n", (10 - count), count);
  }

  if (debug_type == CLAN_DEBUG_UNKNOWN)
  {
    send_to_char("clan debug <type>\r\n", ch);
    send_to_char("\r\nValid types:\r\n", ch);
    send_to_char("            cref          - cross reference variable\r\n", ch);
    send_to_char("            node          - clan edit nodes\r\n", ch);
    send_to_char("            var           - global variables\r\n", ch);
    send_to_char("            all           - all debug reports\r\n", ch);
    return NO;
  }

  fclose(fl);
  return YES;
}

void remove_m(char *thestring)
{
  int x, y;

  for(x=0, y=0; x <= strlen(thestring); x++, y++)
  {
    if(thestring[x] != 13)
      thestring[y] = thestring[x];
    else
      y--;
  }
}
/* (FIDO) End */

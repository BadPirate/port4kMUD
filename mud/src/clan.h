/* *************************************************************************
*   File: clan.h				     Addition to CircleMUD *
*  Usage: Contains structure definitions for clan.c                        *
*                                                                          *
*  Written by Daniel Muller                                                *
*                                                                          *
************************************************************************* */

struct clan_info {
   char *title;
   int colour;
   char *owner;
   char *description;
   char *old_description;
   char *ranksoldiertitle;
   char *ranksargeanttitle;
   char *rankcaptaintitle;
   char *rankrulertitle;
   int clanroom;
   int clangold;
   int nummembers;
   int number;
   int status;
};

struct clan_colours {
  char *colour_string;
};

struct clan_editing {
  int mode;		/* What edit mode are we in? */
  struct descriptor_data *desc; /* Who owns this clan node? */
  int clan;
};

#define CLAN_DEBUG_UNKNOWN		0
#define CLAN_DEBUG_ALL			1
#define CLAN_DEBUG_CREF			2
#define CLAN_DEBUG_NODE			3
#define CLAN_DEBUG_VAR			4

#define CLEAN_NODE	0
#define CLEAN_ALL	1

#define CLAN_EDIT_CONFIRM_SAVE	0
#define CLAN_EDIT_MAIN_MENU	1
#define CLAN_EDIT_NAME		2
#define CLAN_EDIT_NUMBER	3
#define CLAN_EDIT_GOLD		4
#define CLAN_EDIT_OWNER		5
#define CLAN_EDIT_DESCRIPTION	6
#define CLAN_EDIT_CLANROOM	7
#define CLAN_EDIT_COLOUR	8
#define CLAN_EDIT_RANK_MENU	9
#define CLAN_EDIT_SELECT_COLOUR 10
#define CLAN_EDIT_SOLDIER	11
#define CLAN_EDIT_SARGEANT	12
#define CLAN_EDIT_CAPTAIN	13
#define CLAN_EDIT_RULER		14
#define CLAN_EDIT_DESCRIPTION_FINISHED		15
#define CLAN_EDIT_CONFIRM_DELETE		16

#define CLAN_EDIT_MODE(num)	(clan_edit[(num)].mode)
#define CLAN_EDIT_DESC(num)	(clan_edit[(num)].desc)
#define CLAN_EDIT_CLAN(num)	(clan_edit[(num)].clan)

#define CLANSTATUS(cnum)	(clan_index[(cnum)].status)
#define CLAN_DELETE		2
#define CLAN_NEW		1
#define CLAN_OLD		0

#define PLAYERCLAN(ch)		((ch)->player_specials->saved.clannum)
#define PLAYERCLANNUM(ch)	(cross_reference[(ch)->player_specials->saved.clannum])
#define	CLANNUM(clan)		((clan).number)
#define	CLANNAME(clan)		((clan).title)
#define	CLANRANK(ch)		((ch)->player_specials->saved.clanrank)
#define CLANPLAYERS(clan) 	((clan).nummembers)	
#define CLANOWNER(clan)   	((clan).owner)
#define CLANGOLD(clan)		((clan).clangold)
#define CLANROOM(clan)		((clan).clanroom)
#define CLANDESC(clan)		((clan).description)
#define CLANCOLOUR(num)	(clan_colour[clan_index[(num)].colour].colour_string)
#define CLANCOLOR(num)		(clan_index[(num)].colour)

#define SOLDIERTITLE(clan)	((clan).ranksoldiertitle)
#define SARGEANTTITLE(clan)	((clan).ranksargeanttitle)
#define CAPTAINTITLE(clan)	((clan).rankcaptaintitle)
#define RULERTITLE(clan)	((clan).rankrulertitle)

#define CLAN_WHITE		0
#define CLAN_GREEN		1
#define CLAN_RED		2
#define CLAN_BLUE		3
#define CLAN_MAGENTA		4
#define CLAN_YELLOW		5
#define CLAN_CYAN		6
#define CLAN_NORMAL		7

#define CLAN_APPLY 		1
#define CLAN_SOLDIER 		2
#define CLAN_SARGEANT 		3
#define CLAN_CAPTAIN 		4
#define CLAN_RULER  		5

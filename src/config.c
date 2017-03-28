/* ************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"

#define TRUE	1
#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/* Level of mortal/immortal play is an attempt at keeping honest men honest
 * based on the level of play, you can choose how much allowances go in
 * = 0 - No restrictions, regular play
 * = 1 - Whenever a mort and an immort are on from same IP it logs
 * = 2 - Disallows morts and immorts to be on at same time
 */

int IMMORTPLAY = 1;

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed = NO;
int summon_allowed = NO;
int charm_allowed = YES;
int sleep_allowed = YES;
int roomaffect_allowed = NO;
int cheese_allowed = YES;

/* is playerthieving allowed? */
int pt_allowed = NO;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 1;

/* number of movement points it costs to holler */
int holler_move_cost = 5;

/* exp change limits */
int max_exp_gain = 100000;	/* max gainable per kill */
int max_exp_loss = 500000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 10;
int max_portal_time = 8;

/* should items in death traps automatically be junked? */
int dts_are_dumps = NO;

/* "okay" etc. */
char *OK = "Sure.\r\n";
char *NOPERSON = "I would love to, but.. They aren't around.\r\n";
char *NOEFFECT = "You hear the sound of distant laughter.\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int free_rent = YES;

/* maximum number of items players are allowed to rent */
int max_obj_save = 30;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 100;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 10;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 30;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
sh_int mortal_start_room = 3001;

/* virtual number of room that immorts should enter at by default */
sh_int immort_start_room = 1200;

/* virtual number of room that frozen players should enter at */
sh_int frozen_start_room = 1206;

/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 4000;

/* default directory to use as data directory */
char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 10000;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 50000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 3;

/*
 * Some nameservers are very slow and cause the game to lag terribly every 
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = NO;


char *MENU =
"\r\n"
"**************************************************\r\n"
" *            WELCOME BACK TO port4k.com 4000     *\r\n"
"  *                  You were missed.              *\r\n"
"   **************************************************\r\n"
"    *  MENU  *****************************************\r\n"
"     **************************************************\r\n"
"      *  0.  LEAVE THE GAME                            *\r\n"
"       *  1.  ENTER PORT 4000                           *\r\n"
"        *  3.  READ THE HISTORY OF PORT 4000             *\r\n"
"         *  4.  CHANGE YOUR CHARACTERS PASSWORD           *\r\n"
"          *  5.  DELETE YOUR CHARACTER, *COMPLETELY*       *\r\n"
"           **************************************************\r\n"
"\r\n"
"Your Choice? ";


char *GREETINGS =

"\r\n\r\n"
"      :::::::::  :::::::: ::::::::::::::::::::           #########              \r\n"
"     :+:    :+::+:    :+::+:    :+:   :+:                 #######               \r\n"
"    +:+    +:++:+    +:++:+    +:+   +:+                  #######               \r\n"
"   +#++:++#+ +#+    +:++#++:++#:    +#+                   #######               \r\n"
"  +#+       +#+    +#++#+    +#+   +#+                    #######               \r\n"
" #+#       #+#    #+##+#    #+#   #+#                     #######               \r\n"
"###        ######## ###    ###   ###                      #######               \r\n"
"        :::     :::::::  :::::::  :::::::             |-------------|           \r\n"
"      :+:     :+:   :+::+:   :+::+:   :+:                 |     |               \r\n"
"    +:+ +:+  +:+   +:++:+   +:++:+   +:+                  |  |  |               \r\n"
"  +#+  +:+  +#+   +:++#+   +:++#+   +:+                   |  |  |               \r\n"
"+#+#+#+#+#++#+   +#++#+   +#++#+   +#+                    |  |  |               \r\n"
"     #+#  #+#   #+##+#   #+##+#   #+#                     |  |  |               \r\n"
"    ###   #######  #######  #######                       |  |  |               \r\n"
"                                                          |  |  |               \r\n"
"Based on:    Circlemud beta Patch Level 11 : Jeremy Elson |  |  |               \r\n"
"Written By:  Kevin Lohman / Jaxom - badpirate@gmail.com   |  |  |               \r\n"
"Telnet:      port4k.com port 4000                         |  |  |               \r\n"
"Github:      https://github.com/badpirate/port4kmud       |  |  |               \r\n"
"Web Access:  http://mud.port4k.com/                       \\     /               \r\n"
"Previously:  AAA Stockmud, Daggerfall                      \\   /\r\n"
"                                                            \\ /\r\n"
"Big thanks to all those of you who keep coming back.         V\r\n"
"Character Login: ";

char *WELC_MESSG =
"\r\n"
"I knew you'd be back."
"\r\n\r\n";

char *START_MESSG =
"Welcome!  Type commands for a list of commands and help to get a headstart"
"\r\n\r\n";

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = NO;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;

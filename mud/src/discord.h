/* *************************************************************************
*   File: discord.h				     Addition to CircleMUD *
*  Usage: Discord bridge - emits game events to a local Unix socket for   *
*         a Node.js bot process to relay, and the do_discordrelay command *
*         which lets that bot post text back into the game.               *
************************************************************************* */

void discord_emit(const char *event_type, const char *fmt, ...);

ACMD(do_discordrelay);

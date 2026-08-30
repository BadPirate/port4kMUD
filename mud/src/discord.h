/* *************************************************************************
*   File: discord.h				     Addition to CircleMUD *
*  Usage: Discord bridge - emits game events to a local Unix socket for   *
*         a Node.js bot process to relay, and the do_discord command      *
*         which lets that bot post text back into the game.               *
************************************************************************* */

void discord_emit(const char *event_type, const char *fmt, ...);

/* True if ch is the dedicated relay-bot character (matched by exact name
 * against DISCORD_BOT_MUD_USERNAME) - used to keep the bot's own routine
 * reconnects/idle handling out of both game privilege and Discord noise. */
int discord_is_bot_account(struct char_data *ch);

ACMD(do_discord);

/* *************************************************************************
*   File: discord.c				     Addition to CircleMUD *
*  Usage: Discord bridge - emits game events (logins, channel chat,       *
*         level-ups) to a local Unix socket for a Node.js bot process to  *
*         relay to Discord, and do_discord, which lets that bot post     *
*         Discord chat back into the game as a one-way, bot-only channel. *
*                                                                          *
*  Entirely optional: with no DISCORD_BRIDGE_SOCKET_PATH set, discord_emit *
*  is a silent no-op and nothing about the game changes.                  *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "discord.h"

extern struct descriptor_data *descriptor_list;
extern int is_colour(char code);

/* Strips this codebase's inline "&x" colour codes so plain text goes to
 * Discord instead of raw colour-code source. */
static void strip_mud_colors(const char *in, char *out, size_t outsize)
{
  size_t o = 0;

  while (*in && o + 1 < outsize) {
    if (*in == '&' && *(in + 1) && is_colour(*(in + 1)) != -1) {
      in += 2;
      continue;
    }
    out[o++] = *in++;
  }
  out[o] = '\0';
}

void discord_emit(const char *event_type, const char *fmt, ...)
{
  const char *sock_path = getenv("DISCORD_BRIDGE_SOCKET_PATH");
  char msg[MAX_STRING_LENGTH];
  char line[MAX_STRING_LENGTH];
  char clean[MAX_STRING_LENGTH];
  va_list args;
  struct sockaddr_un addr;
  int sock;
  int flags;
  fd_set write_fds;
  struct timeval timeout;

  if (!sock_path || !*sock_path)
    return;

  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  strip_mud_colors(msg, clean, sizeof(clean));
  snprintf(line, sizeof(line), "%s|%s\n", event_type, clean);

  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return;

  flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    if (errno == EINPROGRESS) {
      FD_ZERO(&write_fds);
      FD_SET(sock, &write_fds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000; /* 100ms - never let a wedged listener stall the game loop */
      if (select(sock + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
        close(sock);
        return;
      }
    } else {
      /* No listener (bot not running) - drop silently, this is best-effort. */
      close(sock);
      return;
    }
  }

  write(sock, line, strlen(line));
  close(sock);
}

int discord_is_bot_account(struct char_data *ch)
{
  char *bot_name = getenv("DISCORD_BOT_MUD_USERNAME");
  return bot_name && *bot_name && !str_cmp(GET_NAME(ch), bot_name);
}

ACMD(do_discord)
{
  struct descriptor_data *i;
  char buf[MAX_STRING_LENGTH];

  if (!discord_is_bot_account(ch)) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  if (!*argument)
    return;

  sprintf(buf, "&c[Discord]&n %s\r\n", argument);

  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i->character &&
        !PLR_FLAGGED(i->character, PLR_WRITING) &&
        !PRF_FLAGGED(i->character, PRF_NODISCORD)) {
      send_to_char(buf, i->character);
    }
  }
}

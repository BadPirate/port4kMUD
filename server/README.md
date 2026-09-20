# Port4kMUD Web Interface

A modern web interface for Port4kMUD, providing browser-based access to the classic text-based multi-user dungeon game through a Next.js application.

## Overview

This project creates a web-based interface for the Port4kMUD game, allowing players to connect through a browser instead of requiring a traditional telnet client. It serves as a bridge between:

- Modern web technologies (Next.js, React, TypeScript)
- The traditional MUD server written in C (running on port 4000)

## Architecture

```
┌─────────────────┐           ┌─────────────────┐           ┌─────────────────┐
│                 │           │                 │           │                 │
│  Web Browser    │◄─────────►│  Next.js Server │◄─────────►│  MUD Server     │
│  (Web Client)   │  WebSocket│  (Socket.IO)    │   TCP     │  (Port 4000)    │
│                 │           │                 │           │                 │
└─────────────────┘           └─────────────────┘           └─────────────────┘
```

- **Web Client**: Browser-based terminal emulator using xterm.js
- **Next.js Server**: Handles HTTP requests and WebSocket connections
- **Socket.IO**: Manages real-time communication between clients and server
- **MUD Server**: The original Port4kMUD running on port 4000

## Features

- **Modern Web Interface**: Accessible from any device with a web browser
- **Terminal Emulation**: Full-featured terminal experience using xterm.js
- **Real-time Communication**: Bidirectional data flow via Socket.IO
- **Responsive Design**: Works on desktop and mobile devices
- **Persistent Connections**: Manages connections between web clients and the MUD server
- **Accounts**: Sign in with an emailed link and keep your characters in one menu

## Accounts and characters

Signing in is optional: the game is connected and playable before anyone is
asked for anything, and a visitor can dismiss the prompt and play as a guest.

That connection is open behind the toast, and the MUD hangs up on anyone left
sitting at `Character Login:`. A telnet client gets 15 to 30 seconds of it; a
player who arrived through this page gets two minutes, because they have a
sign-in toast to read first. The game tells the two apart by the PROXY header
the bridge opens with - see `check_idle_passwords` in `mud/src/comm.c`.

An account is an email address, proved by clicking a link sent to it. There is
no password for the portal itself. Signing in adds a menu under the avatar in
the navbar listing the MUD characters on that account; picking one logs in as
it, and the small cross next to a name forgets it here without touching the
character in the game.

The MUD has no notion of accounts and no out-of-band way to say who is playing:
a character is only ever identified by what is typed at its own prompts. So the
portal watches that conversation. When a login succeeds, the character is saved
to the account; when a password is changed through the game's menu, the saved
one is updated to match; when a character is deleted in the game, it is dropped
from the account. Logging in as a saved character types its name and password
at those same prompts, and stops there - the MOTD and the menu are yours.

Because the bridge has to type the password at the game's prompt, it cannot
hash it. Passwords are encrypted with a key derived from `BETTER_AUTH_SECRET`,
so anyone holding both the database and that secret can read them, and rotating
the secret means every character has to be entered again.

## Prerequisites

- Node.js (v22.18+)
- Yarn package manager
- Port4kMUD server running on port 4000

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/badpirate/port4kMUD.git
   cd port4kMUD/server
   ```

2. Install dependencies:

   ```bash
   yarn install
   ```

3. Set up environment variables:

   ```bash
   cp .env.local.EXAMPLE .env.local
   ```

   Every setting is optional in development. With `SMTP_HOST` unset, sign-in
   links are printed to the server's console instead of being emailed, so a
   local checkout needs no mail server. `BETTER_AUTH_SECRET` is required in
   production and falls back to a development value otherwise. See
   `.env.local.EXAMPLE` for the full list.

4. Make sure the MUD server is running:
   ```bash
   cd ../mud
   ./bin/circle
   ```

## Development

Start the development server:

```bash
yarn dev
```

This will:

1. Check if the MUD server is running
2. Start the Next.js development server
3. Set up Socket.IO for WebSocket connections
4. Connect to the MUD server on port 4000

The application will be available at http://localhost:3000

## Production Build

Build the application for production:

```bash
yarn build
```

Start the production server:

```bash
yarn start
```

## Testing

Run unit tests:

```bash
yarn test
```

Run end-to-end tests:

```bash
yarn test:e2e
```

These build and start a real `bin/circle` and drive a real browser through
sign-in and character creation, so they write test characters into
`mud/lib/etc/players` (gitignored) and use a throwaway accounts database under
`test-results/`.

## Project Structure

- **`pages/`**: Next.js pages
- **`server.ts`**: Custom server: Next.js, Socket.IO, and the portal's own HTTP
  routes. It serves `/api/auth/*` and `/api/portal/*` itself, ahead of Next, so
  there is one auth instance and one database connection in the process
- **`src/server/`**: The per-browser MUD bridge and the portal's HTTP routes
- **`src/utils/mud-login-watcher.ts`**: Reads the MUD's login conversation
- **`src/utils/mud-prompts.ts`**: The game's prompts, verbatim, transcribed from
  its C source. Change these if `mud/src/interpreter.c` changes
- **`src/utils/mud-server.ts`**: Utility for checking MUD server connectivity
- **`src/components/`**: React components
- **`prisma/`**: Account database schema and migrations
- **`styles/`**: CSS and styling
- **`public/`**: Static assets

## Technologies

- **Next.js**: React framework for server-rendered applications
- **Socket.IO**: Real-time bidirectional event-based communication
- **xterm.js**: Terminal emulator for the browser
- **Bootstrap**: Responsive UI components
- **TypeScript**: Type-safe JavaScript

## License

MIT

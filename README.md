# Port4kMUD Project

[![Port4kMUD CI](https://github.com/badpirate/port4kMUD/actions/workflows/ci.yml/badge.svg)](https://github.com/badpirate/port4kMUD/actions/workflows/ci.yml)

> Experience a classic MUD through a modern web interface.

Port4kMUD is a project that modernizes the classic text-based multi-user dungeon (MUD) experience by combining a traditional CircleMUD server with a modern web interface. It provides retro gaming without requiring telnet clients, allowing players to enjoy the MUD from the comfort of their browser.

## Project Structure

This project consists of two main components:

### 1. `mud/` - The MUD Server

The MUD server is based on CircleMUD 3.0 bpl 11, extensively modified over the years since its original creation in 1997 as "AAA Stockmud". It provides the core game mechanics, world, and gameplay experience that players interact with.

- Written in C
- Runs on port 4000 by default
- Includes an extensive feature set beyond the base CircleMUD

See the [MUD README](mud/README.md) for detailed information about the MUD server, its features, and how to build and run it.

### 2. `server/` - The Web Interface

The web interface provides a modern browser-based client for accessing the MUD. It eliminates the need for telnet clients by creating a bridge between web technologies and the traditional MUD server.

- Built with Next.js, React, and TypeScript
- Uses Socket.IO for real-time communication
- Provides terminal emulation through xterm.js
- Includes user authentication and account management

See the [Server README](server/README.md) for detailed information about the web interface, its features, and how to set it up.

## Quick Start

To get the full Port4kMUD experience running locally:

1. **Start the MUD server:**
   ```bash
   cd mud
   ./configure
   cd src && make
   cd ..
   ./bin/circle
   ```

2. **Start the web interface:**
   ```bash
   cd server
   yarn install
   yarn dev
   ```

3. **Access the MUD:**
   Open your browser and navigate to http://localhost:3000

## Features

- **Browser-based MUD Client**: Play directly in your web browser without telnet
- **Classic MUD Experience**: Enjoy the rich text-based game world of Port4kMUD
- **Responsive Design**: Access from desktop or mobile devices
- **Single Host Setup**: Run both components on a single machine
- **Persistent Connections**: Reliable connection management between web clients and MUD server

## Development

This project is organized to allow development of either component independently:

- MUD Server: Standard C development workflow with make-based build system
- Web Interface: Modern JavaScript development with Node.js and Yarn

See the respective READMEs in each directory for detailed development instructions.

## License

- MUD Server: Licensed under CircleMUD license (see [LICENSE.txt](mud/LICENSE.txt))
- Web Interface: MIT License (see [LICENSE](server/LICENSE))

## Credits

Port4kMUD was originally created by Kevin Lohman in 1997 and has been maintained and enhanced over the years. The web interface is a modern addition that brings this classic gaming experience to contemporary platforms.
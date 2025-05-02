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
- **Authentication**: User account management through next-auth
- **Real-time Communication**: Bidirectional data flow via Socket.IO
- **Responsive Design**: Works on desktop and mobile devices
- **Persistent Connections**: Manages connections between web clients and the MUD server

## Prerequisites

- Node.js (v18+)
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
   cp .env.example .env.local
   ```
   Edit `.env.local` to configure your environment.

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

## Docker Support

You can use Docker to run the application:

```bash
# Development
docker-compose -f docker-compose.dev.yml up

# Testing
docker-compose -f docker-compose.test.yml up
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

## Project Structure

- **`pages/`**: Next.js pages and API routes
- **`server.ts`**: Custom server implementation with Socket.IO
- **`utils/mud-server.ts`**: Utility for checking and ensuring MUD server connectivity
- **`prisma/`**: Database schema and migrations
- **`components/`**: React components
- **`styles/`**: CSS and styling
- **`public/`**: Static assets

## Technologies

- **Next.js**: React framework for server-rendered applications
- **Socket.IO**: Real-time bidirectional event-based communication
- **xterm.js**: Terminal emulator for the browser
- **Prisma**: Database toolkit for TypeScript
- **next-auth**: Authentication solution for Next.js
- **Bootstrap**: Responsive UI components
- **TypeScript**: Type-safe JavaScript

## License

MIT
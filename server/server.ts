/* eslint-disable no-restricted-syntax */
import { createServer } from 'http'
import { parse } from 'url'
import { Socket as NetSocket } from 'net'
import next from 'next'
import { Socket, Server as SocketIOServer } from 'socket.io'
import { ensureMudServerRunning } from './src/utils/mud-server'
import { processTelnetData, TelnetEchoState } from './src/utils/telnet'
import discordConfig from './src/utils/discord/config'
import { startDiscordBridge } from './src/utils/discord/bot'

// Types for active connection tracking
interface MudConnection extends TelnetEchoState {
  socket: Socket // Socket.IO socket
  mudConnection: NetSocket
  user?: {
    name: string
    [key: string]: unknown
  }
  buffer?: string
}

const dev = process.env.NODE_ENV !== 'production'
const hostname = process.env.HOSTNAME || 'localhost'
const port = parseInt(process.env.PORT || '3000', 10)

// Initialize Next.js with proper settings
const nextApp = next({
  dev,
  hostname,
  port,
  // This disables the file watching in production
  conf: {
    unstable_skipTrailingSlashRedirect: true,
    unstable_strictNextHead: true,
  },
})
const nextHandler = nextApp.getRequestHandler()

// MUD server configuration
const MUD_HOST = '127.0.0.1' // Only connect locally
const MUD_PORT = 4000 // Default port for Port4kMUD

// Track active connections
const activeConnections = new Map<string, MudConnection>()

// Process user input for proper character handling
function processUserInput(data: string): Buffer {
  // Special handling for backspace/delete
  if (data === '\x7F' || data === '\b') {
    // For CircleMUD, use ASCII 8 (BS)
    return Buffer.from([8])
  }

  // Pass through all other characters as-is
  return Buffer.from(data)
}

// Prepare the application before starting the server
nextApp.prepare().then(async () => {
  // Check if MUD server is running
  await ensureMudServerRunning()

  if (discordConfig.enabled) {
    startDiscordBridge().catch((err) => {
      console.error('Failed to start Discord bridge:', err)
    })
  }

  const server = createServer(async (req, res) => {
    try {
      // Be sure to parse the URL only once per request
      const parsedUrl = parse(req.url || '', true)
      await nextHandler(req, res, parsedUrl)
    } catch (err) {
      console.error('Error occurred handling request:', err)
      res.statusCode = 500
      res.end('Internal Server Error')
    }
  })

  // Initialize Socket.IO
  const io = new SocketIOServer(server, {
    path: '/api/socket',
  })

  // Setup WebSocket namespace for MUD connections
  const mudNamespace = io.of('/mud')

  mudNamespace.on('connection', (socket) => {
    console.log('New socket connection:', socket.id)

    // Create a connection to the MUD server
    const mudConnection = new NetSocket()

    // Store connection information
    activeConnections.set(socket.id, {
      socket,
      mudConnection,
      serverEcho: true, // Default to server echo enabled
      buffer: '',
      telnetBuffer: [], // Buffer for telnet protocol parsing
      onEchoChange: (serverEcho) => socket.emit('echo', serverEcho),
    })

    // Connect to the MUD server
    mudConnection.connect(MUD_PORT, MUD_HOST, () => {
      console.log(`Connected to MUD server for client: ${socket.id}`)
      socket.emit('status', { connected: true })
    })

    // Forward data from MUD to WebSocket client
    mudConnection.on('data', (data: Buffer) => {
      const connection = activeConnections.get(socket.id)

      if (connection) {
        // Process telnet commands and clean the data stream
        const processedData = processTelnetData(connection, data)
        if (processedData) {
          socket.emit('data', processedData)
        }
      }
    })

    // Handle errors on the MUD connection
    mudConnection.on('error', (err: Error) => {
      console.error(`MUD connection error for ${socket.id}:`, err.message)
      socket.emit('error', { message: 'Error connecting to MUD server' })
    })

    // Handle MUD connection closing
    mudConnection.on('close', () => {
      console.log(`MUD connection closed for ${socket.id}`)
      socket.emit('status', { connected: false })

      // Clean up if socket is still active
      const connection = activeConnections.get(socket.id)
      if (connection && connection.socket) {
        connection.socket.disconnect(true)
      }

      activeConnections.delete(socket.id)
    })

    // Forward WebSocket input to the MUD with special character processing
    socket.on('input', (data: string) => {
      const connection = activeConnections.get(socket.id)

      if (connection && connection.mudConnection) {
        // Process user input
        const processedData = processUserInput(data)

        // Send to MUD server
        connection.mudConnection.write(processedData)

        // Handle echo here on the server side if appropriate
        if (!connection.serverEcho) {
          // MUD is not echoing (like during password entry), so we need to handle echoing
          // But we don't want to echo passwords, so just don't echo anything when serverEcho is false
        } else {
          // Normal mode - echo what the user typed
          if (data === '\r' || data === '\n') {
            socket.emit('data', '\r\n')
          } else if (data === '\x7F' || data === '\b') {
            // Visual feedback for backspace
            socket.emit('data', '\b \b')
          } else {
            socket.emit('data', data)
          }
        }
      }
    })

    // Handle WebSocket disconnection
    socket.on('disconnect', () => {
      console.log(`WebSocket disconnected: ${socket.id}`)

      const connection = activeConnections.get(socket.id)
      if (connection && connection.mudConnection) {
        connection.mudConnection.destroy()
      }

      activeConnections.delete(socket.id)
    })
  })

  // Start server
  server.listen(port, () => {
    console.log(
      `> Ready on http://${hostname}:${port} - env ${process.env.NODE_ENV || 'production'}`,
    )
  })
})

/* eslint-disable no-restricted-syntax */
import { createServer } from 'http'
import { parse } from 'url'
import next from 'next'
import { toNodeHandler } from 'better-auth/node'
import { Server as SocketIOServer } from 'socket.io'
import { MudBridge } from './src/server/mud-bridge'
import { handlePortalApi, PORTAL_API_PREFIX, resolveSessionUserId } from './src/server/portal-api'
import { auth } from './src/utils/auth'
import { assertRuntimeSecrets } from './src/utils/config'
import { checkMailerConfiguration } from './src/utils/mailer'
import { ensureMudServerRunning } from './src/utils/mud-server'
import discordConfig from './src/utils/discord/config'
import { startDiscordBridge } from './src/utils/discord/bot'

const dev = process.env.NODE_ENV !== 'production'
const hostname = process.env.HOSTNAME || 'localhost'
const port = parseInt(process.env.PORT || '3000', 10)

// Initialize Next.js with proper settings
// Options beyond these live in next.config.mjs, which `next build` reads too.
const nextApp = next({ dev, hostname, port })
const nextHandler = nextApp.getRequestHandler()

// Everything below here is Better Auth's: sign-in, sessions and magic links.
const AUTH_API_PREFIX = '/api/auth/'

// One bridge per browser socket.
const activeBridges = new Map<string, MudBridge>()

// Fails fast on a production deployment that has not been given a secret,
// before anything can sign a session with a default one.
assertRuntimeSecrets()

// Prepare the application before starting the server
nextApp.prepare().then(async () => {
  // Check if MUD server is running
  await ensureMudServerRunning()
  checkMailerConfiguration()

  if (discordConfig.enabled) {
    startDiscordBridge().catch((err) => {
      console.error('Failed to start Discord bridge:', err)
    })
  }

  // Better Auth brings its own request parsing, so it is mounted directly on
  // the Node server rather than as a Next.js route. That keeps one auth
  // instance and one database connection in the process - the same ones the
  // Socket.IO bridge below uses to identify a browser on its handshake - and
  // keeps the generated Prisma client and better-sqlite3's native module out
  // of the Next.js bundle entirely.
  const authHandler = toNodeHandler(auth)

  const server = createServer(async (req, res) => {
    try {
      // Be sure to parse the URL only once per request
      const parsedUrl = parse(req.url || '', true)
      const pathname = parsedUrl.pathname || ''

      if (pathname.startsWith(AUTH_API_PREFIX)) {
        await authHandler(req, res)
        return
      }

      if (pathname.startsWith(PORTAL_API_PREFIX) && (await handlePortalApi(req, res, pathname))) {
        return
      }

      await nextHandler(req, res, parsedUrl)
    } catch (err) {
      console.error('Error occurred handling request:', err)
      res.statusCode = 500
      res.end('Internal Server Error')
    }
  })

  // Initialize Socket.IO.
  //
  // Nothing may be routable by Next.js at this path. Next's dev server adds its
  // own 'upgrade' listener for HMR, and any upgrade whose URL resolves to a
  // page or API route is closed with socket.end() - which would tear down this
  // namespace's connections the instant they are established.
  const io = new SocketIOServer(server, {
    path: '/api/socket',
  })

  // Setup WebSocket namespace for MUD connections
  const mudNamespace = io.of('/mud')

  // The browser sends its session cookie with the WebSocket upgrade, so who is
  // connecting is known before the MUD connection is opened, and the bridge
  // knows from the start whether there is an account to remember a character
  // for.
  mudNamespace.use(async (socket, nextMiddleware) => {
    socket.data.userId = await resolveSessionUserId(socket.handshake.headers)
    nextMiddleware()
  })

  mudNamespace.on('connection', (socket) => {
    console.log('New socket connection:', socket.id)

    const bridge = new MudBridge(socket, socket.data.userId ?? null, resolveSessionUserId)
    activeBridges.set(socket.id, bridge)
    bridge.connect()

    socket.on('input', (data: string) => bridge.handleInput(data))

    // Logging in as a stored character, and starting a new one, both happen
    // over this socket rather than by opening another: the terminal binds its
    // listeners once, so a replacement socket would leave it attached to a
    // dead one. Only the MUD side of the bridge is replaced.
    socket.on('portal:login-as', (characterId: unknown) => {
      if (typeof characterId !== 'string') return
      bridge.loginAs(characterId).catch((err) => {
        console.error(`Failed to log ${socket.id} in as ${characterId}:`, err)
      })
    })

    socket.on('portal:new-character', () => bridge.newCharacter())

    socket.on('disconnect', () => {
      console.log(`WebSocket disconnected: ${socket.id}`)
      bridge.destroy()
      activeBridges.delete(socket.id)
    })
  })

  // Start server
  server.listen(port, () => {
    console.log(
      `> Ready on http://${hostname}:${port} - env ${process.env.NODE_ENV || 'production'}`,
    )
  })
})

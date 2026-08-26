import { EventEmitter } from 'events'
import { createServer, Server } from 'net'
import { unlinkSync, existsSync } from 'fs'

export interface LoginEvent {
  type: 'login'
  name: string
  level: number
}

export interface LevelUpEvent {
  type: 'levelup'
  name: string
  level: number
}

export interface ChannelEvent {
  type: 'channel'
  channelLabel: string
  name: string
  message: string
}

export type BridgeEvent = LoginEvent | LevelUpEvent | ChannelEvent

// Splits on '|' at most `maxParts - 1` times, so the final field can safely
// contain '|' itself (used for free-text chat messages).
function splitFields(input: string, maxParts: number): string[] {
  const parts: string[] = []
  let rest = input
  while (parts.length < maxParts - 1) {
    const idx = rest.indexOf('|')
    if (idx === -1) break
    parts.push(rest.slice(0, idx))
    rest = rest.slice(idx + 1)
  }
  parts.push(rest)
  return parts
}

function parseLine(line: string): BridgeEvent | null {
  const idx = line.indexOf('|')
  if (idx === -1) return null
  const eventType = line.slice(0, idx)
  const payload = line.slice(idx + 1)

  switch (eventType) {
    case 'login':
    case 'levelup': {
      const [name, levelStr] = splitFields(payload, 2)
      const level = parseInt(levelStr, 10)
      if (!name || Number.isNaN(level)) return null
      return { type: eventType, name, level }
    }
    case 'channel': {
      const [channelLabel, name, message] = splitFields(payload, 3)
      if (!channelLabel || !name || message === undefined) return null
      return { type: 'channel', channelLabel, name, message }
    }
    default:
      return null
  }
}

/**
 * Unix domain socket server that mud/src/discord.c's discord_emit() connects
 * to (once per event, writes one newline-delimited line, closes) to report
 * game events. Emits typed 'event' events for the Discord bot to relay.
 */
export function startBridgeListener(socketPath: string): EventEmitter & { server: Server } {
  const emitter = new EventEmitter() as EventEmitter & { server: Server }

  if (existsSync(socketPath)) {
    // Stale socket file from a previous run (unclean shutdown) - safe to remove,
    // nothing on the C side holds this path open between events.
    unlinkSync(socketPath)
  }

  const server = createServer((socket) => {
    let buffer = ''
    socket.on('data', (chunk) => {
      buffer += chunk.toString('utf8')
      let newlineIdx: number
      // eslint-disable-next-line no-cond-assign
      while ((newlineIdx = buffer.indexOf('\n')) !== -1) {
        const line = buffer.slice(0, newlineIdx)
        buffer = buffer.slice(newlineIdx + 1)
        const event = parseLine(line)
        if (event) {
          emitter.emit('event', event)
        }
      }
    })
    socket.on('error', () => {
      // Best-effort relay - a client that disconnects mid-write is not fatal.
    })
  })

  server.on('error', (err) => {
    console.error('Discord bridge listener error:', err)
  })

  server.listen(socketPath, () => {
    console.log(`Discord bridge listening on ${socketPath}`)
  })

  emitter.server = server
  return emitter
}

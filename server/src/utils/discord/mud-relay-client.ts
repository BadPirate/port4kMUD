import { Socket } from 'net'
import { EventEmitter } from 'events'
import { MUD_CONFIG } from '../mud-server'
import { processTelnetData, TelnetEchoState } from '../telnet'

const LOGIN_TIMEOUT_MS = 10000
const RECONNECT_DELAY_MS = 5000
const MAX_RELAY_LENGTH = 200 // stay well under the MUD's MAX_INPUT_LENGTH (256)

type LoginState =
  | 'connecting'
  | 'awaiting-password-prompt'
  | 'awaiting-post-password'
  | 'awaiting-menu-prompt'
  | 'playing'

/**
 * Maintains one persistent connection to the MUD, logged in as a dedicated
 * bot character, so Discord messages can be relayed in via the `discordrelay`
 * command. Connects exactly like a real telnet client (mirrors server.ts's
 * per-browser bridge), since the MUD has no other way to accept input.
 */
export class MudRelayClient extends EventEmitter {
  private socket: Socket | null = null

  private telnetState: TelnetEchoState = { serverEcho: true, telnetBuffer: [] }

  private textBuffer = ''

  private loginState: LoginState = 'connecting'

  private loginTimeout: ReturnType<typeof setTimeout> | null = null

  private stopped = false

  constructor(
    private readonly username: string,
    private readonly password: string,
  ) {
    super()
  }

  start(): void {
    this.stopped = false
    this.connect()
  }

  stop(): void {
    this.stopped = true
    if (this.loginTimeout) clearTimeout(this.loginTimeout)
    this.socket?.destroy()
  }

  send(text: string): void {
    if (this.loginState !== 'playing' || !this.socket) return
    // A pasted multi-line Discord message must not become multiple injected
    // MUD commands, and must stay under the MUD's per-line input cap.
    const singleLine = text
      .replace(/[\r\n]+/g, ' ')
      .trim()
      .slice(0, MAX_RELAY_LENGTH)
    if (!singleLine) return
    this.socket.write(`discordrelay ${singleLine}\r\n`)
  }

  private connect(): void {
    this.textBuffer = ''
    this.telnetState = { serverEcho: true, telnetBuffer: [] }
    this.loginState = 'connecting'

    const socket = new Socket()
    this.socket = socket

    this.loginTimeout = setTimeout(() => {
      console.error('Discord relay: MUD login timed out, reconnecting')
      socket.destroy()
    }, LOGIN_TIMEOUT_MS)

    socket.connect(MUD_CONFIG.port, MUD_CONFIG.host, () => {
      socket.write(`${this.username}\r\n`)
      this.loginState = 'awaiting-password-prompt'
    })

    socket.on('data', (data: Buffer) => {
      const text = processTelnetData(this.telnetState, data)
      this.textBuffer += text
      this.advanceLogin()
    })

    socket.on('error', (err) => {
      console.error('Discord relay MUD connection error:', err.message)
    })

    socket.on('close', () => {
      if (this.loginTimeout) clearTimeout(this.loginTimeout)
      this.socket = null
      const wasPlaying = this.loginState === 'playing'
      this.loginState = 'connecting'
      if (wasPlaying) this.emit('disconnected')
      if (!this.stopped) {
        setTimeout(() => this.connect(), RECONNECT_DELAY_MS)
      }
    })
  }

  private advanceLogin(): void {
    if (this.loginState === 'awaiting-password-prompt') {
      if (this.textBuffer.includes('Password:')) {
        this.textBuffer = ''
        this.socket?.write(`${this.password}\r\n`)
        this.loginState = 'awaiting-post-password'
      }
      return
    }

    if (this.loginState === 'awaiting-post-password') {
      // A dropped/stale prior session skips straight to CON_PLAYING - detect
      // that branch first so we don't wait forever for a menu that never comes.
      if (this.textBuffer.includes('Reconnecting') || this.textBuffer.includes('already in use')) {
        this.finishLogin()
        return
      }
      if (this.textBuffer.includes('*** PRESS RETURN')) {
        this.textBuffer = ''
        this.socket?.write('\r\n')
        this.loginState = 'awaiting-menu-prompt'
      }
      return
    }

    if (this.loginState === 'awaiting-menu-prompt') {
      if (this.textBuffer.includes('Your Choice?')) {
        this.textBuffer = ''
        this.socket?.write('1\r\n')
        this.finishLogin()
      }
    }
  }

  private finishLogin(): void {
    if (this.loginTimeout) clearTimeout(this.loginTimeout)
    this.loginTimeout = null
    this.loginState = 'playing'
    this.textBuffer = ''
    this.emit('ready')
  }
}

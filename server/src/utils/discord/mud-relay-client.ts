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
 * bot character, so Discord messages can be relayed in via the `discord`
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

  private lastSendAt = 0

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
    if (this.loginState !== 'playing' || !this.socket) {
      console.warn(
        `Discord relay: dropping message, not in the game (login state: ${this.loginState})`,
      )
      return
    }
    // A pasted multi-line Discord message must not become multiple injected
    // MUD commands, and must stay under the MUD's per-line input cap.
    const singleLine = text
      .replace(/[\r\n]+/g, ' ')
      .trim()
      .slice(0, MAX_RELAY_LENGTH)
    if (!singleLine) return
    this.socket.write(`discord ${singleLine}\r\n`)
    this.lastSendAt = Date.now()
    console.log(`Discord relay -> MUD: ${singleLine}`)
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
      if (this.loginState === 'playing') {
        // Once in the game the bot hears every global channel, so nothing may
        // accumulate in textBuffer - it only exists to match login prompts.
        this.checkRelayRejected(text)
        return
      }
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
      // The MUD only sends "Password:" for an EXISTING character - an unknown
      // name gets a "Did I get that right ... (Y/N)?" new-character prompt
      // instead (see interpreter.c's CON_GET_NAME), which this bot can't and
      // shouldn't try to answer (that's full character creation). Fail loudly
      // instead of silently waiting out the login timeout on every attempt.
      if (this.textBuffer.includes('Did I get that right')) {
        console.error(
          `Discord relay: MUD character '${this.username}' does not exist - ` +
            'create it in-game first (DISCORD_BOT_MUD_USERNAME/PASSWORD), then restart the bridge',
        )
        this.socket?.destroy()
        return
      }
      if (this.textBuffer.includes('Password:')) {
        this.textBuffer = ''
        this.socket?.write(`${this.password}\r\n`)
        this.loginState = 'awaiting-post-password'
      }
      return
    }

    if (this.loginState === 'awaiting-post-password') {
      // Fail loudly and stop instead of retrying forever on every 5s
      // reconnect - each attempt is a fresh connection, so the MUD's own
      // 3-strikes lockout never kicks in, and every retry both spams its
      // mudlog and increments the persisted bad-password count on the
      // character record.
      if (this.textBuffer.includes('Wrong password')) {
        console.error(
          `Discord relay: wrong password for MUD character '${this.username}' - ` +
            'check DISCORD_BOT_MUD_PASSWORD, then restart the bridge',
        )
        this.socket?.destroy()
        return
      }
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

  /**
   * `discord` is always a valid command, so a "Huh?!?" arriving right after we
   * relayed one means do_discord refused us: DISCORD_BOT_MUD_USERNAME in the
   * MUD's own environment doesn't match this character's name. That check is a
   * getenv inside the MUD process (see discord_is_bot_account in
   * mud/src/discord.c), so it can disagree with ours and otherwise fails in
   * complete silence - every relayed message simply vanishes.
   */
  private checkRelayRejected(text: string): void {
    if (Date.now() - this.lastSendAt > 2000 || !text.includes('Huh?!?')) return
    this.lastSendAt = 0
    console.error(
      `Discord relay: the MUD refused the 'discord' command. Set ` +
        `DISCORD_BOT_MUD_USERNAME to '${this.username}' in the environment ` +
        `bin/circle runs under, matching the character name exactly.`,
    )
  }

  private finishLogin(): void {
    if (this.loginTimeout) clearTimeout(this.loginTimeout)
    this.loginTimeout = null
    this.loginState = 'playing'
    this.textBuffer = ''
    this.emit('ready')
  }
}

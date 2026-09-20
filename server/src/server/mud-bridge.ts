import { Socket as NetSocket } from 'net'
import type { IncomingHttpHeaders } from 'http'
import type { Socket } from 'socket.io'
import {
  CharacterLimitReached,
  getCharacterCredentials,
  removeCharacterByName,
  saveCharacter,
  updateCharacterPassword,
} from '../utils/characters'
import { MudInputLineAssembler } from '../utils/mud-input-lines'
import {
  MudLoginWatcher,
  type LoginWatcherEvent,
  type PromptKind,
} from '../utils/mud-login-watcher'
import { MUD_CONFIG } from '../utils/mud-server'
import type { BridgeMode, PortalEvent } from '../utils/portal-types'
import { processTelnetData, type TelnetEchoState } from '../utils/telnet'

/** Resolves the account this socket belongs to, checked again at write time. */
export type SessionResolver = (headers: IncomingHttpHeaders) => Promise<string | null>

interface AutoLogin {
  characterId: string
  name: string
  password: string
  /** Set once the name has been typed, so it is never typed twice. */
  sentName: boolean
  sentPassword: boolean
}

/**
 * One browser's connection to the MUD.
 *
 * Without a session this is a plain pipe: bytes from the MUD go to the
 * terminal, keystrokes go back, and nothing is watched. That is what every
 * visitor gets, and it is unchanged from before accounts existed.
 *
 * With a session the same pipe also feeds a MudLoginWatcher, so a character
 * the player logs into can be remembered on their account, and a stored
 * character can be logged into by typing its name and password at the MUD's
 * own prompts - the only way in, since the MUD has no other authentication
 * path.
 */
export class MudBridge {
  private mud: NetSocket | null = null

  private telnet: TelnetEchoState = { serverEcho: true, telnetBuffer: [] }

  private watcher: MudLoginWatcher | null = null

  private assembler = new MudInputLineAssembler()

  private mode: BridgeMode = 'observe'

  private autoLogin: AutoLogin | null = null

  /**
   * The PROXY header has to be the first line the MUD sees, so keystrokes
   * that arrive before the connection opens wait here rather than racing
   * ahead of it.
   */
  private mudReady = false

  private pendingInput: Buffer[] = []

  /**
   * Keystrokes held while the bridge is typing a stored character's name and
   * password. An early Enter would answer the name prompt with an empty line,
   * which the MUD treats as a hang-up.
   */
  private heldInput: string[] = []

  private holdingInput = false

  /** Set while replacing the MUD connection, so its close is not a surprise. */
  private replacing = false

  /**
   * Account writes still in flight. The MUD hangs up immediately after a
   * character is deleted, and the browser's socket goes with it, so the close
   * has to wait for these or the player never hears what happened.
   */
  private pendingWork: Promise<unknown> = Promise.resolve()

  private destroyed = false

  constructor(
    private readonly socket: Socket,
    private readonly userId: string | null,
    private readonly resolveSession: SessionResolver,
  ) {}

  /** Opens the MUD connection, optionally logging straight in as a character. */
  connect(autoLogin?: AutoLogin): void {
    if (this.destroyed) return

    this.mud = new NetSocket()
    this.telnet = { serverEcho: true, telnetBuffer: [], onEchoChange: (echo) => this.onEcho(echo) }
    this.assembler = new MudInputLineAssembler()
    this.mudReady = false
    this.pendingInput = []
    this.autoLogin = autoLogin ?? null
    this.holdingInput = Boolean(autoLogin)
    this.heldInput = []

    // Without a session there is nobody to remember a character for, so the
    // login conversation is not watched at all.
    this.watcher = this.userId ? new MudLoginWatcher((event) => this.onWatcherEvent(event)) : null
    this.mode = this.userId ? (autoLogin ? 'auto-login' : 'capture') : 'observe'

    const mud = this.mud

    mud.connect(MUD_CONFIG.port, MUD_CONFIG.host, () => {
      const header = this.proxyHeader()
      mud.write(header)
      console.log(`Connected to MUD server for client: ${this.socket.id} as ${header.trim()}`)

      this.mudReady = true
      this.pendingInput.forEach((chunk) => mud.write(chunk))
      this.pendingInput = []

      this.socket.emit('status', { connected: true })
      this.emitPortal({ type: 'mode', mode: this.mode, characterId: autoLogin?.characterId })
    })

    mud.on('data', (data: Buffer) => {
      const text = processTelnetData(this.telnet, data)
      if (!text) return
      this.socket.emit('data', text)
      this.watcher?.onOutput(text)
    })

    mud.on('error', (err: Error) => {
      console.error(`MUD connection error for ${this.socket.id}:`, err.message)
      this.socket.emit('error', { message: 'Error connecting to MUD server' })
    })

    mud.on('close', () => {
      this.watcher?.onClose()
      if (this.replacing) return

      console.log(`MUD connection closed for ${this.socket.id}`)
      void this.pendingWork.then(() => {
        if (this.destroyed) return
        this.socket.emit('status', { connected: false })
        this.socket.disconnect(true)
      })
    })
  }

  /** Forwards a keystroke from the browser, echoing it back where needed. */
  handleInput(data: string): void {
    if (this.destroyed) return

    if (this.holdingInput) {
      this.heldInput.push(data)
      return
    }

    this.write(data)
  }

  /** Logs in as one of the account's stored characters. */
  async loginAs(characterId: string): Promise<void> {
    const userId = await this.currentUserId()
    if (!userId) {
      this.emitPortal({ type: 'login-failed', reason: 'not-signed-in' })
      return
    }

    const credentials = await getCharacterCredentials(userId, characterId)
    if (!credentials) {
      this.emitPortal({ type: 'login-failed', reason: 'unknown-character' })
      return
    }

    const autoLogin: AutoLogin = {
      characterId,
      name: credentials.name,
      password: credentials.password,
      sentName: false,
      sentPassword: false,
    }

    // A connection still sitting at an untouched name prompt can be used as
    // it is. Anything further along - mid-login, or already in the game - has
    // no way back to the name prompt, so it is replaced.
    if (this.watcher?.isAtFreshNamePrompt) {
      this.autoLogin = autoLogin
      this.mode = 'auto-login'
      this.holdingInput = true
      this.emitPortal({ type: 'mode', mode: this.mode, characterId })
      this.typeName()
      return
    }

    this.reconnect(autoLogin)
  }

  /** Starts a fresh MUD connection so another character can be created. */
  newCharacter(): void {
    if (this.watcher?.isAtFreshNamePrompt) return
    this.reconnect()
  }

  destroy(): void {
    this.destroyed = true
    this.replacing = true
    this.watcher?.onClose()
    this.mud?.destroy()
    this.mud = null
  }

  /** Replaces the MUD connection, keeping the browser's socket as it is. */
  private reconnect(autoLogin?: AutoLogin): void {
    this.emitPortal({ type: 'reconnecting', characterId: autoLogin?.characterId })
    this.replacing = true
    this.watcher?.onClose()
    this.mud?.destroy()
    this.mud = null
    this.replacing = false
    this.connect(autoLogin)
  }

  /** Sends text to the MUD as though it had been typed in the browser. */
  private write(data: string): void {
    const mud = this.mud
    if (!mud) return

    // CircleMUD's process_input expects ASCII 8 for a backspace.
    const payload = data === '\x7f' || data === '\b' ? Buffer.from([8]) : Buffer.from(data)

    if (this.mudReady) {
      mud.write(payload)
    } else {
      this.pendingInput.push(payload)
    }

    this.assembler.push(data).forEach((line) => this.watcher?.onLine(line))
    this.echo(data)
  }

  /**
   * Echoes a keystroke back to the terminal. The MUD does not echo for this
   * bridge, and must not be echoed at all while it has asked for a password
   * (IAC WILL ECHO, tracked by processTelnetData).
   */
  private echo(data: string): void {
    if (!this.telnet.serverEcho) return

    if (data === '\r' || data === '\n') {
      this.socket.emit('data', '\r\n')
    } else if (data === '\x7f' || data === '\b') {
      this.socket.emit('data', '\b \b')
    } else {
      this.socket.emit('data', data)
    }
  }

  private onEcho(serverEcho: boolean): void {
    this.socket.emit('echo', serverEcho)
  }

  private onWatcherEvent(event: LoginWatcherEvent): void {
    switch (event.type) {
      case 'prompt':
        this.onPrompt(event.kind)
        break
      case 'login':
        this.track(this.onLogin(event.name, event.password))
        break
      case 'password-changed':
        this.track(this.onPasswordChanged(event.name, event.password))
        break
      case 'character-deleted':
        this.track(this.onCharacterDeleted(event.name))
        break
      case 'login-failed':
        this.onLoginFailed(event.reason, event.name)
        break
      default:
        break
    }
  }

  private onPrompt(kind: PromptKind): void {
    const pending = this.autoLogin
    if (!pending) return

    if (kind === 'name' && !pending.sentName) {
      this.typeName()
      return
    }

    if (kind === 'password' && pending.sentName && !pending.sentPassword) {
      pending.sentPassword = true
      this.write(`${pending.password}\r\n`)
      return
    }

    if (kind === 'name-confirm') {
      // The MUD offered to create this name, which means it does not have it
      // any more - deleted in game, or lost with the player file. The player
      // is left at the prompt to decide what to do.
      this.emitPortal({
        type: 'character-missing',
        name: pending.name,
        characterId: pending.characterId,
      })
      this.finishAutoLogin()
    }
  }

  private typeName(): void {
    const pending = this.autoLogin
    if (!pending) return
    pending.sentName = true
    this.write(`${pending.name}\r\n`)
  }

  /** Stops driving the login and lets the player's keystrokes through again. */
  private finishAutoLogin(): void {
    this.autoLogin = null
    this.mode = this.userId ? 'capture' : 'observe'
    if (!this.holdingInput) return

    this.holdingInput = false
    const held = this.heldInput
    this.heldInput = []
    held.forEach((data) => this.write(data))
  }

  private async onLogin(name: string, password: string): Promise<void> {
    this.finishAutoLogin()

    // Re-checked rather than trusted from the handshake: signing out deletes
    // the session, and a socket opened before that must not still be able to
    // write to the account.
    const userId = await this.currentUserId()
    if (!userId) return

    try {
      const { character, isNew } = await saveCharacter(userId, name, password)
      this.emitPortal({ type: 'character-saved', character, isNew })
    } catch (err) {
      if (err instanceof CharacterLimitReached) {
        this.emitPortal({ type: 'character-limit-reached', name, limit: err.limit })
        return
      }
      console.error(`Failed to save character ${name} for ${userId}:`, err)
    }
  }

  private async onPasswordChanged(name: string, password: string): Promise<void> {
    const userId = await this.currentUserId()
    if (!userId) return

    const updated = await updateCharacterPassword(userId, name, password)
    if (updated) {
      this.emitPortal({ type: 'password-updated', name: updated.name })
    }
  }

  private async onCharacterDeleted(name: string): Promise<void> {
    const userId = await this.currentUserId()
    if (!userId) return

    if (await removeCharacterByName(userId, name)) {
      this.emitPortal({ type: 'character-forgotten', name })
    }
  }

  private onLoginFailed(reason: string, name?: string): void {
    const pending = this.autoLogin

    if (pending && reason === 'wrong-password') {
      // The password was changed in the game, or from another client. Never
      // retry: each attempt is counted against the character in its player
      // file, and three of them disconnect.
      this.emitPortal({
        type: 'stored-password-rejected',
        name: pending.name,
        characterId: pending.characterId,
      })
      this.finishAutoLogin()
      return
    }

    if (pending) this.finishAutoLogin()
    this.emitPortal({ type: 'login-failed', reason, name })
  }

  /** Keeps an account write in the queue the MUD's close waits on. */
  private track(work: Promise<void>): void {
    this.pendingWork = this.pendingWork.then(
      () => work,
      () => work,
    )
    void this.pendingWork.catch((err) => {
      console.error(`Portal bookkeeping failed for ${this.socket.id}:`, err)
    })
  }

  private async currentUserId(): Promise<string | null> {
    return this.resolveSession(this.socket.handshake.headers)
  }

  private emitPortal(event: PortalEvent): void {
    this.socket.emit('portal', event)
  }

  /**
   * Builds the PROXY protocol v1 header naming the browser this bridge is
   * relaying for. Without it the MUD only ever sees our loopback address,
   * which puts every web player on one host for site bans, multiplay
   * counting, the mort/immort check and the who list. The MUD honours this
   * only from a loopback peer and only as the first line of a connection -
   * see proxy_set_host in mud/src/comm.c.
   */
  private proxyHeader(): string {
    const forwarded = this.socket.handshake.headers['x-forwarded-for']
    const firstHop = (Array.isArray(forwarded) ? forwarded[0] : forwarded)?.split(',')[0]?.trim()
    // Node reports IPv4 peers on a dual-stack listener as ::ffff:1.2.3.4
    const client = (firstHop || this.socket.handshake.address || '').replace(/^::ffff:/i, '')

    if (/^\d{1,3}(\.\d{1,3}){3}$/.test(client)) {
      return `PROXY TCP4 ${client} ${MUD_CONFIG.host} 0 ${MUD_CONFIG.port}\r\n`
    }
    if (client.includes(':')) {
      return `PROXY TCP6 ${client} ::1 0 ${MUD_CONFIG.port}\r\n`
    }
    // Nothing usable - tell the MUD to keep the address it saw us connect from.
    return 'PROXY UNKNOWN\r\n'
  }
}

import {
  CHANGE_PASSWORD_DONE,
  CHANGE_PASSWORD_WRONG,
  CHARACTER_DELETED,
  ENTERED_GAME,
  FATAL_MESSAGES,
  ILLEGAL_PASSWORD,
  INVALID_NAME,
  type LoginFailureReason,
  NEW_CHARACTER,
  PASSWORDS_DONT_MATCH,
  PRESS_RETURN,
  PROMPT_CHANGED_PASSWORD,
  PROMPT_CHARACTER_LOGIN,
  PROMPT_DELETE_PASSWORD,
  PROMPT_GENDER,
  PROMPT_MENU_CHOICE,
  PROMPT_NAME,
  PROMPT_NAME_AGAIN,
  PROMPT_NAME_CONFIRM,
  PROMPT_NEW_PASSWORD,
  PROMPT_OLD_PASSWORD,
  PROMPT_PASSWORD,
  PROMPT_RETYPE_PASSWORD,
  RECONNECT_MESSAGES,
  WRONG_PASSWORD,
  capitalizeName,
} from './mud-prompts'

/** What the MUD is currently waiting for the player to type. */
export type PromptKind =
  | 'name'
  | 'name-confirm'
  | 'password'
  | 'new-password'
  | 'retype-password'
  | 'press-return'
  | 'menu'
  | 'old-password'
  | 'changed-password'
  | 'delete-password'

export type LoginWatcherEvent =
  /** A character was accepted: the name and password are known good. */
  | { type: 'login'; name: string; password: string; isNew: boolean }
  | { type: 'login-failed'; reason: LoginFailureReason; name?: string }
  /** The player changed their password through menu option 4. */
  | { type: 'password-changed'; name: string; password: string }
  /** The player deleted their character through menu option 5. */
  | { type: 'character-deleted'; name: string }
  /** The MUD is now waiting for input of this kind. */
  | { type: 'prompt'; kind: PromptKind }

/**
 * Only the tail of the MUD's output can hold a prompt, and only a little of it
 * is needed to span a prompt split across TCP segments. Keeping the window
 * small also stops the greeting banner, the MOTD and the menu art from
 * carrying a stale match forward.
 */
const OUTPUT_WINDOW = 2048

/**
 * Watches one MUD connection's login conversation and reports which character
 * the player ended up as.
 *
 * It is fed the MUD's output (after telnet negotiation has been stripped) and
 * the lines the player typed, and matches them against the verbatim prompts in
 * mud-prompts.ts. The MUD speaks no GMCP or MSDP and has no other out-of-band
 * identity channel, so this is the only way the bridge can know that a name
 * and password were accepted.
 *
 * Three properties of nanny() (mud/src/interpreter.c) shape the design:
 *
 *  - it consumes exactly one queued line per prompt, in order, so someone who
 *    types ahead ("Bob<Enter>secret<Enter>" before the password prompt has
 *    arrived) is still answering the prompts in sequence. Lines are therefore
 *    queued and handed to whichever prompt is outstanding, rather than to
 *    whatever prompt happened to be on screen when the keystroke arrived.
 *
 *  - a new character is only written to disk once its rolled stats are kept,
 *    immediately before the MOTD. Nothing is reported before that point, so
 *    abandoning creation half way leaves nothing behind to remember.
 *
 *  - the menu can be reached again at any time by quitting, and reaching it
 *    proves every earlier prompt has been answered. The menu and the MOTD's
 *    "press return" are therefore treated as synchronisation points that
 *    discard anything still queued, which keeps a branch this watcher does
 *    not model (the history pager, say) from shifting every later line by one.
 */
export class MudLoginWatcher {
  private readonly emit: (event: LoginWatcherEvent) => void

  /** Rolling tail of MUD output, for matching prompts across chunks. */
  private output = ''

  /** Lines typed but not yet claimed by a prompt. */
  private lines: string[] = []

  /** The prompt awaiting an answer, if any. */
  private expecting: PromptKind | null = null

  private name = ''

  private password = ''

  /** Set while the new-character path is in progress. */
  private creating = false

  /** Set once the retyped password matched and character creation began. */
  private created = false

  /** Set while a password typed at the password prompt is unconfirmed. */
  private awaitingLogin = false

  /** Set while menu option 4 is in progress. */
  private changingPassword = false

  /**
   * Recoverable rejections are reported once per attempt. The MUD's message
   * and the prompt that follows it can arrive in separate reads, and both
   * reads see the message in the rolling window.
   */
  private reportedWrongPassword = false

  private reportedNameRejection = false

  /** Set once this connection can produce nothing further. */
  private finished = false

  /** Set once the player is in the game; only a return to the menu matters. */
  private playing = false

  constructor(emit: (event: LoginWatcherEvent) => void) {
    this.emit = emit
  }

  /** The prompt currently awaiting an answer, or null. */
  get awaitingPrompt(): PromptKind | null {
    return this.expecting
  }

  /** True when the MUD wants a name and nothing has been typed towards one. */
  get isAtFreshNamePrompt(): boolean {
    return this.expecting === 'name' && this.lines.length === 0
  }

  /** The name this connection is logging in as, once one has been typed. */
  get characterName(): string {
    return this.name
  }

  /** Feeds MUD output that has had telnet sequences stripped from it. */
  onOutput(text: string): void {
    if (!text || this.finished) return

    this.output = (this.output + text).slice(-OUTPUT_WINDOW)

    if (this.playing) {
      // In the game the output is the player's, not ours. The one thing still
      // worth noticing is a return to the menu, which `quit` does.
      if (this.output.endsWith(PROMPT_MENU_CHOICE)) {
        this.playing = false
        this.synchronize('menu')
      }
      return
    }

    if (this.handleFatal()) return
    if (this.handleReconnect()) return
    if (this.handleDeletion()) return
    if (this.handleCompletion()) return
    this.handlePasswordChange()
    this.handleEnteredGame()
    if (this.playing || this.finished) return
    this.handlePrompts()
  }

  /** Feeds one line the player typed, as the MUD will receive it. */
  onLine(line: string): void {
    if (this.finished) return

    // '!' repeats the previous line and '^old^new' rewrites it (process_input
    // in mud/src/comm.c). What nanny() then acts on is not what was typed, so
    // nothing this connection types afterwards can be trusted.
    if (line.startsWith('!') || line.startsWith('^')) {
      this.abandon('input-not-captured')
      return
    }

    this.lines.push(line)
    this.drain()
  }

  /** Called when the MUD connection closes. */
  onClose(): void {
    this.finished = true
  }

  private setPrompt(kind: PromptKind): void {
    // The same prompt arriving again is the same question, not a new one.
    if (this.expecting !== kind) {
      this.expecting = kind
      this.emit({ type: 'prompt', kind })
    }
    this.output = ''
    this.drain()
  }

  /**
   * Announces a prompt that everything typed earlier has already been
   * answered past, discarding the queue so a branch this watcher does not
   * model cannot leave it one line out of step.
   */
  private synchronize(kind: PromptKind): void {
    this.lines = []
    this.expecting = null
    this.setPrompt(kind)
  }

  /** Hands the oldest unclaimed line to the outstanding prompt. */
  private drain(): void {
    while (this.expecting && this.lines.length > 0) {
      const kind = this.expecting
      const line = this.lines.shift() as string
      this.expecting = null
      this.apply(kind, line)
    }
  }

  private apply(kind: PromptKind, line: string): void {
    switch (kind) {
      case 'name':
        // _parse_name skips leading whitespace before validating the name.
        this.name = capitalizeName(line.replace(/^\s+/, ''))
        this.creating = false
        this.created = false
        this.awaitingLogin = false
        this.reportedNameRejection = false
        break
      case 'password':
        this.password = line
        this.awaitingLogin = true
        this.reportedWrongPassword = false
        break
      case 'new-password':
      case 'changed-password':
        this.password = line
        break
      default:
        // 'name-confirm', 'retype-password', 'press-return', 'menu',
        // 'old-password' and 'delete-password' carry nothing worth keeping:
        // what matters is the MUD's reply, which onOutput handles.
        break
    }
  }

  private handleFatal(): boolean {
    for (const { match, reason } of FATAL_MESSAGES) {
      if (this.output.includes(match)) {
        this.finished = true
        this.emit({ type: 'login-failed', reason, name: this.name || undefined })
        return true
      }
    }

    if (this.awaitingLogin && !this.reportedWrongPassword && this.output.includes(WRONG_PASSWORD)) {
      // Recoverable: the MUD re-prompts, allowing three attempts in total.
      this.awaitingLogin = false
      this.reportedWrongPassword = true
      this.password = ''
      this.emit({ type: 'login-failed', reason: 'wrong-password', name: this.name || undefined })
    }

    if (!this.reportedNameRejection && this.output.includes(INVALID_NAME)) {
      this.reportedNameRejection = true
      this.name = ''
      this.emit({ type: 'login-failed', reason: 'name-rejected' })
    }

    return false
  }

  private handleReconnect(): boolean {
    if (!this.awaitingLogin) return false
    if (!RECONNECT_MESSAGES.some((message) => this.output.includes(message))) return false

    // perform_dupe_check put the player straight back into their body: no
    // MOTD, no menu, already in the game.
    this.awaitingLogin = false
    this.playing = true
    this.expecting = null
    this.lines = []
    this.emit({ type: 'login', name: this.name, password: this.password, isNew: false })
    this.output = ''
    return true
  }

  private handleCompletion(): boolean {
    if (!this.output.includes(PRESS_RETURN)) return false

    if (this.awaitingLogin) {
      this.awaitingLogin = false
      this.emit({ type: 'login', name: this.name, password: this.password, isNew: false })
    } else if (this.created) {
      // create_entry and save_char have run by now (interpreter.c
      // CON_QROLLSTATS), so the character exists on disk and is safe to keep.
      this.created = false
      this.creating = false
      this.emit({ type: 'login', name: this.name, password: this.password, isNew: true })
    }

    this.synchronize('press-return')
    return true
  }

  private handleDeletion(): boolean {
    const deleted = CHARACTER_DELETED.exec(this.output)
    if (!deleted) return false

    this.finished = true
    this.emit({ type: 'character-deleted', name: deleted[1] })
    return true
  }

  private handlePasswordChange(): void {
    if (!this.changingPassword) return

    if (this.output.includes(CHANGE_PASSWORD_WRONG)) {
      this.changingPassword = false
      return
    }
    if (this.output.includes(CHANGE_PASSWORD_DONE) && this.password) {
      this.changingPassword = false
      this.emit({ type: 'password-changed', name: this.name, password: this.password })
    }
  }

  private handleEnteredGame(): void {
    if (!this.output.includes(ENTERED_GAME)) return
    this.playing = true
    this.expecting = null
    this.lines = []
    this.output = ''
  }

  private handlePrompts(): void {
    const out = this.output

    // Most specific first: several of these end in ": " and one ends with the
    // character's own name, which can be anything the MUD accepts.
    if (out.includes(NEW_CHARACTER) && out.includes(PROMPT_NEW_PASSWORD) && out.endsWith(': ')) {
      this.creating = true
      this.setPrompt('new-password')
      return
    }

    if (out.endsWith(PROMPT_RETYPE_PASSWORD)) {
      this.setPrompt('retype-password')
      return
    }

    if (out.endsWith(PROMPT_OLD_PASSWORD)) {
      this.changingPassword = true
      this.setPrompt('old-password')
      return
    }

    if (out.endsWith(PROMPT_CHANGED_PASSWORD)) {
      this.setPrompt('changed-password')
      return
    }

    if (out.endsWith(PROMPT_DELETE_PASSWORD)) {
      this.setPrompt('delete-password')
      return
    }

    const confirm = PROMPT_NAME_CONFIRM.exec(out)
    if (confirm) {
      // Both a brand new name and reviving a deleted character land here.
      this.name = confirm[1]
      this.creating = true
      this.setPrompt('name-confirm')
      return
    }

    if (
      out.endsWith(PROMPT_CHARACTER_LOGIN) ||
      out.endsWith(PROMPT_NAME_AGAIN) ||
      (out.includes(INVALID_NAME) && out.endsWith(PROMPT_NAME))
    ) {
      this.creating = false
      this.created = false
      this.awaitingLogin = false
      this.setPrompt('name')
      return
    }

    if (out.endsWith(PROMPT_PASSWORD)) {
      // Creation re-prompts with the same bare "Password: " after a rejected
      // or mistyped password, so the surrounding state decides what it means.
      const retry = out.includes(ILLEGAL_PASSWORD) || out.includes(PASSWORDS_DONT_MATCH)
      this.setPrompt(this.creating || retry ? 'new-password' : 'password')
      return
    }

    if (out.includes(PROMPT_GENDER)) {
      // The retyped password matched, so from here the MUD is building a
      // character and will write it out once the stats are kept.
      this.created = true
      this.output = ''
      return
    }

    if (out.endsWith(PROMPT_MENU_CHOICE)) {
      this.synchronize('menu')
    }
  }

  /** Stops capturing on this connection without ending the session. */
  private abandon(reason: LoginFailureReason): void {
    this.lines = []
    this.expecting = null
    this.name = ''
    this.password = ''
    this.creating = false
    this.created = false
    this.awaitingLogin = false
    this.changingPassword = false
    this.playing = true
    this.emit({ type: 'login-failed', reason })
  }
}

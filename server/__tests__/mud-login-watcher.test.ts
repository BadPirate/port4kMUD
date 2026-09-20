import { MudInputLineAssembler } from '../src/utils/mud-input-lines'
import { MudLoginWatcher, type LoginWatcherEvent } from '../src/utils/mud-login-watcher'

/**
 * Every string here was captured from a real bin/circle over TCP, after
 * server/src/utils/telnet.ts had stripped the negotiation, so the watcher is
 * tested against what the MUD actually sends rather than a paraphrase of it.
 */
const MUD = {
  loginPrompt: 'Character Login: ',
  passwordPrompt: 'Password: ',
  wrongPassword: '\r\nWrong password.\r\nPassword: ',
  tooManyPasswords: '\r\nWrong password... disconnecting.\r\n',
  invalidName: 'Invalid name, please try another.\r\nName: ',
  confirmName: (name: string) => `Did I get that right, ${name} (Y/N)? `,
  nameAgain: 'Okay, what IS it, then? ',
  newPassword: (name: string) => `New character.\r\nGive me a password for ${name}: `,
  illegalPassword: '\r\nIllegal password.\r\nPassword: ',
  retypePassword: '\r\nPlease retype password: ',
  passwordMismatch: "\r\nPasswords don't match... start over.\r\nPassword: ",
  gender: '\r\nWhat is your gender (M/F/N)? ',
  race: '\r\nSelect a race:\r\n  1) Human\tNo Modifiers\r\n  2) Dwarf\t+2 Con +1 Str\r\n\r\nRace: ',
  class: '\r\nSelect a class:\r\n  [C]leric\r\n  [T]hief\r\n  [W]arrior\r\n\r\nClass: ',
  rollStats: '\r\nPress enter to roll your stats.',
  keepStats:
    '\r\nStr: [17/0] Int: [7] Wis: [11] Dex: [13] Con: [11] Cha: [7]\r\n\r\nKeep these stats? (y/N)',
  motdAndPressReturn:
    '\r\n[ 06/15/01 ] \u001b[0;31mBUILDERS PORT\u001b[0;0m running at daggerfall.circlemud.net 5000\r\n' +
    '[ 03/28/17 ] Holy shit, 16 years?\r\n\r\n\n*** PRESS RETURN: ',
  menu:
    '\r\n**************************************************\r\n' +
    ' *            WELCOME BACK TO port4k.com 4000     *\r\n' +
    '      *  0.  LEAVE THE GAME                            *\r\n' +
    '       *  1.  ENTER PORT 4000                           *\r\n' +
    '        *  3.  READ THE HISTORY OF PORT 4000             *\r\n' +
    '         *  4.  CHANGE YOUR CHARACTERS PASSWORD           *\r\n' +
    '          *  5.  DELETE YOUR CHARACTER, *COMPLETELY*       *\r\n\r\nYour Choice? ',
  enteredGame:
    "\r\nI knew you'd be back.\r\n\r\n\u001b[36mThe Conference Room\u001b[0m\r\n" +
    '   Upon entering this you can hear faint talking.\r\n\u001b[0;0mDaggerfall: > ',
  takeOverBody: '\r\nYou take over your own body, already in use!\r\n\r\n\u001b[0;0mDaggerfall: > ',
  reconnecting: '\r\nReconnecting.\r\n\u001b[0;0mDaggerfall: > ',
  oldPassword: '\r\nEnter your old password: ',
  changedPassword: '\r\nEnter a new password: ',
  changeDone: '\r\n\r\n\r\nDone.\n\r',
  changeWrong: '\r\n\r\nIncorrect password.\r\n\r\n',
  deletePassword: '\r\nEnter your password for verification: ',
  deleteConfirm:
    '\r\n\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n' +
    'ARE YOU ABSOLUTELY SURE?\r\n\r\nPlease type "yes" to confirm: ',
  deleted: (name: string) => `Character '${name}' deleted!\r\nGoodbye.\r\n`,
  multiplay: 'Sorry, you are only allowed 3 chars from same IP',
  wizlock: 'The game is temporarily restricted.. try again later.\r\n',
}

/** Drives a watcher and records what it emitted. */
class Session {
  readonly events: LoginWatcherEvent[] = []

  readonly watcher = new MudLoginWatcher((event) => this.events.push(event))

  private readonly assembler = new MudInputLineAssembler()

  /** Feeds MUD output, optionally split into fixed-size chunks. */
  out(text: string, chunkSize?: number): this {
    if (!chunkSize) {
      this.watcher.onOutput(text)
      return this
    }
    for (let i = 0; i < text.length; i += chunkSize) {
      this.watcher.onOutput(text.slice(i, i + chunkSize))
    }
    return this
  }

  /** Types characters exactly as the browser sends them, one per keystroke. */
  type(text: string): this {
    for (const char of text) {
      this.assembler.push(char).forEach((line) => this.watcher.onLine(line))
    }
    return this
  }

  /** Types a whole line and presses Enter. */
  line(text: string): this {
    return this.type(`${text}\r`)
  }

  get logins() {
    return this.events.filter((event) => event.type === 'login')
  }

  get failures() {
    return this.events.filter((event) => event.type === 'login-failed')
  }

  prompts() {
    return this.events.filter((event) => event.type === 'prompt').map((event) => event.kind)
  }
}

/** Plays an existing character's login as far as the MOTD. */
function loginExisting(session: Session, name = 'Zephyrtest', password = 'sekrit1') {
  return session
    .out(MUD.loginPrompt)
    .line(name)
    .out(MUD.passwordPrompt)
    .line(password)
    .out(MUD.motdAndPressReturn)
}

/**
 * Plays a new character's creation as far as the MOTD. The MUD capitalises the
 * name before echoing it back, which is the spelling the player file keeps.
 */
function createCharacter(session: Session, name = 'Zephyrtest', password = 'sekrit1') {
  const stored = name.charAt(0).toUpperCase() + name.slice(1)
  return session
    .out(MUD.loginPrompt)
    .line(name)
    .out(MUD.confirmName(stored))
    .line('Y')
    .out(MUD.newPassword(stored))
    .line(password)
    .out(MUD.retypePassword)
    .line(password)
    .out(MUD.gender)
    .line('M')
    .out(MUD.race)
    .line('1')
    .out(MUD.class)
    .line('w')
    .out(MUD.rollStats)
    .line('')
    .out(MUD.keepStats)
    .line('y')
    .out(MUD.motdAndPressReturn)
}

describe('MudLoginWatcher', () => {
  describe('logging in as an existing character', () => {
    it('reports the name and password once the MOTD is reached', () => {
      const session = loginExisting(new Session())

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it('capitalises the name the way the MUD does', () => {
      const session = loginExisting(new Session(), 'zephyrtest')

      expect(session.logins[0]).toMatchObject({ name: 'Zephyrtest' })
    })

    it('ignores leading whitespace, as _parse_name does', () => {
      const session = loginExisting(new Session(), '   zephyrtest')

      expect(session.logins[0]).toMatchObject({ name: 'Zephyrtest' })
    })

    it('matches prompts split across reads', () => {
      const session = new Session()
      session.out(MUD.loginPrompt, 3).line('Zephyrtest')
      session.out('Pass').out('word: ').line('sekrit1')
      session.out(MUD.motdAndPressReturn, 7)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it('answers prompts in order when the player types ahead', () => {
      // nanny() consumes one queued line per prompt, so a fast typist whose
      // name and password both arrive before the password prompt is still
      // answering the prompts in sequence.
      const session = new Session()
      session.out(MUD.loginPrompt)
      session.line('Zephyrtest').line('sekrit1')
      session.out(MUD.passwordPrompt).out(MUD.motdAndPressReturn)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it('reports a rejected password and then the retry that works', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('nope')
      session.out(MUD.wrongPassword).line('sekrit1')
      session.out(MUD.motdAndPressReturn)

      expect(session.failures).toEqual([
        { type: 'login-failed', reason: 'wrong-password', name: 'Zephyrtest' },
      ])
      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it('reports a rejected password only once when the reply is split', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('nope')
      session.out('\r\nWrong password.\r\n').out('Password: ')

      expect(session.failures).toHaveLength(1)
    })

    it('reports the third failed attempt as fatal and stops', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('nope')
      session.out(MUD.tooManyPasswords)
      session.out(MUD.loginPrompt)

      expect(session.failures).toEqual([
        { type: 'login-failed', reason: 'too-many-bad-passwords', name: 'Zephyrtest' },
      ])
      expect(session.prompts()).toEqual(['name', 'password'])
    })

    it.each([
      ['takeOverBody', MUD.takeOverBody],
      ['reconnecting', MUD.reconnecting],
    ])('reports a login when a duplicate session is taken over (%s)', (_label, message) => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('sekrit1')
      session.out(message)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it.each([
      ['multiplay limit', MUD.multiplay, 'multiplay-limit'],
      ['wizlock', MUD.wizlock, 'wizlock'],
    ])('reports %s as a failure and never a login', (_label, message, reason) => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('sekrit1')
      session.out(message)

      expect(session.failures).toEqual([{ type: 'login-failed', reason, name: 'Zephyrtest' }])
      expect(session.logins).toHaveLength(0)
    })

    it('reports a rejected name and accepts the next one', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('x1y2')
      session.out(MUD.invalidName).line('Ab')
      session.out(MUD.confirmName('Ab'))

      expect(session.failures).toEqual([{ type: 'login-failed', reason: 'name-rejected' }])
      expect(session.prompts()).toEqual(['name', 'name', 'name-confirm'])
    })
  })

  describe('creating a new character', () => {
    it('reports the character only once its stats have been kept', () => {
      const session = createCharacter(new Session())

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: true },
      ])
    })

    it('reports nothing if creation is abandoned before the stats are kept', () => {
      // Nothing is written to the player file until CON_QROLLSTATS accepts the
      // roll, so there would be no character to log into later.
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.confirmName('Zephyrtest')).line('Y')
      session.out(MUD.newPassword('Zephyrtest')).line('sekrit1')
      session.out(MUD.retypePassword).line('sekrit1')
      session.out(MUD.gender).line('M')
      session.out(MUD.race)
      session.watcher.onClose()

      expect(session.logins).toHaveLength(0)
    })

    it('keeps the password from the retry after an illegal one', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Quixotical')
      session.out(MUD.confirmName('Quixotical')).line('Y')
      session.out(MUD.newPassword('Quixotical')).line('ab')
      session.out(MUD.illegalPassword).line('goodpass')
      session.out(MUD.retypePassword).line('goodpass')
      session.out(MUD.gender).line('M')
      session.out(MUD.race).line('1')
      session.out(MUD.class).line('w')
      session.out(MUD.rollStats).line('')
      session.out(MUD.keepStats).line('y')
      session.out(MUD.motdAndPressReturn)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Quixotical', password: 'goodpass', isNew: true },
      ])
    })

    it('keeps the password from the restart after a mistyped retype', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Quixotical')
      session.out(MUD.confirmName('Quixotical')).line('Y')
      session.out(MUD.newPassword('Quixotical')).line('goodpass')
      session.out(MUD.retypePassword).line('different')
      session.out(MUD.passwordMismatch).line('secondtry')
      session.out(MUD.retypePassword).line('secondtry')
      session.out(MUD.gender).line('M')
      session.out(MUD.race).line('1')
      session.out(MUD.class).line('w')
      session.out(MUD.rollStats).line('')
      session.out(MUD.keepStats).line('y')
      session.out(MUD.motdAndPressReturn)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Quixotical', password: 'secondtry', isNew: true },
      ])
    })

    it('goes back to the name prompt when the confirmation is declined', () => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Quixotical')
      session.out(MUD.confirmName('Quixotical')).line('n')
      session.out(MUD.nameAgain).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line('sekrit1')
      session.out(MUD.motdAndPressReturn)

      expect(session.logins).toEqual([
        { type: 'login', name: 'Zephyrtest', password: 'sekrit1', isNew: false },
      ])
    })

    it('takes the name from the confirmation prompt, not from what was typed', () => {
      // The MUD capitalises the name it echoes back, and it is that spelling
      // the player file will carry.
      const session = createCharacter(new Session(), 'zephyrtest')

      expect(session.logins[0]).toMatchObject({ name: 'Zephyrtest' })
    })
  })

  describe('after reaching the menu', () => {
    it('reports a password changed through menu option 4', () => {
      const session = loginExisting(new Session())
      session.line('').out(MUD.menu).line('4')
      session.out(MUD.oldPassword).line('sekrit1')
      session.out(MUD.changedPassword).line('newpass9')
      session.out(MUD.retypePassword).line('newpass9')
      session.out(MUD.changeDone + MUD.menu)

      expect(session.events).toContainEqual({
        type: 'password-changed',
        name: 'Zephyrtest',
        password: 'newpass9',
      })
    })

    it('reports nothing when the old password was wrong', () => {
      const session = loginExisting(new Session())
      session.line('').out(MUD.menu).line('4')
      session.out(MUD.oldPassword).line('bogus')
      session.out(MUD.changeWrong + MUD.menu)

      expect(session.events.some((event) => event.type === 'password-changed')).toBe(false)
    })

    it('reports a character deleted through menu option 5', () => {
      const session = loginExisting(new Session(), 'Deleteme', 'delpass1')
      session.line('').out(MUD.menu).line('5')
      session.out(MUD.deletePassword).line('delpass1')
      session.out(MUD.deleteConfirm).line('yes')
      session.out(MUD.deleted('Deleteme'))

      expect(session.events).toContainEqual({ type: 'character-deleted', name: 'Deleteme' })
    })

    it('stops reading prompts out of game output once playing', () => {
      const session = loginExisting(new Session())
      session.line('').out(MUD.menu).line('1')
      session.out(MUD.enteredGame)
      const before = session.events.length

      // A player saying something that looks like a prompt must not be
      // mistaken for one.
      session.out("Someone says, 'Password: '\r\n")
      session.line('hello')

      expect(session.events).toHaveLength(before)
    })

    it('picks the menu up again after quitting back to it', () => {
      const session = loginExisting(new Session())
      session.line('').out(MUD.menu).line('1')
      session.out(MUD.enteredGame).line('quit')
      session.out(MUD.menu).line('4')
      session.out(MUD.oldPassword).line('sekrit1')
      session.out(MUD.changedPassword).line('newpass9')
      session.out(MUD.retypePassword).line('newpass9')
      session.out(MUD.changeDone + MUD.menu)

      expect(session.events).toContainEqual({
        type: 'password-changed',
        name: 'Zephyrtest',
        password: 'newpass9',
      })
    })

    it('stays in step when an unmodelled branch swallows a line', () => {
      // Menu option 3 pages the history and any key returns to the menu, a
      // round trip this watcher does not model. The menu is a synchronisation
      // point, so the stray keypress must not shift every later line by one.
      const session = loginExisting(new Session())
      session.line('').out(MUD.menu).line('3')
      session.out('The history of Port 4000...\r\n').line('')
      session.out(MUD.menu).line('4')
      session.out(MUD.oldPassword).line('sekrit1')
      session.out(MUD.changedPassword).line('newpass9')
      session.out(MUD.retypePassword).line('newpass9')
      session.out(MUD.changeDone + MUD.menu)

      expect(session.events).toContainEqual({
        type: 'password-changed',
        name: 'Zephyrtest',
        password: 'newpass9',
      })
    })
  })

  describe('input the MUD rewrites', () => {
    it.each([
      ['history repeat', '!'],
      ['substitution', '^old^new'],
    ])('stops capturing after %s', (_label, typed) => {
      const session = new Session()
      session.out(MUD.loginPrompt).line('Zephyrtest')
      session.out(MUD.passwordPrompt).line(typed)
      session.out(MUD.motdAndPressReturn)

      expect(session.failures).toEqual([{ type: 'login-failed', reason: 'input-not-captured' }])
      expect(session.logins).toHaveLength(0)
    })
  })

  describe('prompt reporting', () => {
    it('announces each prompt once, in order', () => {
      const session = createCharacter(new Session())

      expect(session.prompts()).toEqual([
        'name',
        'name-confirm',
        'new-password',
        'retype-password',
        'press-return',
      ])
    })

    it('knows when the MUD is waiting for a name and nothing is queued', () => {
      const session = new Session()
      session.out(MUD.loginPrompt)
      expect(session.watcher.isAtFreshNamePrompt).toBe(true)

      session.type('Zep')
      expect(session.watcher.isAtFreshNamePrompt).toBe(true)

      session.line('hyrtest')
      expect(session.watcher.isAtFreshNamePrompt).toBe(false)
      expect(session.watcher.characterName).toBe('Zephyrtest')
    })

    it('ignores everything once the connection has closed', () => {
      const session = loginExisting(new Session())
      session.watcher.onClose()
      const before = session.events.length

      session.out(MUD.menu).line('4')

      expect(session.events).toHaveLength(before)
    })
  })
})

/**
 * Verbatim strings the MUD's login state machine sends, and the shapes of the
 * lines it accepts back.
 *
 * Everything here is transcribed from the C source and must stay in step with
 * it: nanny() in mud/src/interpreter.c, the MENU/GREETINGS/WELC_MESSG text in
 * mud/src/config.c, and process_input/echo_off in mud/src/comm.c. The web
 * bridge has no out-of-band way to learn who is logging in - the MUD speaks no
 * GMCP or MSDP - so watching for these strings is the only way to know which
 * character a browser ended up playing.
 */

/** Trailing line of GREETINGS (mud/src/config.c) - the initial name prompt. */
export const PROMPT_CHARACTER_LOGIN = 'Character Login: '

/** Re-prompt after a rejected name (interpreter.c CON_GET_NAME). */
export const PROMPT_NAME = 'Name: '

/** Answer of "no" to the name confirmation (interpreter.c CON_NAME_CNFRM). */
export const PROMPT_NAME_AGAIN = 'Okay, what IS it, then? '

/** Existing character's password prompt; also re-prompted on a bad password. */
export const PROMPT_PASSWORD = 'Password: '

/** New character confirmation. The name is echoed back capitalised. */
export const NAME_CONFIRM_TEXT = 'Did I get that right'
export const PROMPT_NAME_CONFIRM = /Did I get that right, (\S+) \(Y\/N\)\? $/

/** Sent once the confirmation is accepted (interpreter.c CON_NAME_CNFRM). */
export const NEW_CHARACTER = 'New character.'

/** WELC_MESSG (mud/src/config.c) - sent when menu option 1 enters the game. */
export const ENTERED_GAME = "I knew you'd be back."

/** First password prompt for a brand new character. */
export const PROMPT_NEW_PASSWORD = 'Give me a password for '

/** Second password prompt, for both creation and the menu's change-password. */
export const PROMPT_RETYPE_PASSWORD = 'Please retype password: '

/** Password confirmed; character creation proper begins (CON_QSEX). */
export const PROMPT_GENDER = 'What is your gender (M/F/N)? '

/** Shown after the MOTD, for both a successful login and a new character. */
export const PRESS_RETURN = '*** PRESS RETURN'

/** Final line of MENU (mud/src/config.c). */
export const PROMPT_MENU_CHOICE = 'Your Choice? '

/** Menu option 4, change password (interpreter.c CON_CHPWD_GETOLD). */
export const PROMPT_OLD_PASSWORD = 'Enter your old password: '
export const PROMPT_CHANGED_PASSWORD = 'Enter a new password: '
export const CHANGE_PASSWORD_DONE = 'Done.'
export const CHANGE_PASSWORD_WRONG = 'Incorrect password.'

/** Menu option 5, delete character (interpreter.c CON_DELCNF1/CON_DELCNF2). */
export const PROMPT_DELETE_PASSWORD = 'Enter your password for verification: '
export const CHARACTER_DELETED = /Character '([^']+)' deleted!/
export const CHARACTER_NOT_DELETED = 'Character not deleted.'

/** Rejected name, any of the five reasons, all share one message. */
export const INVALID_NAME = 'Invalid name, please try another.'

/** Bad password, with one retry left (interpreter.c CON_PASSWORD). */
export const WRONG_PASSWORD = 'Wrong password.'

/** Password rules violated during creation (interpreter.c CON_NEWPASSWD). */
export const ILLEGAL_PASSWORD = 'Illegal password.'

/** Retyped password did not match (interpreter.c CON_CNFPASSWD). */
export const PASSWORDS_DONT_MATCH = "Passwords don't match"

/**
 * perform_dupe_check (interpreter.c) can put a returning player straight into
 * the game, skipping the MOTD and the menu entirely.
 */
export const RECONNECT_MESSAGES = [
  'Reconnecting.',
  'You take over your own body, already in use!',
  'Reconnecting to unswitched char.',
] as const

/**
 * Every way the MUD can refuse a login outright. Each one ends the connection,
 * so a watcher that sees one will never see a success.
 */
export const FATAL_MESSAGES = [
  { match: 'Wrong password... disconnecting.', reason: 'too-many-bad-passwords' },
  { match: 'you are only allowed', reason: 'multiplay-limit' },
  { match: 'mort and immort on at same time', reason: 'mort-immort' },
  { match: 'has not been cleared for login from your site', reason: 'site-restricted' },
  { match: 'The game is temporarily restricted', reason: 'wizlock' },
  { match: 'Timed out... goodbye.', reason: 'timed-out' },
  { match: 'new characters are not allowed from your site', reason: 'new-characters-banned' },
  { match: "new players can't be created at the moment", reason: 'creation-restricted' },
  { match: 'Sorry, this site is banned.', reason: 'site-banned' },
  { match: 'is full right now', reason: 'game-full' },
] as const

export type LoginFailureReason =
  | (typeof FATAL_MESSAGES)[number]['reason']
  | 'wrong-password'
  | 'name-rejected'
  | 'input-not-captured'

/**
 * Name rules enforced by _parse_name and the checks around it in
 * interpreter.c's CON_GET_NAME: letters only, two to MAX_NAME_LENGTH of them.
 */
export const MAX_NAME_LENGTH = 20
export const MIN_NAME_LENGTH = 2
export const VALID_NAME = /^[A-Za-z]{2,20}$/

/** MAX_PWD_LENGTH in mud/src/structs.h, and the minimum nanny enforces. */
export const MAX_PASSWORD_LENGTH = 10
export const MIN_PASSWORD_LENGTH = 3

/** MAX_INPUT_LENGTH in mud/src/structs.h; process_input truncates past this. */
export const MAX_INPUT_LENGTH = 256

/**
 * Capitalises a name the way the MUD's CAP() macro does: the first character
 * only, leaving the rest as typed.
 */
export function capitalizeName(name: string): string {
  if (!name) return name
  return name.charAt(0).toUpperCase() + name.slice(1)
}

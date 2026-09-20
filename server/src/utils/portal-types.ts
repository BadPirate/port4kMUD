/**
 * Types shared between the bridge and the browser.
 *
 * Deliberately free of imports: the browser bundle picks these up, and must
 * not drag in Prisma or anything else the server keeps to itself.
 */

export interface PortalCharacter {
  id: string
  name: string
  lastLoginAt: string | null
}

export interface PortalAccount {
  email: string
  image: string | null
  characters: PortalCharacter[]
  max: number
}

/** How the bridge is treating this connection. */
export type BridgeMode =
  /** Nobody is signed in: pipe bytes and watch nothing. */
  | 'observe'
  /** Signed in: watch the login so a character can be remembered. */
  | 'capture'
  /** Signed in and logging in as a stored character. */
  | 'auto-login'

/**
 * What the bridge tells the browser about the account, over the same socket
 * as the game itself. Everything here is a consequence of what happened at
 * the MUD's prompts, which only the server sees.
 */
export type PortalEvent =
  | { type: 'mode'; mode: BridgeMode; characterId?: string }
  /** The MUD connection is being replaced, so the terminal should be cleared. */
  | { type: 'reconnecting'; characterId?: string }
  /** A character was logged into and is now remembered on the account. */
  | { type: 'character-saved'; character: PortalCharacter; isNew: boolean }
  /** A stored password was refreshed after being changed inside the game. */
  | { type: 'password-updated'; name: string }
  /** The character was deleted inside the game, so it is forgotten here too. */
  | { type: 'character-forgotten'; name: string }
  /** The stored password no longer works; the player has to type it again. */
  | { type: 'stored-password-rejected'; name: string; characterId: string }
  /** The MUD does not know this character any more. */
  | { type: 'character-missing'; name: string; characterId: string }
  /** The account is full, so a new character cannot be remembered. */
  | { type: 'character-limit-reached'; name: string; limit: number }
  /** The MUD refused the login. */
  | { type: 'login-failed'; reason: string; name?: string }

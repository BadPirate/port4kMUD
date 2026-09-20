import { createCipheriv, createDecipheriv, randomBytes, scryptSync } from 'crypto'
import config from './config'

/**
 * Reversible encryption for the MUD passwords the portal keeps on an account's
 * behalf.
 *
 * These cannot be hashed the way a password normally would be. The bridge has
 * to type the plaintext at the MUD's own "Password: " prompt, because the game
 * does its own crypt(3) comparison against its player file and accepts nothing
 * else. So the portal holds them the only way it can: encrypted at rest with a
 * key derived from BETTER_AUTH_SECRET, and decrypted only in the moment a
 * character is logged in.
 *
 * Consequences worth knowing: anyone who holds both the database file and
 * BETTER_AUTH_SECRET can read these passwords, and rotating that secret makes
 * every stored password unreadable, so each character has to be entered again.
 *
 * AES-256-GCM, a random 96-bit nonce per value, and the ciphertext's own
 * authentication tag, so a tampered value fails to decrypt rather than
 * decrypting to something else.
 */
const VERSION = 'v1'
const KEY_LENGTH = 32
const NONCE_LENGTH = 12

/**
 * A fixed salt is correct here: this derives one key from one high-entropy
 * application secret, so there is no set of low-entropy secrets for a shared
 * salt to let an attacker attack in parallel.
 */
const KEY_SALT = 'port4k-mud-character-secret-v1'

let cachedKey: Buffer | null = null

function key(): Buffer {
  if (!cachedKey) {
    cachedKey = scryptSync(config.BETTER_AUTH_SECRET, KEY_SALT, KEY_LENGTH)
  }
  return cachedKey
}

export function encryptSecret(plaintext: string): string {
  const nonce = randomBytes(NONCE_LENGTH)
  const cipher = createCipheriv('aes-256-gcm', key(), nonce)
  const ciphertext = Buffer.concat([cipher.update(plaintext, 'utf8'), cipher.final()])

  return [
    VERSION,
    nonce.toString('base64url'),
    cipher.getAuthTag().toString('base64url'),
    ciphertext.toString('base64url'),
  ].join('.')
}

export function decryptSecret(encrypted: string): string {
  const [version, nonce, tag, ciphertext] = encrypted.split('.')
  if (version !== VERSION || !nonce || !tag || !ciphertext) {
    throw new Error('Stored character password is not in a format this version understands.')
  }

  const decipher = createDecipheriv('aes-256-gcm', key(), Buffer.from(nonce, 'base64url'))
  decipher.setAuthTag(Buffer.from(tag, 'base64url'))

  return Buffer.concat([
    decipher.update(Buffer.from(ciphertext, 'base64url')),
    decipher.final(),
  ]).toString('utf8')
}

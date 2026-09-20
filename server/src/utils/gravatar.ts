import { createHash } from 'crypto'

/**
 * The Gravatar image for an email address.
 *
 * Computed on the server and stored on the user record, so the browser never
 * has to hash the address itself - SubtleCrypto is unavailable over plain
 * http, which is how the portal is reached on a LAN.
 */
export function gravatarUrl(email: string, size = 64): string {
  const hash = createHash('sha256').update(email.trim().toLowerCase()).digest('hex')
  return `https://gravatar.com/avatar/${hash}?d=identicon&s=${size}`
}

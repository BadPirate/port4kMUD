/**
 * @jest-environment node
 */
import { decryptSecret, encryptSecret } from '../src/utils/secret-box'

describe('secret box', () => {
  it('gives back what was encrypted', () => {
    expect(decryptSecret(encryptSecret('sekrit1'))).toBe('sekrit1')
  })

  it('handles the whole range of passwords the MUD accepts', () => {
    // MAX_PWD_LENGTH is 10 and nanny accepts any printable line.
    for (const password of ['abc', 'a b c', '$dollar$', '~!@#%^&*()', 'ünïcødé']) {
      expect(decryptSecret(encryptSecret(password))).toBe(password)
    }
  })

  it('produces a different ciphertext every time', () => {
    // A fresh nonce per value, so two characters sharing a password are not
    // visibly identical in the database.
    expect(encryptSecret('sekrit1')).not.toBe(encryptSecret('sekrit1'))
  })

  it('does not leave the password readable', () => {
    expect(encryptSecret('sekrit1')).not.toContain('sekrit1')
  })

  it('refuses a tampered value rather than decrypting it to something else', () => {
    const encrypted = encryptSecret('sekrit1')
    const parts = encrypted.split('.')
    const flipped = Buffer.from(parts[3], 'base64url')
    flipped[0] ^= 0xff
    parts[3] = flipped.toString('base64url')

    expect(() => decryptSecret(parts.join('.'))).toThrow()
  })

  it('refuses a value that is not in its format', () => {
    expect(() => decryptSecret('sekrit1')).toThrow(/format/)
    expect(() => decryptSecret('v2.a.b.c')).toThrow(/format/)
  })
})

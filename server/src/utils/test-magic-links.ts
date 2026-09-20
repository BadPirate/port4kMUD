/**
 * Magic links captured instead of emailed, so the end-to-end tests can follow
 * one without an SMTP server.
 *
 * Only ever written when NODE_ENV is 'test' and SMTP_HOST is unset, and only
 * readable through /api/portal/test/magic-links, which answers 404 outside a
 * test run. A module-level store rather than a global: the custom server owns
 * both the mailer and that route, so they share this module.
 */
export interface CapturedMagicLink {
  identifier: string
  url: string
  capturedAt: string
}

const captured: CapturedMagicLink[] = []

export function captureMagicLink(identifier: string, url: string): void {
  captured.push({ identifier, url, capturedAt: new Date().toISOString() })
}

export function listCapturedMagicLinks(): CapturedMagicLink[] {
  return [...captured]
}

export function clearCapturedMagicLinks(): void {
  captured.length = 0
}

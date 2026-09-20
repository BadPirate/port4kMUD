import nodemailer, { type Transporter } from 'nodemailer'
import config from './config'
import { captureMagicLink } from './test-magic-links'

/**
 * Outbound mail for magic links.
 *
 * SMTP_HOST decides whether mail is really sent. Without it the portal still
 * works, which is what makes a local checkout and the end-to-end tests
 * possible: development prints the link, tests keep it in memory for the
 * test-only route to hand back, and production refuses to sign anyone in
 * rather than pretending a mail went out.
 */
let transporter: Transporter | null = null

function getTransporter(): Transporter | null {
  if (!config.SMTP_HOST) return null
  if (!transporter) {
    transporter = nodemailer.createTransport({
      host: config.SMTP_HOST,
      port: config.SMTP_PORT,
      secure: config.SMTP_SECURE,
      auth: config.SMTP_USER ? { user: config.SMTP_USER, pass: config.SMTP_PASS } : undefined,
    })
  }
  return transporter
}

/** Warns at boot rather than at the first sign-in attempt. */
export function checkMailerConfiguration(): void {
  if (config.SMTP_HOST) return
  if (config.NODE_ENV === 'production') {
    console.warn(
      'SMTP_HOST is not set: magic link emails cannot be sent, so nobody can sign in. ' +
        'Set SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASS and SMTP_FROM.',
    )
  } else {
    console.log('SMTP_HOST is not set: magic links will be logged here instead of emailed.')
  }
}

export async function sendMagicLinkEmail(email: string, url: string): Promise<void> {
  const mailer = getTransporter()

  if (!mailer) {
    if (config.NODE_ENV === 'test') {
      // The end-to-end tests read these back through /api/portal/test/magic-links.
      captureMagicLink(email, url)
      return
    }
    if (config.NODE_ENV === 'production') {
      throw new Error('Cannot send a magic link: SMTP_HOST is not configured.')
    }
    console.log(`\nMagic link for ${email}:\n  ${url}\n`)
    return
  }

  await mailer.sendMail({
    from: config.SMTP_FROM,
    to: email,
    subject: `Sign in to ${config.NEXT_PUBLIC_APP_NAME}`,
    text: [
      'Use this link to sign in. It can only be used once, and expires shortly.',
      '',
      url,
      '',
      'If you did not ask to sign in, you can ignore this message.',
    ].join('\n'),
  })
}

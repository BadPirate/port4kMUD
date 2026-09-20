import dotenv from 'dotenv'
// Import package.json directly
// eslint-disable-next-line import/extensions
import packageJson from '../../package.json'

// Only load dotenv in server environment
if (typeof window === 'undefined') {
  dotenv.config()
}

/** Reads a positive integer setting, falling back when unset or nonsense. */
function parseCount(value: string | undefined, fallback: number): number {
  const parsed = parseInt(value || '', 10)
  return Number.isFinite(parsed) && parsed > 0 ? parsed : fallback
}

const required = {
  NODE_ENV: process.env.NODE_ENV,
  HOSTNAME: process.env.HOSTNAME || 'localhost',
}

const optional = {
  PORT: process.env.PORT,
  CI: process.env.CI,

  // Next.js specific environment variables
  NEXT_PUBLIC_APP_NAME: process.env.NEXT_PUBLIC_APP_NAME || packageJson.name,
  NEXT_PUBLIC_APP_VERSION: process.env.NEXT_PUBLIC_APP_VERSION || packageJson.version,

  // Discord bridge - all optional, the bridge stays off with DISCORD_BOT_TOKEN unset.
  // DISCORD_BOT_MUD_USERNAME and DISCORD_BRIDGE_SOCKET_PATH must also be set in the
  // real process/container environment bin/circle runs under (see mud/src/discord.c) -
  // this file's dotenv loading only reaches the Node process.
  DISCORD_BOT_TOKEN: process.env.DISCORD_BOT_TOKEN,
  DISCORD_CHANNEL_ID: process.env.DISCORD_CHANNEL_ID,
  DISCORD_BRIDGE_SOCKET_PATH: process.env.DISCORD_BRIDGE_SOCKET_PATH || '/tmp/port4k-discord.sock',
  DISCORD_BOT_MUD_USERNAME: process.env.DISCORD_BOT_MUD_USERNAME,
  DISCORD_BOT_MUD_PASSWORD: process.env.DISCORD_BOT_MUD_PASSWORD,

  // Portal accounts. BETTER_AUTH_SECRET signs sessions and encrypts the MUD
  // passwords the bridge replays, so changing it signs everyone out and makes
  // every stored character password unreadable.
  BETTER_AUTH_SECRET: process.env.BETTER_AUTH_SECRET,
  // Only needed when the public URL differs from the request's own host, as
  // it does behind a proxy. Better Auth derives it from the request otherwise.
  BETTER_AUTH_URL: process.env.BETTER_AUTH_URL,
  AUTH_DATABASE_PATH: process.env.AUTH_DATABASE_PATH,
  MAX_CHARACTER_LIMIT: process.env.MAX_CHARACTER_LIMIT,
  MAGIC_LINK_TTL_SECONDS: process.env.MAGIC_LINK_TTL_SECONDS,
  // Shown in the navbar. With this unset the page falls back to telnet
  // instructions built from the host the browser asked for.
  SITE_TITLE: process.env.SITE_TITLE,

  // Outbound mail for magic links. With SMTP_HOST unset no mail is sent:
  // development logs the link, tests capture it, production refuses to send.
  SMTP_HOST: process.env.SMTP_HOST,
  SMTP_PORT: process.env.SMTP_PORT,
  SMTP_USER: process.env.SMTP_USER,
  SMTP_PASS: process.env.SMTP_PASS,
  SMTP_SECURE: process.env.SMTP_SECURE,
  SMTP_FROM: process.env.SMTP_FROM,
}

/** A development-only fallback, so a local checkout runs with no setup. */
const DEV_AUTH_SECRET = 'port4k-development-secret-not-for-production-use'

if (required.NODE_ENV === 'development') {
  // In development, we can use the default values for some environment variables
  optional.PORT = optional.PORT || '3000'
}

for (const key in required) {
  if (!required[key as keyof typeof required]) {
    throw new Error(`Missing required environment variable: ${key}`)
  }
}
const validatedRequired = required as { [key in keyof typeof required]: string }

const config = {
  ...validatedRequired,
  ...optional,

  // Convert string values
  NEXT_PUBLIC_APP_NAME:
    optional.NEXT_PUBLIC_APP_NAME.charAt(0).toUpperCase() + optional.NEXT_PUBLIC_APP_NAME.slice(1),
  NEXT_PUBLIC_APP_VERSION: optional.NEXT_PUBLIC_APP_VERSION,
  PORT: parseInt(optional.PORT || '3000', 10),

  BETTER_AUTH_SECRET: optional.BETTER_AUTH_SECRET || DEV_AUTH_SECRET,
  MAX_CHARACTER_LIMIT: parseCount(optional.MAX_CHARACTER_LIMIT, 5),
  MAGIC_LINK_TTL_SECONDS: parseCount(optional.MAGIC_LINK_TTL_SECONDS, 15 * 60),
  SMTP_PORT: parseCount(optional.SMTP_PORT, 587),
  SMTP_SECURE: optional.SMTP_SECURE === 'true',
  SMTP_FROM: optional.SMTP_FROM || optional.SMTP_USER,
}

export default config

/**
 * Checked when the server starts rather than when this module is imported: a
 * production build runs with NODE_ENV=production but has no business needing
 * runtime secrets, and `next build` imports this to render pages.
 */
export function assertRuntimeSecrets(): void {
  if (config.NODE_ENV !== 'production' || optional.BETTER_AUTH_SECRET) return

  throw new Error(
    'Missing required environment variable: BETTER_AUTH_SECRET. Generate one with ' +
      '`openssl rand -base64 32`. It signs portal sessions and encrypts stored MUD ' +
      'passwords, so keep it stable - changing it signs everyone out and makes every ' +
      'saved character password unreadable.',
  )
}

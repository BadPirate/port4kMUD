import dotenv from 'dotenv'
// Import package.json directly
// eslint-disable-next-line import/extensions
import packageJson from '../../package.json'

// Only load dotenv in server environment
if (typeof window === 'undefined') {
  dotenv.config()
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
}

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
}

export default config

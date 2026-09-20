import { betterAuth } from 'better-auth'
import { prismaAdapter } from 'better-auth/adapters/prisma'
import { magicLink } from 'better-auth/plugins'
import config from './config'
import { gravatarUrl } from './gravatar'
import { sendMagicLinkEmail } from './mailer'
import prisma from './prisma'

/**
 * Portal accounts: an email address, a session cookie, and nothing else.
 *
 * Sign-in is by magic link only, so the portal never stores a password of its
 * own. The MUD passwords it does store are a separate matter - those belong to
 * the game, and the bridge has to be able to replay them verbatim.
 *
 * This instance is owned by the custom server (server.ts), which serves
 * /api/auth/* through toNodeHandler before handing anything else to Next.js.
 * Keeping it out of the Next bundle means one auth instance and one database
 * connection in the process, which is also what lets the Socket.IO bridge
 * check a session on the WebSocket handshake.
 */
export const auth = betterAuth({
  appName: config.NEXT_PUBLIC_APP_NAME,
  secret: config.BETTER_AUTH_SECRET,
  // Left undefined unless configured, so Better Auth derives the origin from
  // the request and magic links work on whatever host the portal is reached on.
  baseURL: config.BETTER_AUTH_URL,
  database: prismaAdapter(prisma, { provider: 'sqlite' }),
  telemetry: { enabled: false },
  plugins: [
    magicLink({
      expiresIn: config.MAGIC_LINK_TTL_SECONDS,
      sendMagicLink: async ({ email, url }) => {
        await sendMagicLinkEmail(email, url)
      },
    }),
  ],
  databaseHooks: {
    user: {
      create: {
        before: async (user) => ({
          data: {
            ...user,
            // Magic-link sign-up has no name to work with, so the address
            // stands in for one, and its Gravatar is the dropdown's avatar.
            name: user.name || user.email.split('@')[0],
            image: user.image || gravatarUrl(user.email),
          },
        }),
      },
    },
  },
})

export type PortalSession = Awaited<ReturnType<typeof auth.api.getSession>>

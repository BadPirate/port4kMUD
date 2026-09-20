import { createAuthClient } from 'better-auth/react'
import { magicLinkClient } from 'better-auth/client/plugins'

/**
 * The browser's half of Better Auth.
 *
 * No baseURL: the portal is served from whatever host the player reached it
 * on, and the API lives at /api/auth on that same origin.
 */
export const authClient = createAuthClient({
  plugins: [magicLinkClient()],
})

export const { useSession, signOut } = authClient

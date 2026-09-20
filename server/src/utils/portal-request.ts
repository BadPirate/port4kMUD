import type { IncomingMessage } from 'http'

/**
 * Who is signed in, made available to a Next.js page without importing the
 * auth stack into its bundle.
 *
 * The custom server has already mounted Better Auth and opened the database,
 * and keeping the generated Prisma client and better-sqlite3's native module
 * out of the Next.js build is the whole reason it is mounted there rather
 * than as a route (see server.ts). So the server leaves a way to ask, and
 * this module - which imports nothing but a type - is all a page needs.
 */
const RESOLVER = Symbol.for('port4k.portalUserId')

type WithResolver = IncomingMessage & {
  [RESOLVER]?: () => Promise<string | null>
}

/** Called by the server for every request it hands to Next.js. */
export function attachPortalSession(
  req: IncomingMessage,
  resolve: () => Promise<string | null>,
): void {
  ;(req as WithResolver)[RESOLVER] = resolve
}

/**
 * The signed-in user's id, or null.
 *
 * Resolved here rather than when the request arrived, so the requests that
 * never ask - every asset and data fetch - pay nothing for it. Null when
 * nothing attached a resolver, which keeps a page rendered by anything other
 * than our own server working.
 */
export async function readPortalUserId(req: IncomingMessage): Promise<string | null> {
  const resolve = (req as WithResolver)[RESOLVER]
  return resolve ? resolve() : null
}

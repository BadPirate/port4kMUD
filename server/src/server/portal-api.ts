import type { IncomingMessage, ServerResponse } from 'http'
import { fromNodeHeaders } from 'better-auth/node'
import { auth } from '../utils/auth'
import { listCharacters, removeCharacter } from '../utils/characters'
import config from '../utils/config'
import { clearCapturedMagicLinks, listCapturedMagicLinks } from '../utils/test-magic-links'

/**
 * The portal's own HTTP endpoints, served by the custom server alongside
 * Better Auth's and ahead of Next.js.
 *
 * They are deliberately read-only plus one delete, so nothing here has to
 * parse a request body: everything that writes a character goes through the
 * Socket.IO bridge, which is the only place that knows a MUD login succeeded.
 */
export const PORTAL_API_PREFIX = '/api/portal/'

function sendJson(res: ServerResponse, status: number, body: unknown): void {
  const payload = JSON.stringify(body)
  res.writeHead(status, {
    'Content-Type': 'application/json',
    'Content-Length': Buffer.byteLength(payload),
    // Account state must never be served from a cache.
    'Cache-Control': 'no-store',
  })
  res.end(payload)
}

/** Resolves the signed-in user from the request's session cookie. */
export async function getSessionUser(req: IncomingMessage) {
  try {
    const session = await auth.api.getSession({ headers: fromNodeHeaders(req.headers) })
    return session?.user ?? null
  } catch (err) {
    console.error('Portal API: failed to read session:', err)
    return null
  }
}

/**
 * Handles a /api/portal/ request. Returns false when the path is not one of
 * ours, so the caller can pass it on to Next.js.
 */
export async function handlePortalApi(
  req: IncomingMessage,
  res: ServerResponse,
  pathname: string,
): Promise<boolean> {
  if (!pathname.startsWith(PORTAL_API_PREFIX)) return false

  const route = pathname.slice(PORTAL_API_PREFIX.length)

  if (route === 'test/magic-links') {
    return handleTestMagicLinks(req, res)
  }

  if (route === 'characters' && req.method === 'GET') {
    const user = await getSessionUser(req)
    if (!user) {
      sendJson(res, 401, { error: 'Not signed in' })
      return true
    }
    sendJson(res, 200, {
      email: user.email,
      image: user.image ?? null,
      characters: await listCharacters(user.id),
      max: config.MAX_CHARACTER_LIMIT,
    })
    return true
  }

  const character = /^characters\/([^/]+)$/.exec(route)
  if (character && req.method === 'DELETE') {
    const user = await getSessionUser(req)
    if (!user) {
      sendJson(res, 401, { error: 'Not signed in' })
      return true
    }
    // Forgets the character here only; the character itself stays in the MUD.
    const removed = await removeCharacter(user.id, decodeURIComponent(character[1]))
    sendJson(res, removed ? 200 : 404, { removed })
    return true
  }

  sendJson(res, 404, { error: 'Not found' })
  return true
}

/**
 * Hands back the magic links the mailer captured instead of sending, so the
 * end-to-end tests can follow one. It exists only under NODE_ENV=test, and
 * answers 404 otherwise so it cannot become a way to read anyone's links.
 */
function handleTestMagicLinks(req: IncomingMessage, res: ServerResponse): boolean {
  if (config.NODE_ENV !== 'test') {
    sendJson(res, 404, { error: 'Not found' })
    return true
  }

  if (req.method === 'DELETE') {
    clearCapturedMagicLinks()
    sendJson(res, 200, { cleared: true })
    return true
  }

  sendJson(res, 200, { emails: listCapturedMagicLinks() })
  return true
}

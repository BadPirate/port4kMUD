import { mkdirSync } from 'fs'
import path from 'path'

/**
 * Where the portal's SQLite database lives.
 *
 * The default sits inside mud/lib, which is already the volume a deployment
 * mounts for persistence (see DOKKU.md) and is already covered by the
 * repository's .gitignore, so accounts survive a redeploy with no extra setup.
 * AUTH_DATABASE_PATH overrides it, which the tests use to work on a throwaway
 * file.
 *
 * The caller supplies that setting rather than this module reading it, because
 * prisma.config.ts runs under the Prisma CLI, outside the app, where the
 * config utility's own requirements (NODE_ENV among them) do not hold - but it
 * must still resolve the database to exactly the same absolute path.
 */
export function resolveAuthDatabasePath(configured?: string): string {
  return configured
    ? path.resolve(configured)
    : path.resolve(process.cwd(), '..', 'mud', 'lib', 'etc', 'portal.sqlite')
}

/**
 * Makes sure the directory holding the database exists. better-sqlite3 will
 * create the file but not the path to it, and fails with a bare "Cannot open
 * database because the directory does not exist" if it is missing.
 */
export function ensureAuthDatabaseDirectory(databasePath: string): void {
  mkdirSync(path.dirname(databasePath), { recursive: true })
}

import { PrismaBetterSqlite3 } from '@prisma/adapter-better-sqlite3'
import { PrismaClient } from '../generated/prisma/client'
import { ensureAuthDatabaseDirectory, resolveAuthDatabasePath } from './auth-db-path'
import config from './config'

/**
 * The portal's database handle.
 *
 * Only the custom server ever touches this: server.ts serves /api/auth and
 * /api/portal itself, ahead of Next.js, so the bundler never sees the
 * generated client or better-sqlite3's native module, and the process holds a
 * single connection to a single SQLite file.
 *
 * Cached on globalThis because ts-node reloads modules across a dev restart,
 * and a second handle on the same file would mean a second write lock.
 */
const globalForPrisma = globalThis as typeof globalThis & { portalPrisma?: PrismaClient }

function createClient(): PrismaClient {
  const databasePath = resolveAuthDatabasePath(config.AUTH_DATABASE_PATH)
  // better-sqlite3 creates the file but not the directory above it.
  ensureAuthDatabaseDirectory(databasePath)
  return new PrismaClient({ adapter: new PrismaBetterSqlite3({ url: `file:${databasePath}` }) })
}

const prisma = globalForPrisma.portalPrisma ?? createClient()

if (config.NODE_ENV !== 'production') {
  globalForPrisma.portalPrisma = prisma
}

export default prisma

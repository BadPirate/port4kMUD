// Loaded by the Prisma CLI. Prisma 7 no longer reads .env by itself, and the
// datasource URL lives here rather than in schema.prisma so the app and the
// CLI resolve the database to the same absolute path.
import 'dotenv/config'
import { defineConfig } from 'prisma/config'
import { ensureAuthDatabaseDirectory, resolveAuthDatabasePath } from './src/utils/auth-db-path'

const databasePath = resolveAuthDatabasePath(process.env.AUTH_DATABASE_PATH)
ensureAuthDatabaseDirectory(databasePath)

export default defineConfig({
  schema: 'prisma/schema.prisma',
  migrations: { path: 'prisma/migrations' },
  datasource: { url: `file:${databasePath}` },
})

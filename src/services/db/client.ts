import path from 'path'
import { PrismaClient } from '@prisma/client'
import config from '@/src/utils/config'

/**
 * Test-friendly Prisma client implementation
 *
 * This implementation provides explicit path handling for database files and
 * direct configuration of connection strings. It's particularly useful for:
 *
 * 1. Test environments that run outside the Next.js runtime
 * 2. Scenarios requiring custom database configurations
 * 3. Services that need consistent database connections across different environments
 */

let connectionString = config.DATABASE_URL

// If using filesystem SQLite URL, convert to absolute file URL so paths resolve consistently
if (connectionString.startsWith('file:')) {
  const filePath = connectionString.replace(/^file:(?:\/\/)?/, '')
  const absPath = path.resolve(process.cwd(), filePath)
  connectionString = 'file:' + absPath
}

const prisma = new PrismaClient({
  datasources: { db: { url: connectionString } },
})

export default prisma

import path from 'path'
import { PrismaClient } from '@prisma/client'
import dotenv from 'dotenv'

dotenv.config()

// Use DATABASE_URL env var or fallback to local SQLite dev.db
let connectionString = process.env.DATABASE_URL || 'file:./dev.db'
// If using filesystem SQLite URL, convert to absolute file URL so paths resolve consistently
if (connectionString.startsWith('file:')) {
  const filePath = connectionString.replace(/^file:(?:\/\/)?/, '')
  const absPath = path.resolve(process.cwd(), filePath)
  connectionString = 'file:' + absPath
}

/**
 * Prisma client instance configured at runtime.
 */
const prisma = new PrismaClient({
  datasources: { db: { url: connectionString } },
})

export default prisma

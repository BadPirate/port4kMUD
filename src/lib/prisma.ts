/* eslint-disable no-trailing-spaces, no-multiple-empty-lines, eol-last */
import path from 'path'
import fs from 'fs'
import { PrismaClient } from '@prisma/client'
import config from '../utils/config'

/**
 * Creates a properly configured Prisma client for any environment
 *
 * @param connectionUrl - Database connection URL (defaults to config.DATABASE_URL)
 * @returns A configured PrismaClient instance
 */
export function createPrismaClient(connectionUrl = config.DATABASE_URL): PrismaClient {
  let finalUrl = connectionUrl

  // Handle SQLite file: URLs by converting to absolute paths
  if (finalUrl.startsWith('file:')) {
    const filePath = finalUrl.replace(/^file:(?:\/\/)?/, '')
    const absPath = path.resolve(process.cwd(), filePath)
    finalUrl = 'file:' + absPath

    // Determine which provider schema to use (SQLite or PostgreSQL)
    const provider = 'sqlite'
    const schemaPath = path.resolve(process.cwd(), 'prisma', 'generated', `${provider}.prisma`)

    // Check if the schema exists, throw helpful error if not
    if (!fs.existsSync(schemaPath)) {
      throw new Error(
        `Prisma schema not found at ${schemaPath}. ` +
          'Please run `yarn build:prisma-schemas` first.',
      )
    }
  }

  // Create a new Prisma client with the processed connection URL
  return new PrismaClient({
    datasources: { db: { url: finalUrl } },
  })
}

/**
 * Next.js-specific Prisma client singleton
 *
 * Uses the globally scoped pattern recommended by Prisma for Next.js
 * applications to prevent multiple instances during hot reloading.
 */

// Define global type for Prisma instance
declare global {
  // eslint-disable-next-line vars-on-top, no-var
  var prisma: PrismaClient | undefined // eslint-disable-line no-unused-vars
}

// Create or reuse a Prisma client instance
const prisma = global.prisma || createPrismaClient()

// Save to global in development to prevent multiple instances
if (config.NODE_ENV !== 'production') {
  global.prisma = prisma
}

export default prisma

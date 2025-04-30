import { createPrismaClient } from '../../lib/prisma'

// Create a dedicated client instance for services
// This ensures tests and services can have their own client if needed
const client = {
  prisma: createPrismaClient(),
}

export default client

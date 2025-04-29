/* eslint-disable no-trailing-spaces, no-multiple-empty-lines, eol-last */
import { PrismaClient } from '@prisma/client'
import config from '../utils/config'

// Prevent multiple instances of Prisma Client in development
declare global {
  // eslint-disable-next-line vars-on-top, no-var
  var prisma: PrismaClient | undefined // eslint-disable-line no-unused-vars
}

const prisma = global.prisma || new PrismaClient()
if (config.NODE_ENV !== 'production') global.prisma = prisma

export default prisma

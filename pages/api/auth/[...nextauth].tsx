/* eslint-disable camelcase */
import NextAuth, { type NextAuthOptions } from 'next-auth'
import CredentialsProvider from 'next-auth/providers/credentials'
import EmailProvider from 'next-auth/providers/email'
import { PrismaAdapter } from '@next-auth/prisma-adapter'
import prisma from '../../../src/lib/prisma'

type ProfileData = {
  sub: string
  client_id: string
  aud: string
  scope: string
  iss: string
  email: string
  name: string
  picture: string
  iat: number
  exp: number
}

// Enable a simple credentials-based login when TEST_MODE is true (e.g., during Playwright tests)
const isTestMode = process.env.TEST_MODE === 'true'
// Provide a default secret for NextAuth (override via NEXTAUTH_SECRET in production)
const authSecret = process.env.NEXTAUTH_SECRET || 'test-secret'

// Build NextAuth provider list
const providers: NextAuthOptions['providers'] = []
// Email (magic link) & credentials providers in test mode
if (isTestMode) {
  providers.push(
    EmailProvider({
      server: {
        host: process.env.SMTP_HOST || 'localhost',
        port: Number(process.env.SMTP_PORT) || 1025,
        auth: { user: process.env.SMTP_USER || '', pass: process.env.SMTP_PASS || '' },
      },
      from: process.env.EMAIL_FROM || 'nextstrap@example.com',
      // Capture emails in test mode
      sendVerificationRequest: async ({ identifier, url }) => {
        // Store the magic link in memory
        global.TEST_EMAILS = global.TEST_EMAILS || []
        global.TEST_EMAILS.push({ identifier, url })
      },
    }),
  )
  providers.push(
    CredentialsProvider({
      id: 'nextstrap',
      name: 'TestAuth',
      credentials: {
        email: { label: 'Email', type: 'text', placeholder: 'you@example.com' },
        password: { label: 'Password', type: 'text', placeholder: 'test password' },
      },
      async authorize(credentials) {
        if (credentials?.email && credentials?.password) {
          return { id: credentials.password, name: credentials.password, email: credentials.email }
        }
        return null
      },
    }),
  )
} else {
  // OAuth provider in production mode
  providers.push({
    id: 'nextstrap',
    name: 'GarageAuth',
    type: 'oauth',
    version: '2.0',
    issuer: 'https://auth.badpirate.net',
    clientId: process.env.GARAGE_AUTH_CLIENT_ID,
    clientSecret: process.env.GARAGE_AUTH_CLIENT_SECRET,
    wellKnown: 'https://auth.badpirate.net/.well-known/openid-configuration',
    authorization: { params: { scope: 'openid profile email' } },
    checks: ['state', 'pkce'],
    async profile(profileData: ProfileData) {
      const { sub, name, email, picture, client_id, iss } = profileData
      if (client_id !== process.env.GARAGE_AUTH_CLIENT_ID) throw new Error('Invalid client_id')
      if (iss !== 'https://auth.badpirate.net') throw new Error('Invalid issuer')
      return {
        id: sub,
        name,
        email,
        image: picture,
      }
    },
  })
}

const authOptions: NextAuthOptions = {
  adapter: PrismaAdapter(prisma),
  secret: authSecret,
  providers,
}

export default NextAuth(authOptions)

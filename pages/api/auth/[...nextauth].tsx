/* eslint-disable camelcase */
import NextAuth from 'next-auth'
import CredentialsProvider from 'next-auth/providers/credentials'

type ProfileData = {
  sub: string,
  client_id: string,
  aud: string,
  scope: string,
  iss: string,
  email: string,
  name: string,
  picture: string,
  iat: number,
  exp: number
}

// Enable a simple credentials-based login when TEST_MODE is true (e.g., during Playwright tests)
const isTestMode = process.env.TEST_MODE === 'true'
// Provide a default secret for NextAuth (override via NEXTAUTH_SECRET in production)
const authSecret = process.env.NEXTAUTH_SECRET || 'test-secret'

export default NextAuth({
  secret: authSecret,
  providers: isTestMode ? [
    // Credentials provider for test mode: accepts any email/password,
    // returns a user with id and name set to password
    CredentialsProvider({
      id: 'GarageAuth',
      name: 'TestLogin',
      credentials: {
        email: { label: 'Email', type: 'text', placeholder: 'you@example.com' },
        password: { label: 'Password', type: 'text', placeholder: 'test password' },
      },
      async authorize(credentials: any) {
        if (credentials && credentials.email && credentials.password) {
          return { id: credentials.password, name: credentials.password, email: credentials.email }
        }
        return null
      },
    }),
  ] : [
    // Standard OAuth provider
    {
      id: 'campfire',
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
        const {
          sub, name, email, picture, client_id, iss,
        } = profileData
        if (client_id !== process.env.GARAGE_AUTH_CLIENT_ID) {
          throw new Error('Invalid client_id')
        }
        if (iss !== 'https://auth.badpirate.net') {
          throw new Error('Invalid issuer')
        }
        return {
          id: sub,
          name,
          email,
          image: picture,
        }
      },
    },
  ],
})

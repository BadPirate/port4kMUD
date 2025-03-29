/* eslint-disable camelcase */
import NextAuth from 'next-auth'

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

export default NextAuth({
  providers: [
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

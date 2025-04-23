/* eslint-disable no-unused-vars */
import { useSession } from 'next-auth/react'
import type { Session } from 'next-auth'
import type { ReactNode } from 'react'

type BodyOptions = {
  authenticated: (_session: Session) => ReactNode
  unauthenticated: ReactNode
}

const WithSession = ({ authenticated, unauthenticated }: BodyOptions) => {
  const { data, status } = useSession()
  // Show authenticated content when session exists; else show unauthenticated
  return (status === 'authenticated' && data)
    ? authenticated(data)
    : unauthenticated
}

// No loading state in this simplified version

export default WithSession

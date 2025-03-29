import { useSession } from 'next-auth/react'
import { Spinner } from 'react-bootstrap'
import { Session } from 'next-auth'

type BodyOptions = {
  // eslint-disable-next-line no-unused-vars
  authenticated: (session: Session) => React.ReactNode,
  loading?: React.ReactNode,
  unauthenticated: React.ReactNode
}

const WithSession = ({ authenticated, loading, unauthenticated }: BodyOptions) => {
  const { data: session, status } = useSession()
  return (
    <>
      { status === 'loading' ? loading : null }
      { status === 'unauthenticated' ? unauthenticated : null }
      { status === 'authenticated' ? authenticated(session) : null }
    </>
  )
}

WithSession.defaultProps = {
  loading: <Spinner />,
}

export default WithSession

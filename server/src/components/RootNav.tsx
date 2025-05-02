import React from 'react'
import { Navbar, Container, Button } from 'react-bootstrap'
import { signIn, signOut } from 'next-auth/react'
import config from '../utils/config'
import WithSession from './WithSession'

const { NEXT_PUBLIC_APP_NAME: appName, NEXT_PUBLIC_APP_VERSION: appVersion } = config

const RootNav = ({ children }: { children: React.ReactNode }) => (
  <div>
    <Navbar variant="dark" bg="secondary">
      <Navbar.Brand href="/" style={{ marginLeft: '1em', textTransform: 'capitalize' }}>
        {appName}
      </Navbar.Brand>
      <Navbar.Text>{`v${appVersion}`}</Navbar.Text>
      <span className="ms-auto me-3">
        <WithSession
          // eslint-disable-next-line react/no-unstable-nested-components
          authenticated={(session) => (
            <Button onClick={() => signOut()}>
              {session.user?.name ?? session.user?.email ?? 'Logout'}
            </Button>
          )}
          unauthenticated={
            <Button
              onClick={() => {
                const provider = config.NEXT_PUBLIC_TEST_MODE ? 'email' : 'nextstrap'
                signIn(provider, { callbackUrl: '/' })
              }}
            >
              Login
            </Button>
          }
        />
      </span>
    </Navbar>
    <Container style={{ marginTop: '1em' }}>{children}</Container>
  </div>
)

export default RootNav

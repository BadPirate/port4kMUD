import React from 'react'
import { Navbar, Container } from 'react-bootstrap'
import config from '../utils/config'

const { NEXT_PUBLIC_APP_NAME: appName, NEXT_PUBLIC_APP_VERSION: appVersion } = config

const RootNav = ({ children }: { children: React.ReactNode }) => (
  <div>
    <Navbar variant="dark" bg="secondary">
      <Navbar.Brand href="/" style={{ marginLeft: '1em', textTransform: 'capitalize' }}>
        {appName}
      </Navbar.Brand>
      <Navbar.Text>{`v${appVersion}`}</Navbar.Text>
    </Navbar>
    <Container style={{ marginTop: '1em' }}>{children}</Container>
  </div>
)

export default RootNav

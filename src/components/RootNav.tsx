import React from 'react'
import { Navbar, Container } from 'react-bootstrap'

const RootNav = ({ children } : {children : React.ReactNode}) => (
  <div>
    <Navbar variant="dark" bg="secondary">
      <Navbar.Brand href="/" style={{ marginLeft: '1em', textTransform: 'capitalize' }}>
        {process.env.NEXT_PUBLIC_APP_NAME}
      </Navbar.Brand>
      <Navbar.Text>
        {`v${process.env.NEXT_PUBLIC_APP_VERSION}`}
      </Navbar.Text>
    </Navbar>
    <Container style={{ marginTop: '1em' }}>
      {children}
    </Container>
  </div>
)

export default RootNav

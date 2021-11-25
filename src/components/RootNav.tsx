import { Navbar, Nav, Container } from "react-bootstrap"

const RootNav = ({children} : {children : JSX.Element}) =>  
  <div>
    <Navbar variant="dark" bg="secondary">
      <Navbar.Brand href="http://badpirate.net" style={{marginLeft: "1em"}}>
      BadPirate
      </Navbar.Brand>
      <Nav.Link href="https://github.com/BadPirate/nextstrap">Nextstrap</Nav.Link>
    </Navbar>
    <Container style={{marginTop: '1em'}}>
      {children}
    </Container>
  </div> 

export default RootNav
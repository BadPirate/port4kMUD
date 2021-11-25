import {
  Card, ListGroup,
} from 'react-bootstrap'
import RootNav from '../src/components/RootNav'

const Home = () => (
  <div>
    <RootNav>
      <Card>
        <Card.Body>
          <Card.Title>Nextstrap</Card.Title>
          <Card.Text>
            A template for building full stack projects, just the way BadPirate likes it.
          </Card.Text>
          <ListGroup>
            <ListGroup.Item>Next JS</ListGroup.Item>
            <ListGroup.Item>Typescript</ListGroup.Item>
            <ListGroup.Item>ESLint</ListGroup.Item>
            <ListGroup.Item>ReactBootstrap</ListGroup.Item>
          </ListGroup>
        </Card.Body>
      </Card>
    </RootNav>
  </div>
)

export default Home

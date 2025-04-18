import { Card } from 'react-bootstrap'

const WelcomeCard = () => (
  <Card>
    <Card.Body>
      <Card.Title>Welcome to Nextstrap</Card.Title>
      <Card.Text>
        <strong>This template includes:</strong>
        <ul>
          <li>Next.js</li>
          <li>TypeScript</li>
          <li>ESLint with Airbnb config</li>
          <li>React-Bootstrap (Cyborg theme)</li>
          <li>NextAuth.js authentication</li>
          <li>Playwright end-to-end testing</li>
        </ul>
      </Card.Text>
    </Card.Body>
  </Card>
)

export default WelcomeCard


import { Card, CardBody, CardTitle } from 'react-bootstrap'

const WelcomeCard = () => (
  <Card>
    <CardBody>
      <CardTitle>Welcome to Nextstrap</CardTitle>
      <p className="card-text">
        <strong>A Next.js template with Bootstrap and authentication.</strong>
      </p>
      <div className="features">
        <ul>
          <li>Next.js 13+ with React 18+</li>
          <li>Bootstrap 5 styling</li>
          <li>Built-in authentication</li>
          <li>TypeScript support</li>
        </ul>
      </div>
    </CardBody>
  </Card>
)

export default WelcomeCard

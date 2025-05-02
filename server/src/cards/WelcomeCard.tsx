import Card from 'react-bootstrap/Card'
import config from '../utils/config'

const WelcomeCard = () => {
  return (
    <Card>
      <Card.Header>
        <Card.Title data-testid="welcome-title">
          Welcome to {config.NEXT_PUBLIC_APP_NAME}
        </Card.Title>
      </Card.Header>
      <Card.Body>
        <p>Your app is up and running!</p>
      </Card.Body>
    </Card>
  )
}

export default WelcomeCard

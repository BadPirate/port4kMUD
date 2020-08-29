import Head from 'next/head'
import styles from '../styles/Home.module.css'
import { Card } from 'react-bootstrap'

const Home = () => {
  return <div>
    <Card>
      <Card.Title>
        Get Strapped
      </Card.Title>
      <Card.Body>
        <a href="https://github.com/BadPirate/nextstrap">BadPirate NextStrap</a>
      </Card.Body>
    </Card>
  </div>
}

export default Home
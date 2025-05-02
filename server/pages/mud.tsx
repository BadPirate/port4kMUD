import { useEffect, useRef, useState } from 'react'
import Head from 'next/head'
import { Button, Container, Row, Col, Card, Spinner } from 'react-bootstrap'
import { useRouter } from 'next/router'
// eslint-disable-next-line import/no-named-as-default
import io from 'socket.io-client'
import dynamic from 'next/dynamic'
import type { TerminalSocket } from '../src/components/Terminal'

// Dynamically import Terminal with no SSR
const Terminal = dynamic(() => import('../src/components/Terminal'), {
  ssr: false,
  // Use React Bootstrap Spinner for loading state
  loading: () => (
    <div
      style={{
        height: '500px',
        width: '100%',
        background: '#000',
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
      }}
    >
      <Spinner animation="border" variant="light" role="status">
        <span className="visually-hidden">Loading terminal...</span>
      </Spinner>
    </div>
  ),
})

const MudClient = () => {
  const [connected, setConnected] = useState(false)
  const [error, setError] = useState('')
  const router = useRouter()
  const socketRef = useRef<TerminalSocket | null>(null)

  // Initialize socket connection
  useEffect(() => {
    console.log('Initializing socket connection')
    setError('')

    // Function to initialize the socket connection
    const socketInstance = io('/mud', {
      path: '/api/socket',
      reconnectionAttempts: 5,
      reconnectionDelay: 1000,
      transports: ['websocket'],
    })

    // Store socket in ref to maintain reference
    socketRef.current = socketInstance

    // Set up socket event handlers
    socketInstance.on('connect', () => {
      // eslint-disable-next-line no-console
      console.log('WebSocket connected')
      setConnected(true)
    })

    socketInstance.on('status', (status: { connected: boolean }) => {
      setConnected(status.connected)
    })

    socketInstance.on('disconnect', () => {
      // eslint-disable-next-line no-console
      console.log('WebSocket disconnected')
      setConnected(false)
    })

    socketInstance.on('error', (err: { message?: string }) => {
      // eslint-disable-next-line no-console
      console.error('Socket error:', err)
      setError(`Connection error: ${err.message || 'Unknown error'}`)
    })

    socketRef.current = socketInstance
    // Clean up function
    return () => {
      console.log('Cleaning up socket connection')
      if (socketRef.current) {
        socketRef.current.disconnect()
        socketRef.current = null
      }
    }
  }, [])

  return (
    <>
      <Head>
        <title>Port4k MUD Client</title>
        <meta name="description" content="Web client for Port4k MUD" />
      </Head>

      <Container fluid className="p-3">
        <Row>
          <Col>
            <h1 className="mb-4">Port4k MUD</h1>
            {error && <div className="alert alert-danger">{error}</div>}
            <Card className="mb-3">
              <Card.Body style={{ height: '500px' }}>
                <Terminal socketRef={socketRef} />
              </Card.Body>
              <Card.Footer>
                <div className="d-flex justify-content-between">
                  <div>
                    Status:{' '}
                    {connected ? (
                      <span className="text-success">Connected</span>
                    ) : (
                      <span className="text-danger">Disconnected</span>
                    )}
                  </div>
                  <div>
                    <Button variant="secondary" size="sm" onClick={() => router.push('/')}>
                      Back to Home
                    </Button>
                  </div>
                </div>
              </Card.Footer>
            </Card>
          </Col>
        </Row>
      </Container>
    </>
  )
}

export default MudClient

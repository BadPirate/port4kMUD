import { useEffect, useRef, useState, useCallback } from 'react'
import { Button, Alert, Spinner } from 'react-bootstrap'
// eslint-disable-next-line import/no-named-as-default
import io from 'socket.io-client'
import dynamic from 'next/dynamic'
import type { TerminalSocket } from './Terminal'

// Dynamically import Terminal with no SSR
const Terminal = dynamic(() => import('./Terminal'), {
  ssr: false,
  // Use React Bootstrap Spinner for loading state
  loading: () => (
    <div
      style={{
        height: '100%',
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
  const socketRef = useRef<TerminalSocket | null>(null)

  // Initialize socket connection
  const initializeSocket = useCallback(() => {
    console.log('Initializing socket connection')
    setError('')

    // Cleanup any existing socket
    if (socketRef.current) {
      socketRef.current.disconnect()
      socketRef.current = null
    }

    // Create new socket connection
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
      console.log('WebSocket connected')
      setConnected(true)
      setError('')
    })

    socketInstance.on('status', (status: { connected: boolean }) => {
      setConnected(status.connected)
      if (!status.connected) {
        setError('Connection to MUD server lost')
      }
    })

    socketInstance.on('disconnect', () => {
      console.log('WebSocket disconnected')
      setConnected(false)
      setError('Connection to MUD server disconnected')
    })

    socketInstance.on('error', (err: { message?: string }) => {
      console.error('Socket error:', err)
      setError(`Connection error: ${err.message || 'Unknown error'}`)
    })

    socketInstance.on('connect_error', (err: Error) => {
      console.error('Connection failed:', err)
      setError(`Failed to connect: ${err.message || 'Unknown error'}`)
    })

    return socketInstance
  }, [])

  // Initialize on component mount
  useEffect(() => {
    const socket = initializeSocket()

    // Clean up function
    return () => {
      console.log('Cleaning up socket connection')
      if (socket) {
        socket.disconnect()
        socketRef.current = null
      }
    }
  }, [initializeSocket])

  // Handler for reconnect button
  const handleReconnect = () => {
    initializeSocket()
  }

  // Render error screen with reconnect button when disconnected or error
  if (!connected || error) {
    return (
      <div
        className="d-flex flex-column justify-content-center align-items-center"
        style={{
          height: '100%',
          width: '100%',
          minHeight: '400px',
        }}
      >
        <Alert variant="danger" className="mb-3">
          {error || 'Disconnected from MUD server'}
        </Alert>
        <Button variant="primary" onClick={handleReconnect}>
          Reconnect
        </Button>
      </div>
    )
  }

  // Render terminal when connected
  return (
    <div
      style={{
        height: '100%',
        width: '100%',
        minHeight: '400px',
      }}
    >
      <Terminal socketRef={socketRef} />
    </div>
  )
}

export default MudClient

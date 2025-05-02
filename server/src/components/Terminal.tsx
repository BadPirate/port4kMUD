// Importing FitAddon but commenting it out until needed
// import { FitAddon } from '@xterm/addon-fit'
import { RefObject, useEffect } from 'react'
import { useXTerm } from '../react-xterm'
import { Socket } from 'socket.io-client'

export type TerminalSocket = typeof Socket

interface TerminalProps {
  socketRef: RefObject<TerminalSocket | null>
}

const Terminal = ({ socketRef }: TerminalProps) => {
  const { elementRef, terminalRef } = useXTerm({
    options: {
      cursorBlink: true,
      fontSize: 14,
      fontFamily: 'Menlo, Monaco, Consolas, monospace',
      theme: {
        background: '#000000',
        foreground: '#f0f0f0',
      },
      disableStdin: false,
      allowTransparency: true,
    },
  })

  // Connect socket to terminal
  useEffect(() => {
    console.log('Connecting socket to terminal')
    const socket = socketRef.current
    const terminal = terminalRef.current
    if (!socket || !terminal) {
      console.error('Socket or terminal not available')
      return
    }

    const sendToSocket = (data: string) => {
      console.log('Sending data to socket:', data)
      if (socket && socket.connected) {
        socket.emit('input', data)
      }
    }

    const dispose = terminal.onData(sendToSocket)

    // Create named handler function for proper cleanup
    const sendToTerminal = (data: string) => {
      console.log('Received data from socket:', data)
      terminal.write(data)
    }

    socket.on('data', sendToTerminal)

    // Clean up function
    return () => {
      dispose.dispose()
      if (socket) {
        socket.off('data', sendToSocket)
      }
    }
  }, [socketRef, terminalRef]) // Added missing dependencies

  return (
    <div ref={elementRef as RefObject<HTMLDivElement>} style={{ height: '100%', width: '100%' }} />
  )
}

export default Terminal

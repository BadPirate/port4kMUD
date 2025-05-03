import { FitAddon } from '@xterm/addon-fit'
import { RefObject, useEffect, useState, useCallback, useRef } from 'react'
import { Socket } from 'socket.io-client'
import { useXTerm } from '../react-xterm'

export type TerminalSocket = typeof Socket

interface TerminalProps {
  socketRef: RefObject<TerminalSocket | null>
}

const Terminal = ({ socketRef }: TerminalProps) => {
  const [fontSize, setFontSize] = useState(14)
  const [fitAddon] = useState(() => new FitAddon())
  const containerRef = useRef<HTMLDivElement | null>(null)

  const { elementRef, terminalRef } = useXTerm({
    options: {
      cursorBlink: true,
      fontSize,
      fontFamily: 'Menlo, Monaco, Consolas, monospace',
      theme: {
        background: '#000000',
        foreground: '#f0f0f0',
      },
      disableStdin: false,
      allowTransparency: true,
      cols: 80,
      rows: 24,
      convertEol: true, // Convert \n to \r\n (important for MUDs)
      scrollback: 1000, // Provide scrollback for history
    },
    addons: [fitAddon],
  })

  // Function to optimize font size for 80 columns
  const optimizeFontSize = useCallback(() => {
    if (!containerRef.current || !terminalRef.current) return

    const containerWidth = containerRef.current.clientWidth

    // Calculate optimal font size to fit 80 columns
    // Using a more direct calculation without relying on hidden renderer properties
    const optimalFontSize = Math.floor((containerWidth / 80) * 1.66)

    // Set limits to prevent too small/large fonts
    const newSize = Math.max(8, Math.min(22, optimalFontSize))

    if (newSize !== fontSize) {
      setFontSize(newSize)

      // After setting font size, trigger a fit to make sure the terminal adjusts
      setTimeout(() => {
        if (fitAddon && terminalRef.current) {
          fitAddon.fit()
        }
      }, 10)
    }
  }, [containerRef, terminalRef, fontSize, fitAddon])

  // Function to handle terminal resizing
  const handleResize = useCallback(() => {
    if (!terminalRef.current) return

    optimizeFontSize()

    try {
      setTimeout(() => {
        if (fitAddon && terminalRef.current) {
          fitAddon.fit()
        }
      }, 0)
    } catch (err) {
      console.warn('Error fitting terminal:', err)
    }
  }, [fitAddon, terminalRef, optimizeFontSize])

  // Set up resize observer for the terminal container
  useEffect(() => {
    if (!elementRef.current) return

    containerRef.current = elementRef.current
    const observedElement = elementRef.current

    const resizeObserver = new ResizeObserver(() => {
      handleResize()
    })

    resizeObserver.observe(observedElement)
    window.addEventListener('resize', handleResize)

    setTimeout(handleResize, 100)

    return () => {
      resizeObserver.unobserve(observedElement)
      window.removeEventListener('resize', handleResize)
    }
  }, [elementRef, handleResize])

  // Apply font size changes to terminal
  useEffect(() => {
    if (terminalRef.current) {
      terminalRef.current.options.fontSize = fontSize
      setTimeout(() => {
        if (fitAddon && terminalRef.current) {
          fitAddon.fit()
        }
      }, 0)
    }
  }, [fontSize, terminalRef, fitAddon])

  // Connect socket to terminal
  useEffect(() => {
    const socket = socketRef.current
    const terminal = terminalRef.current
    if (!socket || !terminal) return

    // Handle user input - simply send to server
    // The server will handle echoing characters back when appropriate
    const handleData = (data: string) => {
      if (socket && socket.connected) {
        socket.emit('input', data)
      }
    }

    const dataDisposable = terminal.onData(handleData)

    // Handle incoming data from server
    const handleServerData = (data: string) => {
      terminal.write(data)
    }

    socket.on('data', handleServerData)

    // Clean up on unmount
    return () => {
      dataDisposable.dispose()
      socket.off('data', handleServerData)
    }
  }, [socketRef, terminalRef])

  return (
    <div ref={elementRef as RefObject<HTMLDivElement>} style={{ height: '100%', width: '100%' }} />
  )
}

export default Terminal

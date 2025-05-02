import { Socket } from 'net'

/**
 * Configuration for the MUD server
 */
export const MUD_CONFIG = {
  host: '127.0.0.1',
  port: 4000,
  connectionTimeout: 2000, // 2 seconds
}

/**
 * Check if the MUD server is running and accessible
 * @returns Promise that resolves to true if server is running, false otherwise
 */
export const checkMudServer = (): Promise<boolean> => {
  return new Promise((resolve) => {
    const socket = new Socket()
    let resolved = false

    // Set timeout to handle connection failure
    socket.setTimeout(MUD_CONFIG.connectionTimeout)

    socket.on('connect', () => {
      if (!resolved) {
        resolved = true
        socket.destroy()
        resolve(true)
      }
    })

    socket.on('timeout', () => {
      if (!resolved) {
        resolved = true
        socket.destroy()
        resolve(false)
      }
    })

    socket.on('error', () => {
      if (!resolved) {
        resolved = true
        socket.destroy()
        resolve(false)
      }
    })

    // Attempt connection
    socket.connect(MUD_CONFIG.port, MUD_CONFIG.host)
  })
}

/**
 * Start the MUD server if it's not already running
 * This function should be called from server.js on startup
 */
export const ensureMudServerRunning = async (): Promise<void> => {
  const isRunning = await checkMudServer()

  if (!isRunning) {
    console.warn('WARNING: MUD server is not running on port 4000')
    console.warn('Please start the MUD server with: cd src && ../bin/circle')
  } else {
    console.log('MUD server is running and accessible')
  }
}

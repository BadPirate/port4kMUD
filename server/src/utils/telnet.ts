// Telnet protocol constants
export const IAC = 255 // Interpret As Command
export const DONT = 254 // You are not to do this
export const DO = 253 // Please do this
export const WONT = 252 // I won't do this
export const WILL = 251 // I will do this
export const SB = 250 // Subnegotiation Begin
export const SE = 240 // Subnegotiation End
export const TELOPT_ECHO = 1 // Echo option

export interface TelnetEchoState {
  serverEcho: boolean
  telnetBuffer: number[]
  onEchoChange?: (serverEcho: boolean) => void
}

/**
 * Strips telnet IAC negotiation sequences out of a raw data stream, returning
 * just the plain text. Tracks server-echo state (e.g. disabled during password
 * entry) via the raw, non-negotiated IAC WILL/WONT TELOPT_ECHO bytes this MUD
 * sends (see comm.c echo_off/echo_on) - no reply is expected or sent back.
 */
export function processTelnetData(connection: TelnetEchoState, data: Buffer): string {
  const bytes = Array.from(data)
  let processed = ''
  let i = 0

  // Append new bytes to existing buffer if any
  const buffer = [...connection.telnetBuffer, ...bytes]
  connection.telnetBuffer = []

  while (i < buffer.length) {
    // Check for IAC
    if (buffer[i] === IAC) {
      if (i + 1 >= buffer.length) {
        // Incomplete command, store in buffer
        connection.telnetBuffer = buffer.slice(i)
        break
      }

      // Process telnet command
      if (buffer[i + 1] === WILL && i + 2 < buffer.length) {
        if (buffer[i + 2] === TELOPT_ECHO) {
          // Server wants to handle echo (which means client should NOT echo)
          connection.serverEcho = false
          connection.onEchoChange?.(false)
        }
        i += 3 // Skip the 3-byte command
        continue
      } else if (buffer[i + 1] === WONT && i + 2 < buffer.length) {
        if (buffer[i + 2] === TELOPT_ECHO) {
          // Server does not want to handle echo (which means client SHOULD echo)
          connection.serverEcho = true
          connection.onEchoChange?.(true)
        }
        i += 3 // Skip the 3-byte command
        continue
      } else if (buffer[i + 1] === DO || buffer[i + 1] === DONT) {
        // Skip these commands if they're complete
        if (i + 2 < buffer.length) {
          i += 3
          continue
        } else {
          // Incomplete command, store in buffer
          connection.telnetBuffer = buffer.slice(i)
          break
        }
      } else if (buffer[i + 1] === SB) {
        // Find the end of subnegotiation
        let j = i + 2
        while (j < buffer.length - 1 && !(buffer[j] === IAC && buffer[j + 1] === SE)) {
          j++
        }

        if (j < buffer.length - 1) {
          // Complete subnegotiation
          i = j + 2
          continue
        } else {
          // Incomplete subnegotiation
          connection.telnetBuffer = buffer.slice(i)
          break
        }
      } else if (buffer[i + 1] === IAC) {
        // Escaped IAC - add a single IAC byte
        processed += String.fromCharCode(IAC)
        i += 2
        continue
      } else {
        // Unknown or incomplete command
        i++
        continue
      }
    }

    // Regular data byte
    processed += String.fromCharCode(buffer[i])
    i++
  }

  return processed
}

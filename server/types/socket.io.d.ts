import { Socket as ClientSocket } from 'socket.io-client'

declare global {
  // Re-export the Socket type so it can be used in TypeScript
  namespace SocketIOClient {
    interface Socket extends ClientSocket {}
  }
}
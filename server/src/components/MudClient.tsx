import { useCallback, useEffect, useRef, useState } from 'react'
import { Alert, Button, Spinner, ToastContainer } from 'react-bootstrap'
// eslint-disable-next-line import/no-named-as-default
import io from 'socket.io-client'
import dynamic from 'next/dynamic'
import { useCharacters } from '../hooks/useCharacters'
import { authClient, useSession } from '../utils/auth-client'
import type { PortalEvent } from '../utils/portal-types'
import PortalNavbar from './PortalNavbar'
import type { TerminalHandle, TerminalSocket } from './Terminal'
import CharacterToast from './toasts/CharacterToast'
import EmailToast from './toasts/EmailToast'
import NoticeToast, { type Notice } from './toasts/NoticeToast'

// Dynamically import Terminal with no SSR
const Terminal = dynamic(() => import('./Terminal'), {
  ssr: false,
  // Use React Bootstrap Spinner for loading state
  loading: () => (
    <div className="portal-terminal-loading">
      <Spinner animation="border" variant="light" role="status">
        <span className="visually-hidden">Loading terminal...</span>
      </Spinner>
    </div>
  ),
})

interface MudClientProps {
  title: string
  /**
   * Whether the server saw a session on the request that rendered this page.
   * Better Auth cannot answer that until a round trip after the first paint,
   * and reports it as pending again on every re-check while nobody is signed
   * in, so this is what the page believes in the meantime.
   */
  signedInAtLoad: boolean
}

/** Which toast, if any, is covering the terminal. */
type Overlay = 'none' | 'email' | 'characters'

const MudClient = ({ title, signedInAtLoad }: MudClientProps) => {
  const [connected, setConnected] = useState(false)
  const [error, setError] = useState('')
  /**
   * Which state of the page the player last dismissed a toast in. The toast to
   * show is worked out from the session and the character list rather than
   * stored, so that signing in or out brings back the one that now applies -
   * and a dismissal only silences the situation it was made in.
   */
  const [dismissedFor, setDismissedFor] = useState<string | null>(null)
  const [notices, setNotices] = useState<Notice[]>([])
  const [activeCharacterId, setActiveCharacterId] = useState<string | undefined>()
  const socketRef = useRef<TerminalSocket | null>(null)
  const terminalRef = useRef<TerminalHandle | null>(null)
  const nextNoticeId = useRef(1)

  const { data: session, isPending: sessionPending, refetch } = useSession()
  const userId = session?.user?.id ?? null
  const signedIn = Boolean(userId)
  const { account, loading, refresh, forget } = useCharacters(userId)

  /**
   * What to believe while a session request is in flight. Better Auth derives
   * isPending from `data === null`, which is always true for a visitor who is
   * not signed in - so every re-check, including the one the browser makes on
   * its own each time the tab regains focus, reports pending again. Deciding
   * the overlay from that alone makes the sign-in toast blink, and takes the
   * address the player typed and the "check your inbox" line away with it.
   */
  const signedInOrPending = sessionPending ? signedInAtLoad : signedIn

  const overlayKey = userId ?? 'signed-out'
  const overlay: Overlay = resolveOverlay({
    dismissed: dismissedFor === overlayKey,
    signedIn: signedInOrPending,
    hasCharacters: Boolean(account && account.characters.length > 0),
  })
  const dismissOverlay = useCallback(() => setDismissedFor(overlayKey), [overlayKey])

  const notify = useCallback((notice: Omit<Notice, 'id'>) => {
    const id = nextNoticeId.current
    nextNoticeId.current += 1
    setNotices((current) => [...current, { ...notice, id }])
  }, [])

  const dismissNotice = useCallback((id: number) => {
    setNotices((current) => current.filter((notice) => notice.id !== id))
  }, [])

  // Initialize socket connection
  const initializeSocket = useCallback(() => {
    console.log('Initializing socket connection')
    // No setState here: this runs synchronously from the mount effect, and the
    // 'connect' handler below already clears the error once we are up.

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

    // A socket that has been replaced must not still be setting state: its
    // disconnect arrives after the replacement has connected, and would leave
    // the page showing an error over a perfectly live connection. React's
    // strict mode makes this the normal case, since it mounts, unmounts and
    // mounts again.
    const isCurrent = () => socketRef.current === socketInstance

    // Set up socket event handlers
    socketInstance.on('connect', () => {
      console.log('WebSocket connected')
      if (!isCurrent()) return
      setConnected(true)
      setError('')
    })

    socketInstance.on('status', (status: { connected: boolean }) => {
      if (!isCurrent()) return
      setConnected(status.connected)
      if (!status.connected) {
        setError('Connection to MUD server lost')
      }
    })

    socketInstance.on('disconnect', () => {
      console.log('WebSocket disconnected')
      if (!isCurrent()) return
      setConnected(false)
      setError('Connection to MUD server disconnected')
    })

    socketInstance.on('error', (err: { message?: string }) => {
      console.error('Socket error:', err)
      if (!isCurrent()) return
      setError(`Connection error: ${err.message || 'Unknown error'}`)
    })

    socketInstance.on('connect_error', (err: Error) => {
      console.error('Connection failed:', err)
      if (!isCurrent()) return
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

  const selectCharacter = useCallback(
    (characterId: string) => {
      setActiveCharacterId(characterId)
      dismissOverlay()
      socketRef.current?.emit('portal:login-as', characterId)
    },
    [dismissOverlay],
  )

  const addCharacter = useCallback(() => {
    setActiveCharacterId(undefined)
    dismissOverlay()
    socketRef.current?.emit('portal:new-character')
  }, [dismissOverlay])

  // What the bridge saw at the MUD's prompts. Only the server can know any of
  // this, because only the server sees the login conversation.
  useEffect(() => {
    const socket = socketRef.current
    if (!socket) return undefined

    const handler = (event: PortalEvent) => {
      switch (event.type) {
        case 'reconnecting':
          // The MUD connection is being replaced, so the previous session's
          // output would otherwise sit above the new login.
          terminalRef.current?.clear()
          break
        case 'character-saved':
          setActiveCharacterId(event.character.id)
          void refresh()
          if (event.isNew) {
            notify({
              title: 'Character saved',
              body: `${event.character.name} is on your account now, and will be in the menu next time.`,
              autohide: true,
            })
          }
          break
        case 'password-updated':
          void refresh()
          notify({
            title: 'Password updated',
            body: `The password saved for ${event.name} has been updated to match.`,
            autohide: true,
          })
          break
        case 'character-forgotten':
          setActiveCharacterId(undefined)
          void refresh()
          notify({
            title: 'Character deleted',
            body: `${event.name} was deleted in the game, so it has been removed from your account.`,
            autohide: true,
          })
          break
        case 'stored-password-rejected':
          setActiveCharacterId(undefined)
          notify({
            title: 'Saved password no longer works',
            body: `The game refused the saved password for ${event.name}. Type the current one at the prompt and it will be saved again.`,
            autohide: false,
          })
          break
        case 'character-missing':
          setActiveCharacterId(undefined)
          notify({
            title: `${event.name} is not in the game`,
            body: `The game has no character called ${event.name} any more. Answer the prompts to create it again, or forget it here.`,
            autohide: false,
            action: { label: 'Forget it', run: () => void forget(event.characterId) },
          })
          break
        case 'character-limit-reached':
          notify({
            title: 'Account is full',
            body: `This account already holds ${event.limit} characters, so ${event.name} was not saved. Remove one from the menu first.`,
            autohide: false,
          })
          break
        case 'login-failed':
          // A wrong password or a rejected name are part of ordinary typing:
          // the MUD says so on screen and asks again.
          if (event.reason === 'wrong-password' || event.reason === 'name-rejected') break
          notify({
            title: 'The game refused that login',
            body: describeFailure(event.reason),
            autohide: true,
          })
          break
        default:
          break
      }
    }

    socket.on('portal', handler)
    return () => {
      socket.off('portal', handler)
    }
  }, [connected, forget, notify, refresh])

  // A magic link usually opens in another tab, or on a phone, so this one
  // watches for the session appearing rather than asking the player to reload.
  //
  // It asks with getSession rather than refetch so that a poll which finds
  // nothing leaves the session alone: refetching reports pending, and the
  // toast the player is reading is decided from that.
  useEffect(() => {
    if (signedIn || overlay !== 'email') return undefined

    const poll = setInterval(() => {
      void (async () => {
        const { data } = await authClient.getSession()
        if (!data) return
        await refetch()
        // The bridge learns who is connected from the cookie on the WebSocket
        // handshake, so a socket opened while signed out stays anonymous for
        // as long as it lives - and would save none of the characters this
        // player is about to log in. Signing in replaces it, as signing out
        // already does.
        initializeSocket()
      })()
    }, 3000)

    return () => clearInterval(poll)
  }, [initializeSocket, overlay, refetch, signedIn])

  const signOutAndReset = useCallback(async () => {
    await authClient.signOut()
    setActiveCharacterId(undefined)
    await refetch()
    // The bridge decides what it may watch from the session on the handshake,
    // so a signed-out player needs a fresh socket to get the plain pipe back.
    initializeSocket()
    setDismissedFor(null)
  }, [initializeSocket, refetch])

  // Handler for reconnect button
  const handleReconnect = () => {
    setError('')
    initializeSocket()
  }

  return (
    <div className="portal-shell">
      <PortalNavbar
        title={title}
        signedIn={signedIn}
        email={session?.user?.email}
        image={session?.user?.image}
        characters={account?.characters ?? []}
        max={account?.max ?? 0}
        loading={loading}
        activeCharacterId={activeCharacterId}
        onSignIn={() => setDismissedFor(null)}
        onSignOut={() => void signOutAndReset()}
        onSelectCharacter={selectCharacter}
        onAddCharacter={addCharacter}
        onForgetCharacter={(characterId) => {
          if (characterId === activeCharacterId) setActiveCharacterId(undefined)
          void forget(characterId)
        }}
      />

      <div className="portal-stage">
        {/*
          Three states, not two: the first paint of every visit has no
          connection yet and nothing has gone wrong, and saying "disconnected"
          in red until the socket opens is its own kind of flicker. Every path
          that really does lose the MUD sets an error.
        */}
        {error ? (
          <div className="portal-disconnected">
            <Alert variant="danger" className="mb-3">
              {error}
            </Alert>
            <Button variant="primary" onClick={handleReconnect}>
              Reconnect
            </Button>
          </div>
        ) : connected ? (
          <Terminal socketRef={socketRef} handleRef={terminalRef} />
        ) : (
          <div className="portal-terminal-loading">
            <Spinner animation="border" variant="light" role="status">
              <span className="visually-hidden">Connecting to the game...</span>
            </Spinner>
          </div>
        )}

        <ToastContainer position="bottom-end" className="portal-toasts p-3">
          {overlay === 'email' && <EmailToast onDismiss={dismissOverlay} />}
          {overlay === 'characters' && account && (
            <CharacterToast
              account={account}
              onSelect={selectCharacter}
              onAdd={addCharacter}
              onDismiss={dismissOverlay}
            />
          )}
          {notices.map((notice) => (
            <NoticeToast key={notice.id} notice={notice} onDismiss={dismissNotice} />
          ))}
        </ToastContainer>
      </div>
    </div>
  )
}

/**
 * Which toast belongs over the terminal right now. A signed-out visitor is
 * asked for an email; somebody signed in with characters is offered them; a
 * signed-in player with none is left at the MUD's own prompt, which is where
 * a character gets onto the account in the first place.
 */
function resolveOverlay({
  dismissed,
  signedIn,
  hasCharacters,
}: {
  dismissed: boolean
  signedIn: boolean
  hasCharacters: boolean
}): Overlay {
  if (dismissed) return 'none'
  if (!signedIn) return 'email'
  return hasCharacters ? 'characters' : 'none'
}

/** Turns the MUD's reason for refusing a login into something readable. */
function describeFailure(reason: string): string {
  switch (reason) {
    case 'too-many-bad-passwords':
      return 'Too many wrong passwords, so the game closed the connection.'
    case 'multiplay-limit':
      return 'The game allows only so many characters online from one address at a time.'
    case 'mort-immort':
      return 'The game will not have an immortal and a mortal online from one address at once.'
    case 'site-restricted':
    case 'site-banned':
      return 'The game is not accepting connections from this address.'
    case 'wizlock':
      return 'The game is temporarily restricted. Try again later.'
    case 'timed-out':
      return 'The login prompt timed out.'
    case 'new-characters-banned':
    case 'creation-restricted':
      return 'The game is not accepting new characters at the moment.'
    case 'game-full':
      return 'The game is full right now. Try again in a moment.'
    case 'input-not-captured':
      return 'That used the game’s "!" or "^" shortcuts, so this login was not saved.'
    default:
      return 'The game closed the connection during login.'
  }
}

export default MudClient

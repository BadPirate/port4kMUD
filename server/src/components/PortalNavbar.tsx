import Container from 'react-bootstrap/Container'
import Dropdown from 'react-bootstrap/Dropdown'
import Navbar from 'react-bootstrap/Navbar'
import Spinner from 'react-bootstrap/Spinner'
import type { PortalCharacter } from '../utils/portal-types'

interface PortalNavbarProps {
  title: string
  /**
   * Taken from the session, not from the character list: the menu has to be
   * openable the moment somebody is signed in, whether or not their characters
   * have arrived yet, or ever.
   */
  signedIn: boolean
  email?: string
  image?: string | null
  characters: PortalCharacter[]
  max: number
  loading: boolean
  activeCharacterId?: string
  onSignIn: () => void
  onSignOut: () => void
  onSelectCharacter: (characterId: string) => void
  onAddCharacter: () => void
  onForgetCharacter: (characterId: string) => void
}

/** A plain head-and-shoulders glyph, for a visitor with no account yet. */
const AnonymousAvatar = () => (
  <svg width="32" height="32" viewBox="0 0 32 32" aria-hidden="true" focusable="false">
    <circle cx="16" cy="16" r="16" fill="#3a3f4b" />
    <circle cx="16" cy="12" r="5" fill="#9aa3b2" />
    <path d="M6 30c0-5.5 4.5-10 10-10s10 4.5 10 10z" fill="#9aa3b2" />
  </svg>
)

const PortalNavbar = ({
  title,
  signedIn,
  email,
  image,
  characters,
  max,
  loading,
  activeCharacterId,
  onSignIn,
  onSignOut,
  onSelectCharacter,
  onAddCharacter,
  onForgetCharacter,
}: PortalNavbarProps) => {
  const atLimit = max > 0 && characters.length >= max

  return (
    <Navbar bg="dark" variant="dark" className="portal-navbar px-3 py-2">
      <Container fluid className="px-0">
        <Navbar.Brand className="portal-title mb-0" data-testid="portal-title">
          {title}
        </Navbar.Brand>

        {/*
          Two shapes rather than one with a conditional handler: passing our
          own onClick to Dropdown.Toggle overrides the one that opens the menu,
          because react-bootstrap spreads caller props last.
        */}
        {signedIn ? (
          <Dropdown align="end">
            <Dropdown.Toggle
              as="button"
              type="button"
              className="portal-avatar-button"
              id="portal-account-menu"
              data-testid="portal-account-button"
              aria-label={`Account: ${email}`}
            >
              {image ? (
                // eslint-disable-next-line @next/next/no-img-element
                <img src={image} alt="" width={32} height={32} className="rounded-circle" />
              ) : (
                <AnonymousAvatar />
              )}
            </Dropdown.Toggle>

            <Dropdown.Menu className="portal-account-menu">
              <Dropdown.Header data-testid="portal-account-email">{email}</Dropdown.Header>
              <Dropdown.Divider />

              {loading && characters.length === 0 && (
                <div className="px-3 py-2 text-muted d-flex align-items-center gap-2">
                  <Spinner animation="border" size="sm" role="status" />
                  <span>Loading characters</span>
                </div>
              )}

              {!loading && characters.length === 0 && (
                <div className="px-3 py-2 text-muted portal-menu-hint">
                  No characters yet. Log in or create one at the prompt and it will appear here.
                </div>
              )}

              {characters.map((character) => (
                <div key={character.id} className="portal-character-row">
                  <Dropdown.Item
                    as="button"
                    type="button"
                    active={character.id === activeCharacterId}
                    onClick={() => onSelectCharacter(character.id)}
                    data-testid={`portal-character-${character.name}`}
                  >
                    {character.name}
                  </Dropdown.Item>
                  <button
                    type="button"
                    className="portal-forget-button"
                    // Portal-side only: the character itself stays in the game.
                    title={`Forget ${character.name} on this account`}
                    aria-label={`Forget ${character.name} on this account`}
                    data-testid={`portal-forget-${character.name}`}
                    onClick={(event) => {
                      event.stopPropagation()
                      onForgetCharacter(character.id)
                    }}
                  >
                    &times;
                  </button>
                </div>
              ))}

              {!atLimit && (
                <Dropdown.Item
                  as="button"
                  type="button"
                  onClick={onAddCharacter}
                  data-testid="portal-add-character"
                >
                  + Add character
                </Dropdown.Item>
              )}

              {atLimit && (
                <div className="px-3 py-2 text-muted portal-menu-hint">
                  {`That is all ${max} characters this account can hold.`}
                </div>
              )}

              <Dropdown.Divider />
              <Dropdown.Item as="button" type="button" onClick={onSignOut}>
                Sign out
              </Dropdown.Item>
            </Dropdown.Menu>
          </Dropdown>
        ) : (
          <button
            type="button"
            className="portal-avatar-button"
            data-testid="portal-account-button"
            aria-label="Sign in"
            onClick={onSignIn}
          >
            <AnonymousAvatar />
          </button>
        )}
      </Container>
    </Navbar>
  )
}

export default PortalNavbar

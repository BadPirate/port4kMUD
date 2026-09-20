import Button from 'react-bootstrap/Button'
import ListGroup from 'react-bootstrap/ListGroup'
import Toast from 'react-bootstrap/Toast'
import type { PortalAccount } from '../../utils/portal-types'

interface CharacterToastProps {
  account: PortalAccount
  onSelect: (characterId: string) => void
  onAdd: () => void
  onDismiss: () => void
}

/**
 * Offered on arrival when the account already has characters, so getting back
 * into the game is one click rather than a name and a password.
 *
 * Dismissing it leaves the MUD's own prompt underneath, which still works:
 * typing a name there is how a character gets onto the account in the first
 * place.
 */
const CharacterToast = ({ account, onSelect, onAdd, onDismiss }: CharacterToastProps) => {
  const atLimit = account.characters.length >= account.max

  return (
    <Toast onClose={onDismiss} data-testid="character-toast" className="portal-toast">
      <Toast.Header closeButton>
        <strong className="me-auto">Who are you playing?</strong>
      </Toast.Header>
      <Toast.Body>
        <ListGroup variant="flush" className="portal-character-list">
          {account.characters.map((character) => (
            <ListGroup.Item
              key={character.id}
              action
              as="button"
              type="button"
              onClick={() => onSelect(character.id)}
              data-testid={`character-toast-${character.name}`}
            >
              {character.name}
            </ListGroup.Item>
          ))}
        </ListGroup>

        {!atLimit && (
          <Button
            variant="link"
            size="sm"
            className="px-0 mt-1"
            onClick={onAdd}
            data-testid="character-toast-add"
          >
            + Add character
          </Button>
        )}
        {atLimit && (
          <p className="text-muted portal-menu-hint mt-2 mb-0">
            {`That is all ${account.max} characters this account can hold.`}
          </p>
        )}
      </Toast.Body>
    </Toast>
  )
}

export default CharacterToast

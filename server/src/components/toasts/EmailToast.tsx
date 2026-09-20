import { useState, type FormEvent } from 'react'
import Button from 'react-bootstrap/Button'
import Form from 'react-bootstrap/Form'
import Spinner from 'react-bootstrap/Spinner'
import Toast from 'react-bootstrap/Toast'
import { authClient } from '../../utils/auth-client'

interface EmailToastProps {
  onDismiss: () => void
}

/**
 * Asks for an email address and sends a sign-in link to it.
 *
 * It sits over the terminal rather than in front of it: the game is already
 * connected and playable underneath, and dismissing this leaves a visitor
 * exactly where they were before accounts existed.
 */
const EmailToast = ({ onDismiss }: EmailToastProps) => {
  const [email, setEmail] = useState('')
  const [sending, setSending] = useState(false)
  const [sent, setSent] = useState(false)
  const [error, setError] = useState('')

  const submit = async (event: FormEvent) => {
    event.preventDefault()
    if (!email || sending) return

    setSending(true)
    setError('')
    try {
      const { error: sendError } = await authClient.signIn.magicLink({
        email,
        callbackURL: '/',
      })
      if (sendError) {
        setError(sendError.message || 'Could not send the link. Please try again.')
        return
      }
      setSent(true)
    } catch (err) {
      console.error('Could not request a magic link:', err)
      setError('Could not send the link. Please try again.')
    } finally {
      setSending(false)
    }
  }

  return (
    <Toast onClose={onDismiss} data-testid="email-toast" className="portal-toast">
      <Toast.Header closeButton>
        <strong className="me-auto">Save your characters</strong>
      </Toast.Header>
      <Toast.Body>
        {sent ? (
          <div data-testid="email-toast-sent">
            <p className="mb-1">
              Click the magic link sent to <strong>{email}</strong> to sign in.
            </p>
            <p className="mb-0 text-muted portal-menu-hint">
              You can keep playing here while you wait.
            </p>
          </div>
        ) : (
          <Form onSubmit={submit}>
            <p className="mb-2">
              Sign in with your email to keep your characters on this account. No password needed.
            </p>
            <Form.Group controlId="portal-email">
              <Form.Label visuallyHidden>Email address</Form.Label>
              <Form.Control
                type="email"
                value={email}
                required
                placeholder="you@example.com"
                autoComplete="email"
                data-testid="email-toast-input"
                onChange={(event) => setEmail(event.target.value)}
              />
            </Form.Group>
            {error && (
              <p className="text-danger mt-2 mb-0" role="alert">
                {error}
              </p>
            )}
            <div className="d-flex gap-2 mt-2">
              <Button type="submit" size="sm" disabled={sending} data-testid="email-toast-submit">
                {sending ? <Spinner animation="border" size="sm" role="status" /> : 'Send link'}
              </Button>
              <Button type="button" size="sm" variant="link" onClick={onDismiss}>
                Play as a guest
              </Button>
            </div>
          </Form>
        )}
      </Toast.Body>
    </Toast>
  )
}

export default EmailToast

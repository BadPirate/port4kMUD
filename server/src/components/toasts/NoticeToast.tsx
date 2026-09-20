import Button from 'react-bootstrap/Button'
import Toast from 'react-bootstrap/Toast'

export interface Notice {
  id: number
  title: string
  body: string
  /** Notices that need an answer stay up until the player deals with them. */
  autohide: boolean
  action?: { label: string; run: () => void }
}

interface NoticeToastProps {
  notice: Notice
  onDismiss: (id: number) => void
}

/** One thing the bridge noticed at the MUD's prompts, said plainly. */
const NoticeToast = ({ notice, onDismiss }: NoticeToastProps) => (
  <Toast
    onClose={() => onDismiss(notice.id)}
    autohide={notice.autohide}
    delay={6000}
    show
    data-testid="portal-notice"
    className="portal-toast"
  >
    <Toast.Header closeButton>
      <strong className="me-auto">{notice.title}</strong>
    </Toast.Header>
    <Toast.Body>
      <p className="mb-0">{notice.body}</p>
      {notice.action && (
        <Button
          variant="link"
          size="sm"
          className="px-0 mt-1"
          data-testid="portal-notice-action"
          onClick={() => {
            notice.action?.run()
            onDismiss(notice.id)
          }}
        >
          {notice.action.label}
        </Button>
      )}
    </Toast.Body>
  </Toast>
)

export default NoticeToast

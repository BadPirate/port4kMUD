import { render, screen } from '@testing-library/react'
import WelcomeCard from '../src/cards/WelcomeCard'

describe('WelcomeCard', () => {
  it('renders the welcome card', () => {
    render(<WelcomeCard />)

    const titleElement = screen.getByTestId('welcome-title')
    expect(titleElement).toBeInTheDocument()

    // Verify it has welcome text (more flexible than checking exact content)
    expect(titleElement.textContent).toContain('Welcome to')
  })
})

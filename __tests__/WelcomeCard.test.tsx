import { render, screen } from '@testing-library/react'
import WelcomeCard from '../src/cards/WelcomeCard'

describe('WelcomeCard', () => {
  it('renders the welcome message', () => {
    render(<WelcomeCard />)
    const titleElement = screen.getByText(/Welcome to Campfire/i)
    const textElement = screen.getByText(/Come and sit for a while!/i)

    expect(titleElement).toBeInTheDocument()
    expect(textElement).toBeInTheDocument()
  })
})

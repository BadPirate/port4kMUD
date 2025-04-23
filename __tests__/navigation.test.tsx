
import { render, screen } from '@testing-library/react'
import { SessionProvider } from 'next-auth/react'
import RootNav from '../src/components/RootNav'

describe('Navigation', () => {
  test('navbar displays app name', () => {
    render(<RootNav>Test</RootNav>)
    const brand = screen.getByRole('link', { name: /nextstrap/i })
    expect(brand).toBeInTheDocument()
  })

  test('shows login button when unauthenticated', () => {
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )
    const loginBtn = screen.getByRole('button', { name: /login/i })
    expect(loginBtn).toBeInTheDocument()
  })

  test('shows logout button when authenticated', () => {
    const mockSession = {
      user: { name: 'Test User' },
      expires: new Date(Date.now() + 3600 * 1000).toISOString()
    }
    render(
      <SessionProvider session={mockSession}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )
    const userBtn = screen.getByRole('button', { name: /test user/i })
    expect(userBtn).toBeInTheDocument()
  })
})

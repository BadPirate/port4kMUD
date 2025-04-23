
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { signIn } from 'next-auth/react'
import { SessionProvider } from 'next-auth/react'
import RootNav from '../src/components/RootNav'

jest.mock('next-auth/react', () => ({
  ...jest.requireActual('next-auth/react'),
  signIn: jest.fn()
}))

describe('Authentication', () => {
  test('allows login via credentials', async () => {
    const mockSignIn = signIn as jest.Mock
    mockSignIn.mockResolvedValueOnce({ ok: true })
    
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )

    const loginBtn = screen.getByRole('button', { name: /login/i })
    fireEvent.click(loginBtn)

    const emailInput = screen.getByRole('textbox', { name: /email/i })
    const submitBtn = screen.getByRole('button', { name: /sign in/i })

    fireEvent.change(emailInput, { target: { value: 'test@example.com' } })
    fireEvent.click(submitBtn)

    await waitFor(() => {
      expect(mockSignIn).toHaveBeenCalledWith('credentials', {
        email: 'test@example.com',
        callbackUrl: '/'
      })
    })
  })
})

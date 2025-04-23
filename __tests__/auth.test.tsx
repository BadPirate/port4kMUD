
import { render, screen, fireEvent } from '@testing-library/react'
import { signIn } from 'next-auth/react'
import { SessionProvider } from 'next-auth/react'
import RootNav from '../src/components/RootNav'

jest.mock('next-auth/react', () => ({
  ...jest.requireActual('next-auth/react'),
  signIn: jest.fn(),
  useSession: () => ({ data: null, status: 'unauthenticated' })
}))

describe('Authentication', () => {
  test('allows login via button click', async () => {
    const mockSignIn = signIn as jest.Mock
    
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )

    const loginBtn = screen.getByRole('button', { name: /login/i })
    fireEvent.click(loginBtn)

    expect(mockSignIn).toHaveBeenCalledWith(
      expect.any(String),
      expect.objectContaining({
        callbackUrl: '/'
      })
    )
  })
})

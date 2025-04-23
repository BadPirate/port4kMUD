
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
  beforeEach(() => {
    process.env.NEXT_PUBLIC_TEST_MODE = 'false'
  })

  afterEach(() => {
    delete process.env.NEXT_PUBLIC_TEST_MODE
    jest.clearAllMocks()
  })

  test('allows login via button click in normal mode', async () => {
    const mockSignIn = signIn as jest.Mock
    
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )



  test('allows login via button click in test mode', async () => {
    const mockSignIn = signIn as jest.Mock
    process.env.NEXT_PUBLIC_TEST_MODE = 'true'
    
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>
    )

    const loginBtn = screen.getByRole('button', { name: /login/i })
    fireEvent.click(loginBtn)

    expect(mockSignIn).toHaveBeenCalledWith(
      'email',
      expect.objectContaining({
        callbackUrl: '/'
      })
    )
  })

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

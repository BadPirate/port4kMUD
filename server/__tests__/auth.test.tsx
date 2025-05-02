import { render, screen, fireEvent } from '@testing-library/react'
import { signIn } from 'next-auth/react'
import { SessionProvider } from 'next-auth/react'
import RootNav from '../src/components/RootNav'
import config from '../src/utils/config'

jest.mock('next-auth/react', () => ({
  ...jest.requireActual('next-auth/react'),
  signIn: jest.fn(),
  useSession: () => ({ data: null, status: 'unauthenticated' }),
}))

// Mock the config object for tests
jest.mock('../src/utils/config', () => {
  const originalConfig = jest.requireActual('../src/utils/config').default
  return {
    __esModule: true,
    default: {
      ...originalConfig,
      NEXT_PUBLIC_TEST_MODE: false,
    },
  }
})

describe('Authentication', () => {
  beforeEach(() => {
    // Update the mocked config values
    config.NEXT_PUBLIC_TEST_MODE = false
  })

  afterEach(() => {
    jest.clearAllMocks()
  })

  test('allows login via button click in normal mode', async () => {
    const mockSignIn = signIn as jest.Mock

    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )

    const loginBtn = screen.getByRole('button', { name: /login/i })
    fireEvent.click(loginBtn)

    expect(mockSignIn).toHaveBeenCalledWith(
      expect.any(String),
      expect.objectContaining({
        callbackUrl: '/',
      }),
    )
  })

  test('allows login via button click in test mode', async () => {
    const mockSignIn = signIn as jest.Mock
    config.NEXT_PUBLIC_TEST_MODE = true

    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )

    const loginBtn = screen.getByRole('button', { name: /login/i })
    fireEvent.click(loginBtn)

    expect(mockSignIn).toHaveBeenCalledWith(
      'email',
      expect.objectContaining({
        callbackUrl: '/',
      }),
    )
  })
})

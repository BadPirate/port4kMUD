import { render, screen } from '@testing-library/react'
import { SessionProvider } from 'next-auth/react'
import RootNav from '../src/components/RootNav'
import _config from '../src/utils/config'

// Mock the config object for tests
jest.mock('../src/utils/config', () => {
  const originalConfig = jest.requireActual('../src/utils/config').default
  return {
    __esModule: true,
    default: {
      ...originalConfig,
      NEXT_PUBLIC_APP_NAME: 'nextstrap',
      NEXT_PUBLIC_TEST_MODE: false,
    },
  }
})

describe('Navigation', () => {
  afterEach(() => {
    jest.clearAllMocks()
  })

  test('navbar displays default app name when not set', () => {
    // No need for spyOn, the mock above already sets NEXT_PUBLIC_APP_NAME to 'nextstrap'
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )
    const brand = screen.getByRole('link', { name: /nextstrap/i })
    expect(brand).toBeInTheDocument()
  })

  test('navbar displays capitalized app name', () => {
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )
    const brand = screen.getByRole('link', { name: /nextstrap/i })
    expect(brand).toBeInTheDocument()
  })

  test('shows login button when unauthenticated', () => {
    render(
      <SessionProvider session={null}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )
    const loginBtn = screen.getByRole('button', { name: /login/i })
    expect(loginBtn).toBeInTheDocument()
  })

  test('shows logout button when authenticated', () => {
    const mockSession = {
      user: { name: 'Test User' },
      expires: new Date(Date.now() + 3600 * 1000).toISOString(),
    }
    render(
      <SessionProvider session={mockSession}>
        <RootNav>Test</RootNav>
      </SessionProvider>,
    )
    const userBtn = screen.getByRole('button', { name: /test user/i })
    expect(userBtn).toBeInTheDocument()
  })
})

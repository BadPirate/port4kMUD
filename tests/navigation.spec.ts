import { test, expect } from '@playwright/test'

test('navbar is visible and displays app name', async ({ page }) => {
  // Navigate to the root page
  await page.goto('/')

  // The brand element should be visible and display the app name
  const brand = page.locator('nav.navbar .navbar-brand')
  // Wait for navbar-brand to become visible (remove default FOUC hide)
  await page.waitForSelector('nav.navbar .navbar-brand', { state: 'visible', timeout: 10000 })
  await expect(brand).toBeVisible()

  // Determine expected app name (capitalized)
  const rawName = process.env.NEXT_PUBLIC_APP_NAME || 'nextstrap'
  const expected = rawName.charAt(0).toUpperCase() + rawName.slice(1)
  await expect(brand).toHaveText(expected, { useInnerText: true })
})

test.describe('authentication UI', () => {
  test('shows login button when unauthenticated', async ({ page }) => {
    // Mock session as unauthenticated
    // Mock NextAuth session endpoint for unauthenticated user
    await page.route('**/api/auth/session**', (route) => route.fulfill({
      status: 200,
      contentType: 'application/json',
      // NextAuth returns null for no session
      body: JSON.stringify(null),
    }))
    await page.goto('/')
    const loginBtn = page.getByRole('button', { name: 'Login' })
    await expect(loginBtn).toBeVisible()
  })

  test('shows logout button when authenticated', async ({ page }) => {
    // Mock session as authenticated with a dummy user
    const fakeSession = {
      user: { name: 'Test User' },
      expires: new Date(Date.now() + 3600 * 1000).toISOString(),
    }
    // Mock NextAuth session endpoint for authenticated user
    await page.route('**/api/auth/session**', (route) => route.fulfill({
      status: 200,
      contentType: 'application/json',
      // NextAuth returns the session object
      body: JSON.stringify(fakeSession),
    }))
    await page.goto('/')
    const logoutBtn = page.getByRole('button', { name: 'Test User' })
    await expect(logoutBtn).toBeVisible()
  })
})

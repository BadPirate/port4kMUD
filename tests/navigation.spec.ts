import { test, expect } from '@playwright/test'

test('navbar is visible and displays app name', async ({ page }) => {
  // Navigate to the root page
  await page.goto('/')

  // The brand element should be visible and display the app name
  const brand = page.locator('nav.navbar .navbar-brand')
  await expect(brand).toBeVisible()

  // Determine expected app name (capitalized)
  const rawName = process.env.NEXT_PUBLIC_APP_NAME || 'nextstrap'
  const expected = rawName.charAt(0).toUpperCase() + rawName.slice(1)
  await expect(brand).toHaveText(expected, { useInnerText: true })
})

test.describe('authentication UI', () => {
  test('shows login button when unauthenticated', async ({ page }) => {
    // Mock session as unauthenticated
    await page.route('**/api/auth/session', (route) => route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ status: 'unauthenticated', data: null }),
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
    await page.route('**/api/auth/session', (route) => route.fulfill({
      status: 200,
      contentType: 'application/json',
      body: JSON.stringify({ status: 'authenticated', data: fakeSession }),
    }))
    await page.goto('/')
    const logoutBtn = page.getByRole('button', { name: 'Test User' })
    await expect(logoutBtn).toBeVisible()
  })
})

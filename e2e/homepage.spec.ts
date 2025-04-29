import { test, expect } from '@playwright/test'

test('homepage displays welcome message', async ({ page }) => {
  // Navigate to the homepage
  await page.goto('/')

  // Wait for the welcome card title to be visible using the test ID
  await page.waitForSelector('[data-testid="welcome-title"]')

  // Verify the welcome title exists and contains welcome text (more flexible approach)
  const title = await page.textContent('[data-testid="welcome-title"]')
  expect(title).toContain('Welcome to')
})

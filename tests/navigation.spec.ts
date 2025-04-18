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

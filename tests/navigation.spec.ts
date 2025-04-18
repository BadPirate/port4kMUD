import { test, expect } from '@playwright/test'

test('navbar is visible and displays app name', async ({ page }) => {
  // Navigate to the root page
  await page.goto('/')

  // The navigation bar should be present
  const navbar = page.getByRole('navigation')
  await expect(navbar).toBeVisible()

  // The app name (from NEXT_PUBLIC_APP_NAME) should be displayed
  const appName = process.env.NEXT_PUBLIC_APP_NAME || 'nextstrap'
  await expect(navbar).toContainText(appName, { useInnerText: true })
})

import { test, expect } from '@playwright/test'

test.describe('Authentication UI elements', () => {
  test('login button is displayed in the navigation bar', async ({ page }) => {
    // Navigate to the homepage
    await page.goto('/')

    // Verify login button is visible in the navigation bar
    const loginButton = await page.getByRole('button', { name: /login/i })
    await expect(loginButton).toBeVisible()

    // Verify the app name is also displayed in the navigation bar
    const appName = await page.getByRole('link', { name: /nextstrap/i })
    await expect(appName).toBeVisible()
  })

  test('login button is correctly positioned in the navbar', async ({ page }) => {
    // Navigate to the homepage
    await page.goto('/')

    // Find the login button
    const loginButton = await page.getByRole('button', { name: /login/i })

    // Get the bounding box of the login button to verify its position
    const buttonBox = await loginButton.boundingBox()
    expect(buttonBox).not.toBeNull()

    // Verify the button is in the upper portion of the page (nav bar)
    if (buttonBox) {
      expect(buttonBox.y).toBeLessThan(100) // Button should be in the top part of page
      expect(buttonBox.x).toBeGreaterThan(100) // Button should not be at the far left
    }
  })

  test('login button has correct styling', async ({ page }) => {
    // Navigate to the homepage
    await page.goto('/')

    // Find the login button
    const loginButton = await page.getByRole('button', { name: /login/i })

    // Check that it has the appropriate bootstrap styling
    const hasBootstrapClass = await loginButton.evaluate((el) => {
      return el.classList.contains('btn')
    })

    expect(hasBootstrapClass).toBeTruthy()
  })
})

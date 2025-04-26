import { test, expect } from '@playwright/test'

test('homepage displays welcome message', async ({ page }) => {
  // Navigate to the homepage
  await page.goto('/')

  // Wait for the welcome card title to be visible
  await page.waitForSelector('div.card-title')

  // Verify the welcome card title and text are displayed
  const title = await page.textContent('div.card-title')
  expect(title).toContain('Welcome to Campfire')
})

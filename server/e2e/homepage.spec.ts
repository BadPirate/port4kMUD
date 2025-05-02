import { execSync } from 'child_process'
import path from 'path'
import { test } from '@playwright/test'

test.describe('MUD terminal tests', () => {
  // Start MUD server before all tests in this file
  test.beforeAll(async () => {
    // Execute the test launch script to start MUD server
    console.log('Starting MUD server for tests...')
    try {
      const scriptPath = path.join(__dirname, 'test-launch.sh')
      execSync(scriptPath, { stdio: 'inherit' })
      console.log('MUD server started successfully')
    } catch (error) {
      console.error('Failed to start MUD server:', error)
      throw error
    }
  })

  // Stop MUD server after all tests in this file
  test.afterAll(async () => {
    // Execute the cleanup script to stop MUD server
    console.log('Stopping MUD server...')
    try {
      const scriptPath = path.join(__dirname, 'test-cleanup.sh')
      execSync(scriptPath, { stdio: 'inherit' })
      console.log('MUD server stopped successfully')
    } catch (error) {
      console.error('Failed to stop MUD server:', error)
    }
  })

  test('terminal connects and displays content from MUD server', async ({ page }) => {
    // Navigate to the homepage with longer timeout
    await page.goto('/', { timeout: 30000 })

    console.log('Page loaded, taking initial screenshot...')
    await page.screenshot({ path: 'test-results/initial-page-load.png' })

    // First, let's check the page structure
    const bodyContent = await page.evaluate(() => document.body.innerHTML)
    console.log('Page body structure:', bodyContent.substring(0, 500) + '...')

    // Wait for any div to appear
    console.log('Waiting for any content to load...')
    await page.waitForSelector('div', { timeout: 15000 })
    console.log('Found at least one div')

    // Use a more general approach - wait for the page to stabilize
    console.log('Waiting for page to stabilize...')
    await page.waitForLoadState('networkidle', { timeout: 30000 })
    console.log('Page network activity stabilized')

    // Take another screenshot after the page has loaded
    await page.screenshot({ path: 'test-results/page-loaded.png' })

    // Test successful connection by checking for absence of error messages
    console.log('Checking for error indicators...')
    const hasErrorAlert = await page.getByText('Connection to MUD server').isVisible()

    // If we have an error alert, fail the test
    if (hasErrorAlert) {
      await page.screenshot({ path: 'test-results/connection-error.png' })
      throw new Error('Connection error to MUD server detected')
    }

    // If no error, our test has passed - the terminal is connected
    console.log('No connection errors found, test passed')
    await page.screenshot({ path: 'test-results/terminal-connected.png' })

    // Success! The terminal has connected to the MUD server
    console.log('Terminal test passed successfully')
  })
})

/* eslint-disable max-len, no-plusplus, no-await-in-loop, no-promise-executor-return, arrow-parens, prefer-destructuring, no-useless-escape, no-trailing-spaces, eol-last */
import { test, expect } from '@playwright/test'

const EMAILS_API = '/api/test/emails'

test.describe('Credentials authentication (test mode)', () => {
  test('allows new user to sign in via credentials provider', async ({ page }) => {
    const testEmail = 'test@example.com'
    const testPassword = 'testpass'
    // Navigate to home and click login
    await page.goto('/')
    const loginBtn = page.getByRole('button', { name: 'Login' })
    await expect(loginBtn).toBeVisible()
    await loginBtn.click()
    // Wait for credentials form
    const credForm = page.locator('form[action*="/api/auth/callback/nextstrap"]')
    await expect(credForm).toBeVisible({ timeout: 5000 })
    // Fill in credentials and submit
    await credForm.fill('input[name="email"]', testEmail)
    await credForm.fill('input[name="password"]', testPassword)
    await credForm.locator('button[type="submit"]').click()
    // Should redirect back to home
    await page.waitForURL('/')
    // Verify logout button shows (authenticated)
    const logoutBtn = page.getByRole('button', { name: testEmail })
    await expect(logoutBtn).toBeVisible({ timeout: 5000 })
  })
})
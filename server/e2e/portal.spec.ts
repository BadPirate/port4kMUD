import { execSync } from 'child_process'
import path from 'path'
import { expect, test, type Page } from '@playwright/test'

/**
 * The portal's account flow, end to end: sign in by email, create a character
 * at the MUD's own prompt, and log back in as it with one click.
 *
 * Nothing here stubs the game. The MUD is the real bin/circle, and the
 * characters this creates land in its player file (mud/lib/etc/players, which
 * is gitignored), because the only way onto an account is through the login
 * conversation the bridge watches.
 */

const screen = (page: Page) => page.locator('.xterm-rows')

/**
 * A name the MUD will accept. _parse_name allows letters only, two to twenty
 * of them, and Valid_Name rejects anything containing a word from
 * mud/lib/misc/xnames. Every entry in that list contains a vowel, so a
 * consonant-only suffix cannot collide with one however the dice fall.
 */
function uniqueCharacterName(): string {
  const consonants = 'bcdfghjklmnpqrstvwxyz'
  let name = 'Portal'
  for (let i = 0; i < 6; i += 1) {
    name += consonants[Math.floor(Math.random() * consonants.length)]
  }
  return name
}

/** Waits for a prompt, then answers it as a player would. */
async function answer(page: Page, prompt: string, reply: string) {
  await expect(screen(page)).toContainText(prompt, { timeout: 30000 })
  await page.keyboard.type(reply)
}

test.describe('portal accounts', () => {
  test.beforeAll(() => {
    console.log('Starting MUD server for tests...')
    execSync(path.join(__dirname, 'test-launch.sh'), { stdio: 'inherit' })
  })

  test.afterAll(() => {
    console.log('Stopping MUD server...')
    try {
      execSync(path.join(__dirname, 'test-cleanup.sh'), { stdio: 'inherit' })
    } catch (error) {
      console.error('Failed to stop MUD server:', error)
    }
  })

  test('signs in by email, saves a character and logs back in as it', async ({ page }) => {
    const name = uniqueCharacterName()
    const email = `e2e-${Date.now()}@example.com`
    const password = 'e2epass1'

    await page.goto('/')

    // Signed out, the game is already playable and the toast only offers.
    await expect(screen(page)).toContainText('Character Login:', { timeout: 30000 })
    await expect(page.getByTestId('email-toast')).toBeVisible()

    await page.getByTestId('email-toast-input').fill(email)
    await page.getByTestId('email-toast-submit').click()
    await expect(page.getByTestId('email-toast-sent')).toBeVisible()

    // Without SMTP configured the mailer keeps the link for this route rather
    // than sending it, which is what makes the flow testable at all.
    const captured = await page.request.get('/api/portal/test/magic-links')
    const { emails } = await captured.json()
    const last = emails[emails.length - 1]
    expect(last.identifier).toBe(email)

    await page.goto(last.url)
    await expect(screen(page)).toContainText('Character Login:', { timeout: 30000 })

    await page.getByTestId('portal-account-button').click()
    await expect(page.getByTestId('portal-account-email')).toHaveText(email)
    await page.keyboard.press('Escape')

    // Create a character the ordinary way, at the game's prompts.
    await page.locator('.xterm').click()
    await page.keyboard.type(`${name}\r`)
    await answer(page, 'Did I get that right', 'Y\r')
    await answer(page, 'Give me a password', `${password}\r`)
    await answer(page, 'retype password', `${password}\r`)
    await answer(page, 'gender', 'M\r')
    await answer(page, 'Race:', '1\r')
    await answer(page, 'Class:', 'w\r')
    await answer(page, 'roll your stats', '\r')
    await answer(page, 'Keep these stats', 'y\r')
    await expect(screen(page)).toContainText('PRESS RETURN', { timeout: 30000 })

    // The bridge saw the login succeed and put it on the account.
    await expect(page.getByTestId('portal-notice')).toContainText(name)
    await page.getByTestId('portal-account-button').click()
    await expect(page.getByTestId(`portal-character-${name}`)).toBeVisible()
    await page.keyboard.press('Escape')

    // Coming back, one click logs in as it: the bridge types the name and the
    // stored password at the prompts, and the password never reaches the page.
    await page.reload()
    await expect(page.getByTestId('character-toast')).toBeVisible({ timeout: 30000 })
    await page.getByTestId(`character-toast-${name}`).click()
    await expect(screen(page)).toContainText('PRESS RETURN', { timeout: 30000 })
    await expect(screen(page)).toContainText(name)
    await expect(screen(page)).not.toContainText(password)

    // Forgetting it is portal-side only; the character stays in the game.
    await page.getByTestId('portal-account-button').click()
    await page.getByTestId(`portal-forget-${name}`).click()
    await expect(page.getByTestId(`portal-character-${name}`)).toHaveCount(0)

    const account = await (await page.request.get('/api/portal/characters')).json()
    expect(account.characters).toHaveLength(0)
  })

  test('keeps the sign-in toast up while the session is re-checked', async ({ page }) => {
    const email = `e2e-flicker-${Date.now()}@example.com`

    await page.goto('/')
    await expect(page.getByTestId('email-toast')).toBeVisible({ timeout: 30000 })

    await page.getByTestId('email-toast-input').fill(email)
    await page.getByTestId('email-toast-submit').click()
    await expect(page.getByTestId('email-toast-sent')).toContainText(email)

    // Two things re-check the session behind this toast: the page's own poll,
    // watching for a link clicked on another device, and the browser itself
    // every time the tab comes back - which is precisely what a player does on
    // the way to their inbox and back. Better Auth calls a session it has not
    // got yet "pending" on every one of those, so deciding what to show from
    // that alone took this message, and the address they had typed, away with
    // it several times a minute.
    await page.evaluate(() => document.dispatchEvent(new Event('visibilitychange')))
    await page.waitForTimeout(8000)
    await page.evaluate(() => document.dispatchEvent(new Event('visibilitychange')))

    await expect(page.getByTestId('email-toast-sent')).toContainText(email)
    await expect(page.getByTestId('email-toast')).toBeVisible()
  })

  test('a guest can dismiss the toast and still use the game', async ({ page }) => {
    await page.goto('/')
    await expect(page.getByTestId('email-toast')).toBeVisible({ timeout: 30000 })

    await page.getByRole('button', { name: 'Play as a guest' }).click()
    await expect(page.getByTestId('email-toast')).toHaveCount(0)

    await page.locator('.xterm').click()
    await page.keyboard.type('Nosuchperson\r')
    await expect(screen(page)).toContainText('Did I get that right')
  })
})

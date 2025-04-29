import path from 'path'
import getPort from 'get-port'
import { defineConfig, devices, PlaywrightTestConfig } from '@playwright/test'

const configPromise: Promise<PlaywrightTestConfig> = (async () => {
  const PORT = process.env.PORT ? Number(process.env.PORT) : await getPort()
  process.env.PORT = String(PORT)
  const baseURL = `http://localhost:${PORT}`

  return defineConfig({
    timeout: 30 * 1000,
    testDir: path.join(__dirname, 'e2e'),
    retries: 2,
    outputDir: 'test-results/',
    webServer: {
      command: 'yarn dev',
      url: baseURL,
      timeout: 120 * 1000,
      reuseExistingServer: !process.env.CI,
      env: { PORT: String(PORT) },
    },
    globalSetup: require.resolve('./scripts/setup-test-db.js'),
    use: {
      baseURL,
      trace: 'retry-with-trace',
      headless: true, // Run tests in headless mode (no browser UI)
    },
    projects: [
      {
        name: 'Desktop Chrome',
        use: { ...devices['Desktop Chrome'] },
      },
    ],
  })
})()

export default configPromise

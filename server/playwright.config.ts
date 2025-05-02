import path from 'path'
import getPort from 'get-port'
import { defineConfig, devices, PlaywrightTestConfig } from '@playwright/test'
import config from './src/utils/config'

const configPromise: Promise<PlaywrightTestConfig> = (async () => {
  const PORT = config.PORT ? Number(config.PORT) : await getPort()
  // We can't avoid this direct assignment as it needs to be available for other processes
  // eslint-disable-next-line no-restricted-syntax
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
      reuseExistingServer: !config.CI,
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

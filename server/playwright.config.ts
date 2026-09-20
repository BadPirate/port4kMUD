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
    timeout: 90 * 1000,
    testDir: path.join(__dirname, 'e2e'),
    retries: 2,
    outputDir: 'test-results/',
    webServer: {
      command: 'yarn dev',
      url: baseURL,
      timeout: 180 * 1000,
      reuseExistingServer: !config.CI,
      env: {
        PORT: String(PORT),
        // A throwaway accounts database, so a test run never touches the real
        // one under mud/lib.
        AUTH_DATABASE_PATH: path.join(__dirname, 'test-results', 'portal-e2e.sqlite'),
        // Any stable value will do: it only has to outlive the run.
        BETTER_AUTH_SECRET: 'port4k-e2e-secret-not-for-production-0123456789',
        // BETTER_AUTH_URL is deliberately unset, so magic links are built from
        // the request and inherit whichever port this run picked.
      },
    },
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

/* eslint-disable import/no-extraneous-dependencies, semi, eol-last */
import { defineConfig } from '@playwright/test'

const isDocker = process.env.TEST_MODE === 'true'
const baseURL = isDocker
  ? 'http://test-server:3000'
  : 'http://localhost:3000'

export default defineConfig({
  testDir: 'tests',
  timeout: 30 * 1000,
  outputDir: 'test-results/',
  use: {
    baseURL,
    headless: true,
    viewport: { width: 1280, height: 720 },
    ignoreHTTPSErrors: true,
    video: 'retain-on-failure',
  },
})
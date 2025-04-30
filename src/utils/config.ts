import dotenv from 'dotenv'
// Import package.json directly
// eslint-disable-next-line import/extensions
import packageJson from '../../package.json'

// Only load dotenv in server environment
if (typeof window === 'undefined') {
  dotenv.config()
}

const requireEnv = <T extends Record<string, string | undefined>, U = string>(
  values: T,
  type?: (value: string) => U,
): { [K in keyof T]: U } => {
  const result: Partial<{ [K in keyof T]: U }> = {}

  for (const key in values) {
    const value = process.env[key] || values[key]
    if (!value) {
      throw new Error(`Missing required environment variable: ${key}`)
    }
    const typedValue = type ? type(value) : (value as unknown as U)
    if (type) {
      const convertedValue = type(value)
      if (typeof typedValue !== typeof convertedValue) {
        throw new Error(`Environment variable ${key} is not of the expected type`)
      }
    }
    result[key] = typedValue
  }

  return result as { [K in keyof T]: U }
}

const env = requireEnv({
  NODE_ENV: 'development',
  DATABASE_URL: 'file:./dev.db',
})

// Optional environment variables with defaults
const optionalEnv = {
  PORT: process.env.PORT,
  CI: process.env.CI === 'true',
  // Auth-related environment variables
  NEXTAUTH_SECRET: process.env.NEXTAUTH_SECRET || 'test-secret',
  // OAuth provider settings
  GARAGE_AUTH_CLIENT_ID: process.env.GARAGE_AUTH_CLIENT_ID || '',
  GARAGE_AUTH_CLIENT_SECRET: process.env.GARAGE_AUTH_CLIENT_SECRET || '',
  // Email provider settings
  SMTP_HOST: process.env.SMTP_HOST || 'localhost',
  SMTP_PORT: process.env.SMTP_PORT || '1025',
  SMTP_USER: process.env.SMTP_USER || '',
  SMTP_PASS: process.env.SMTP_PASS || '',
  EMAIL_FROM: process.env.EMAIL_FROM || 'nextstrap@example.com',
  // Testing modes
  TEST_MODE: process.env.TEST_MODE === 'true',
  NEXT_PUBLIC_TEST_MODE: process.env.NEXT_PUBLIC_TEST_MODE === 'true',
}

// Use values directly from package.json
const config = {
  NEXT_PUBLIC_APP_NAME: packageJson.name,
  NEXT_PUBLIC_APP_VERSION: packageJson.version,
  ...env,
  ...optionalEnv,
}

export default config

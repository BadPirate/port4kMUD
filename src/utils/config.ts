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
    if (type && typeof typedValue !== typeof type(value)) {
      throw new Error(`Environment variable ${key} is not of the expected type`)
    }
    result[key] = typedValue
  }

  return result as { [K in keyof T]: U }
}

const env = requireEnv({
  NODE_ENV: 'development',
  DATABASE_URL: 'file:./dev.db',
})

// Use values directly from package.json
const config = {
  NEXT_PUBLIC_APP_NAME: packageJson.name,
  NEXT_PUBLIC_APP_VERSION: packageJson.version,
  ...env,
}

export default config

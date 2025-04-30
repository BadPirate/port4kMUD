import dotenv from 'dotenv'
// Import package.json directly
// eslint-disable-next-line import/extensions
import packageJson from '../../package.json'

// Only load dotenv in server environment
if (typeof window === 'undefined') {
  dotenv.config()
}

const required = {
  NODE_ENV: process.env.NODE_ENV,
  DATABASE_URL: process.env.DATABASE_URL,
  GARAGE_AUTH_CLIENT_ID: process.env.GARAGE_AUTH_CLIENT_ID,
  GARAGE_AUTH_CLIENT_SECRET: process.env.GARAGE_AUTH_CLIENT_SECRET,
  NEXTAUTH_SECRET: process.env.NEXTAUTH_SECRET,
}

const optional = {
  PORT: process.env.PORT,
  CI: process.env.CI,

  // Email provider settings
  SMTP_HOST: process.env.SMTP_HOST,
  SMTP_PORT: process.env.SMTP_PORT,
  SMTP_USER: process.env.SMTP_USER,
  SMTP_PASS: process.env.SMTP_PASS,
  EMAIL_FROM: process.env.EMAIL_FROM,

  // Testing modes
  TEST_MODE: process.env.TEST_MODE,
  NEXT_PUBLIC_TEST_MODE: process.env.NEXT_PUBLIC_TEST_MODE,

  // Next.js specific environment variables
  NEXT_PUBLIC_APP_NAME: process.env.NEXT_PUBLIC_APP_NAME || packageJson.name,
  NEXT_PUBLIC_APP_VERSION: process.env.NEXT_PUBLIC_APP_VERSION || packageJson.version,
}

if (required.NODE_ENV === 'development') {
  // In development, we can use the default values for some environment variables
  required.DATABASE_URL = required.DATABASE_URL || 'file:./dev.db'
  optional.PORT = optional.PORT || '3000'
  required.GARAGE_AUTH_CLIENT_ID = required.GARAGE_AUTH_CLIENT_ID || optional.NEXT_PUBLIC_APP_NAME
  required.GARAGE_AUTH_CLIENT_SECRET = required.GARAGE_AUTH_CLIENT_SECRET || 'development_secret'
  required.NEXTAUTH_SECRET = 'development_secret'
} else if (required.NODE_ENV === 'test') {
  // In test mode, we can set the database URL to a test database
  required.DATABASE_URL = required.DATABASE_URL || 'file:./test.db'
  optional.SMTP_HOST = optional.SMTP_HOST || 'localhost'
  optional.SMTP_PORT = optional.SMTP_PORT || '1025'
  optional.SMTP_USER = optional.SMTP_USER || ''
  optional.SMTP_PASS = optional.SMTP_PASS || ''
  optional.EMAIL_FROM = optional.EMAIL_FROM || 'nextstrap@example.com'
  optional.TEST_MODE = optional.TEST_MODE || 'true'
}

for (const key in required) {
  if (!required[key as keyof typeof required]) {
    throw new Error(`Missing required environment variable: ${key}`)
  }
}
const validatedRequired = required as { [key in keyof typeof required]: string }

const config = {
  ...validatedRequired,
  ...optional,

  // Convert string values
  TEST_MODE: required.NODE_ENV === 'test' || optional.TEST_MODE === 'true',
  NEXT_PUBLIC_TEST_MODE: required.NODE_ENV === 'test' || optional.NEXT_PUBLIC_TEST_MODE === 'true',
  NEXT_PUBLIC_APP_NAME:
    optional.NEXT_PUBLIC_APP_NAME.charAt(0).toUpperCase() + optional.NEXT_PUBLIC_APP_NAME.slice(1),
  NEXT_PUBLIC_APP_VERSION: optional.NEXT_PUBLIC_APP_VERSION,
}

export default config

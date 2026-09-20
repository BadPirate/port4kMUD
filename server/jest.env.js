// Runs before the test framework and before any test file is loaded, so the
// config utility (which reads the environment once, at import time) sees these.
//
// Tests that touch the database get their own throwaway SQLite file rather
// than the real mud/lib/etc/portal.sqlite, and a fixed secret so encrypted
// MUD passwords round-trip within a run.
const os = require('os')
const path = require('path')

process.env.AUTH_DATABASE_PATH =
  process.env.AUTH_DATABASE_PATH || path.join(os.tmpdir(), `port4k-jest-${process.pid}.sqlite`)
process.env.BETTER_AUTH_SECRET =
  process.env.BETTER_AUTH_SECRET || 'jest-secret-not-for-production-0123456789abcdef'

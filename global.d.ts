/* eslint-disable no-var, vars-on-top, no-unused-vars, eol-last */
// Global type declarations
export {}

declare global {
  // In test mode, we capture sent emails here
  var TEST_EMAILS: Array<{ identifier: string; url: string }>
}
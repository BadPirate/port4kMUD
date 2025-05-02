// Next.js API route to inspect captured emails in test mode
import type { NextApiRequest, NextApiResponse } from 'next'
import config from '../../../src/utils/config'

const isTestMode = config.TEST_MODE

const handler = (req: NextApiRequest, res: NextApiResponse) => {
  if (!isTestMode) {
    // Not in test mode: no such endpoint
    res.status(404).json({ error: 'Not found' })
    return
  }
  // Retrieve stored emails
  const emails = global.TEST_EMAILS || []
  if (req.method === 'DELETE') {
    global.TEST_EMAILS = []
    res.status(204).end()
    return
  }
  res.status(200).json(emails)
}

export default handler

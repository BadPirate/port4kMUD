import type { NextApiRequest, NextApiResponse } from 'next'

/**
 * This is a placeholder API route that our frontend can ping
 * to ensure the socket server is initialized.
 */
export default function handler(req: NextApiRequest, res: NextApiResponse) {
  // Just respond with a success message
  res.status(200).json({ status: 'ok' })
}

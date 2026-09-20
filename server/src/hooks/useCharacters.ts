import { useCallback, useEffect, useState } from 'react'
import type { PortalAccount } from '../utils/portal-types'

interface UseCharacters {
  account: PortalAccount | null
  loading: boolean
  refresh: () => Promise<void>
  forget: (characterId: string) => Promise<void>
}

async function fetchAccount(): Promise<PortalAccount | null> {
  try {
    const response = await fetch('/api/portal/characters')
    return response.ok ? ((await response.json()) as PortalAccount) : null
  } catch (err) {
    console.error('Could not load characters:', err)
    return null
  }
}

/**
 * The signed-in account and its characters.
 *
 * Kept apart from the session itself, because the bridge changes this list as
 * a side effect of what happens at the MUD's prompts: the page refetches
 * whenever it hears that something did.
 *
 * What was fetched is stored with the account it was fetched for, so signing
 * out or signing in as somebody else can never show the previous account's
 * characters while the new list is on its way.
 */
export function useCharacters(userId: string | null): UseCharacters {
  const [fetched, setFetched] = useState<{ userId: string; account: PortalAccount } | null>(null)

  useEffect(() => {
    if (!userId) return undefined

    let cancelled = false
    void (async () => {
      const account = await fetchAccount()
      if (!cancelled && account) setFetched({ userId, account })
    })()

    return () => {
      cancelled = true
    }
  }, [userId])

  const refresh = useCallback(async () => {
    if (!userId) return
    const account = await fetchAccount()
    if (account) setFetched({ userId, account })
  }, [userId])

  const forget = useCallback(
    async (characterId: string) => {
      await fetch(`/api/portal/characters/${encodeURIComponent(characterId)}`, {
        method: 'DELETE',
      })
      await refresh()
    },
    [refresh],
  )

  const account = fetched && fetched.userId === userId ? fetched.account : null

  return { account, loading: Boolean(userId) && !account, refresh, forget }
}

import config from './config'
import type { PortalCharacter } from './portal-types'
import prisma from './prisma'
import { decryptSecret, encryptSecret } from './secret-box'

export type { PortalCharacter }

export interface SaveResult {
  character: PortalCharacter
  isNew: boolean
}

/** Raised when an account already holds MAX_CHARACTER_LIMIT characters. */
export class CharacterLimitReached extends Error {
  constructor(readonly limit: number) {
    super(`This account already has ${limit} characters.`)
    this.name = 'CharacterLimitReached'
  }
}

/**
 * MUD logins are case-insensitive: build_player_index lowercases names and
 * find_name compares with str_cmp, so "Bob" and "bob" are one character.
 */
function toNameKey(name: string): string {
  return name.trim().toLowerCase()
}

function toPortalCharacter(row: {
  id: string
  name: string
  lastLoginAt: Date | null
}): PortalCharacter {
  return {
    id: row.id,
    name: row.name,
    lastLoginAt: row.lastLoginAt ? row.lastLoginAt.toISOString() : null,
  }
}

export async function listCharacters(userId: string): Promise<PortalCharacter[]> {
  const rows = await prisma.mudCharacter.findMany({
    where: { userId },
    orderBy: [{ lastLoginAt: 'desc' }, { createdAt: 'asc' }],
  })
  return rows.map(toPortalCharacter)
}

export async function countCharacters(userId: string): Promise<number> {
  return prisma.mudCharacter.count({ where: { userId } })
}

/**
 * Remembers a character the player just logged in as, or refreshes the
 * password of one already remembered.
 *
 * A name the account already holds always takes the update path, so a player
 * at the limit can still log in as their existing characters and so a password
 * changed inside the game is picked up. Only a genuinely new name is refused.
 */
export async function saveCharacter(
  userId: string,
  name: string,
  password: string,
): Promise<SaveResult> {
  const nameKey = toNameKey(name)
  const secret = encryptSecret(password)

  return prisma.$transaction(async (tx) => {
    const existing = await tx.mudCharacter.findUnique({
      where: { userId_nameKey: { userId, nameKey } },
    })

    if (existing) {
      const updated = await tx.mudCharacter.update({
        where: { id: existing.id },
        data: { name, secret, lastLoginAt: new Date() },
      })
      return { character: toPortalCharacter(updated), isNew: false }
    }

    const total = await tx.mudCharacter.count({ where: { userId } })
    if (total >= config.MAX_CHARACTER_LIMIT) {
      throw new CharacterLimitReached(config.MAX_CHARACTER_LIMIT)
    }

    const created = await tx.mudCharacter.create({
      data: { userId, name, nameKey, secret, lastLoginAt: new Date() },
    })
    return { character: toPortalCharacter(created), isNew: true }
  })
}

/** Updates a stored password after the player changed it through the menu. */
export async function updateCharacterPassword(
  userId: string,
  name: string,
  password: string,
): Promise<PortalCharacter | null> {
  const nameKey = toNameKey(name)
  const existing = await prisma.mudCharacter.findUnique({
    where: { userId_nameKey: { userId, nameKey } },
  })
  if (!existing) return null

  const updated = await prisma.mudCharacter.update({
    where: { id: existing.id },
    data: { secret: encryptSecret(password) },
  })
  return toPortalCharacter(updated)
}

/**
 * The name and password to type at the MUD's prompts, or null if this account
 * does not own that character.
 */
export async function getCharacterCredentials(
  userId: string,
  characterId: string,
): Promise<{ id: string; name: string; password: string } | null> {
  const character = await prisma.mudCharacter.findFirst({ where: { id: characterId, userId } })
  if (!character) return null

  return {
    id: character.id,
    name: character.name,
    password: decryptSecret(character.secret),
  }
}

/** Forgets a character on the portal. Nothing in the MUD is touched. */
export async function removeCharacter(userId: string, characterId: string): Promise<boolean> {
  const { count } = await prisma.mudCharacter.deleteMany({ where: { id: characterId, userId } })
  return count > 0
}

/** Forgets a character by name, after the player deleted it inside the MUD. */
export async function removeCharacterByName(userId: string, name: string): Promise<boolean> {
  const { count } = await prisma.mudCharacter.deleteMany({
    where: { userId, nameKey: toNameKey(name) },
  })
  return count > 0
}

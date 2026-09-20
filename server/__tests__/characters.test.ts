/**
 * @jest-environment node
 */
import { readFileSync, readdirSync, rmSync, statSync } from 'fs'
import path from 'path'
import Database from 'better-sqlite3'
import config from '../src/utils/config'
import {
  CharacterLimitReached,
  getCharacterCredentials,
  listCharacters,
  removeCharacter,
  removeCharacterByName,
  saveCharacter,
  updateCharacterPassword,
} from '../src/utils/characters'
import prisma from '../src/utils/prisma'

const databasePath = config.AUTH_DATABASE_PATH as string

/** Applies the committed migrations, so the test runs the real schema. */
function migrate(): void {
  const migrations = path.join(__dirname, '..', 'prisma', 'migrations')
  const database = new Database(databasePath)
  try {
    for (const entry of readdirSync(migrations).sort()) {
      const sql = path.join(migrations, entry, 'migration.sql')
      // migrations/ also holds migration_lock.toml, which is not a migration.
      if (!statSync(path.join(migrations, entry)).isDirectory()) continue
      database.exec(readFileSync(sql, 'utf8'))
    }
  } finally {
    database.close()
  }
}

async function createUser(email: string): Promise<string> {
  const user = await prisma.user.create({
    data: { id: `user-${email}`, name: email, email, updatedAt: new Date() },
  })
  return user.id
}

describe('character store', () => {
  let userId: string

  beforeAll(async () => {
    for (const suffix of ['', '-journal', '-wal', '-shm']) {
      rmSync(`${databasePath}${suffix}`, { force: true })
    }
    migrate()
    userId = await createUser('player@example.com')
  })

  afterAll(async () => {
    await prisma.$disconnect()
    for (const suffix of ['', '-journal', '-wal', '-shm']) {
      rmSync(`${databasePath}${suffix}`, { force: true })
    }
  })

  afterEach(async () => {
    await prisma.mudCharacter.deleteMany({ where: { userId } })
  })

  it('remembers a character and gives its password back', async () => {
    const { character, isNew } = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')

    expect(isNew).toBe(true)
    expect(character.name).toBe('Zephyrtest')
    await expect(getCharacterCredentials(userId, character.id)).resolves.toMatchObject({
      name: 'Zephyrtest',
      password: 'sekrit1',
    })
  })

  it('stores the password encrypted rather than in the clear', async () => {
    const { character } = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')
    const row = await prisma.mudCharacter.findUniqueOrThrow({ where: { id: character.id } })

    expect(row.secret).not.toContain('sekrit1')
  })

  it('updates the password when the same character logs in again', async () => {
    const first = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')
    const second = await saveCharacter(userId, 'Zephyrtest', 'newpass9')

    expect(second.isNew).toBe(false)
    expect(second.character.id).toBe(first.character.id)
    await expect(getCharacterCredentials(userId, first.character.id)).resolves.toMatchObject({
      password: 'newpass9',
    })
  })

  it('treats names that differ only in case as one character', async () => {
    // MUD logins are case-insensitive, so "bob" and "Bob" are the same player.
    await saveCharacter(userId, 'Zephyrtest', 'sekrit1')
    const again = await saveCharacter(userId, 'ZEPHYRTEST', 'sekrit1')

    expect(again.isNew).toBe(false)
    await expect(listCharacters(userId)).resolves.toHaveLength(1)
  })

  it('refuses a new character once the limit is reached', async () => {
    for (let i = 0; i < config.MAX_CHARACTER_LIMIT; i += 1) {
      await saveCharacter(userId, `Character${'x'.repeat(i + 1)}`, 'sekrit1')
    }

    await expect(saveCharacter(userId, 'Onetoomany', 'sekrit1')).rejects.toBeInstanceOf(
      CharacterLimitReached,
    )
  })

  it('still accepts a character it already knows when at the limit', async () => {
    for (let i = 0; i < config.MAX_CHARACTER_LIMIT; i += 1) {
      await saveCharacter(userId, `Character${'x'.repeat(i + 1)}`, 'sekrit1')
    }

    const again = await saveCharacter(userId, 'Characterx', 'changed9')

    expect(again.isNew).toBe(false)
    await expect(getCharacterCredentials(userId, again.character.id)).resolves.toMatchObject({
      password: 'changed9',
    })
  })

  it('updates a password changed inside the game', async () => {
    const { character } = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')
    await updateCharacterPassword(userId, 'zephyrtest', 'changed9')

    await expect(getCharacterCredentials(userId, character.id)).resolves.toMatchObject({
      password: 'changed9',
    })
  })

  it('ignores a password change for a character it does not know', async () => {
    await expect(updateCharacterPassword(userId, 'Nobody', 'changed9')).resolves.toBeNull()
  })

  it('forgets a character', async () => {
    const { character } = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')

    await expect(removeCharacter(userId, character.id)).resolves.toBe(true)
    await expect(listCharacters(userId)).resolves.toHaveLength(0)
  })

  it('forgets a character by name after it was deleted in the MUD', async () => {
    await saveCharacter(userId, 'Zephyrtest', 'sekrit1')

    await expect(removeCharacterByName(userId, 'ZEPHYRTEST')).resolves.toBe(true)
    await expect(listCharacters(userId)).resolves.toHaveLength(0)
  })

  describe('with a second account', () => {
    let otherId: string

    beforeAll(async () => {
      otherId = await createUser('other@example.com')
    })

    afterEach(async () => {
      await prisma.mudCharacter.deleteMany({ where: { userId: otherId } })
    })

    it('keeps each account to its own characters', async () => {
      const mine = await saveCharacter(userId, 'Zephyrtest', 'sekrit1')
      await saveCharacter(otherId, 'Someoneelse', 'other1')

      await expect(listCharacters(userId)).resolves.toHaveLength(1)
      await expect(getCharacterCredentials(otherId, mine.character.id)).resolves.toBeNull()
      await expect(removeCharacter(otherId, mine.character.id)).resolves.toBe(false)
    })

    it('lets two accounts remember the same character name', async () => {
      await saveCharacter(userId, 'Zephyrtest', 'sekrit1')

      await expect(saveCharacter(otherId, 'Zephyrtest', 'sekrit1')).resolves.toMatchObject({
        isNew: true,
      })
    })
  })
})

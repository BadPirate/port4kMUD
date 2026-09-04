import { Client, GatewayIntentBits, TextChannel } from 'discord.js'
import discordConfig from './config'
import { startBridgeListener, BridgeEvent } from './bridge-listener'
import { MudRelayClient } from './mud-relay-client'

function formatEvent(event: BridgeEvent): string {
  switch (event.type) {
    case 'login':
      return `**${event.name}** has entered the game.`
    case 'levelup':
      return `**${event.name}** reached level ${event.level}!`
    case 'channel':
      return `**[${event.channelLabel}] ${event.name}:** ${event.message}`
    default:
      return ''
  }
}

export async function startDiscordBridge(): Promise<void> {
  if (!discordConfig.enabled || !discordConfig.botToken) return

  const client = new Client({
    intents: [
      GatewayIntentBits.Guilds,
      GatewayIntentBits.GuildMessages,
      GatewayIntentBits.MessageContent,
    ],
  })

  // discord.js's Client is an EventEmitter - an unhandled 'error' (or
  // 'shardError') event throws and takes down this whole process, which also
  // serves the game's web interface. A Discord-side problem (bad token,
  // disallowed intents, a gateway hiccup) must never be able to do that.
  client.on('error', (err) => console.error('Discord client error:', err))
  client.on('shardError', (err) => console.error('Discord shard error:', err))

  let relayClient: MudRelayClient | null = null
  if (discordConfig.botMudUsername && discordConfig.botMudPassword) {
    relayClient = new MudRelayClient(discordConfig.botMudUsername, discordConfig.botMudPassword)
    relayClient.on('ready', () => console.log('Discord relay: logged into MUD'))
    relayClient.on('disconnected', () =>
      console.log('Discord relay: MUD connection dropped, reconnecting'),
    )
    relayClient.start()
  } else {
    console.warn(
      'Discord bridge: DISCORD_BOT_MUD_USERNAME/DISCORD_BOT_MUD_PASSWORD not set - ' +
        'Discord-to-MUD relay disabled, MUD-to-Discord events will still be posted',
    )
  }

  const bridgeListener = startBridgeListener(discordConfig.bridgeSocketPath)

  client.once('ready', () => {
    console.log(`Discord bridge: logged in as ${client.user?.tag}`)
  })

  // Posting to Discord is pure REST, but reading from it needs a live gateway
  // session, so the outbound half keeps working perfectly while the inbound
  // half is dead. A rejected identify (usually the Message Content intent not
  // being enabled for the application) arrives after login() has resolved and
  // only reaches the error handlers above, so say so plainly instead.
  const readyCheck = setTimeout(() => {
    if (!client.isReady()) {
      console.error(
        'Discord bridge: no gateway session after 30s - Discord-to-MUD relay will ' +
          'not receive anything. Check that the Message Content Intent is enabled ' +
          "for this application in the Discord Developer Portal (posting to Discord doesn't need it).",
      )
    }
  }, 30000)
  readyCheck.unref()

  bridgeListener.on('event', async (event: BridgeEvent) => {
    if (!discordConfig.channelId) return
    try {
      const channel = await client.channels.fetch(discordConfig.channelId)
      if (channel instanceof TextChannel) {
        await channel.send(formatEvent(event))
      }
    } catch (err) {
      console.error('Discord bridge: failed to post event:', err)
    }
  })

  client.on('messageCreate', (message) => {
    if (message.author.id === client.user?.id) return // never relay our own posts
    if (message.channelId !== discordConfig.channelId) return
    if (message.author.bot) return
    const author = message.member?.displayName || message.author.username
    if (!relayClient) {
      console.warn(
        `Discord bridge: dropping message from ${author} - no MUD relay ` +
          '(DISCORD_BOT_MUD_USERNAME/DISCORD_BOT_MUD_PASSWORD are not set)',
      )
      return
    }
    relayClient.send(`${author}: ${message.content}`)
  })

  await client.login(discordConfig.botToken)
}

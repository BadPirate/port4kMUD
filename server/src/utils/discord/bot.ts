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
    relayClient?.send(`${author}: ${message.content}`)
  })

  await client.login(discordConfig.botToken)
}

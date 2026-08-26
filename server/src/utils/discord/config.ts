import config from '../config'

// Discord bridge configuration, derived from the centralized config utility.
// Everything here is optional - with no DISCORD_BOT_TOKEN set, `enabled` is
// false and the bridge never starts, so the game runs unchanged with no
// Discord config present.
const discordConfig = {
  botToken: config.DISCORD_BOT_TOKEN,
  channelId: config.DISCORD_CHANNEL_ID,
  bridgeSocketPath: config.DISCORD_BRIDGE_SOCKET_PATH,
  botMudUsername: config.DISCORD_BOT_MUD_USERNAME,
  botMudPassword: config.DISCORD_BOT_MUD_PASSWORD,
  get enabled(): boolean {
    return !!this.botToken
  },
}

export default discordConfig

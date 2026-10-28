/** Known distractions, blocked by every session. Users can add their own on top. */
export const PRESETS: { group: string; sites: string[] }[] = [
	{
		group: 'Social',
		sites: [
			'x.com',
			'twitter.com',
			'facebook.com',
			'instagram.com',
			'tiktok.com',
			'reddit.com',
			'snapchat.com',
			'threads.net',
			'bsky.app',
			'pinterest.com',
			'tumblr.com',
			'discord.com'
		]
	},
	{
		group: 'Video & streaming',
		sites: ['youtube.com', 'netflix.com', 'twitch.tv', 'kick.com', 'primevideo.com', 'disneyplus.com', 'hulu.com', 'max.com']
	},
	{
		group: 'News & forums',
		sites: ['news.ycombinator.com', 'cnn.com', 'foxnews.com', 'bbc.com', 'nytimes.com', 'buzzfeed.com', '9gag.com']
	},
	{
		group: 'Shopping',
		sites: ['amazon.com', 'ebay.com', 'aliexpress.com', 'temu.com', 'etsy.com', 'shein.com']
	},
	{
		group: 'Games',
		sites: ['chess.com', 'lichess.org', 'roblox.com', 'store.steampowered.com', 'poki.com']
	}
];

export const PRESET_SITES = PRESETS.flatMap((g) => g.sites);

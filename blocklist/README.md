# Mast blocklist

The curated list of distractions Mast ships with. It is one file, [`blocklist.json`](blocklist.json),
read by both the demo (`demo/src/lib/mast/blocklist.ts`) and the Chromium build.

```sh
node blocklist/check.mjs            # shape, duplicates, exceptions, and that every domain resolves
node blocklist/check.mjs --offline  # shape only
node blocklist/format.mjs           # canonical layout: one service per line
```

## Shape

`categories → services → domains`. People choose **services** ("YouTube"), not domains. A
service lists every domain that opens it.

```json
{ "name": "Amazon", "domains": ["amazon.com", "amazon.co.uk", "a.co"], "except": ["aws.amazon.com"] }
```

- **Domains match themselves and every subdomain.** `youtube.com` covers `m.youtube.com` and
  `music.youtube.com`, so subdomains of a listed domain are never listed separately. The checker
  rejects them.
- **`except` keeps work subdomains open** under a blocked domain, e.g. the AWS console under
  amazon.com.

## What goes in

1. **The sites people use.** That means the web app and its short links (`youtu.be`, `redd.it`,
   `fb.me`, `pin.it`), plus country domains for the big stores and networks.
2. **Not shared infrastructure.** CDNs, image hosts and APIs are left out (`ytimg.com`,
   `fbcdn.net`, `twimg.com`, `googlevideo.com`, `t.co`), because blocking them breaks unrelated
   sites.
3. **One home per domain.** A domain belongs to exactly one service. For example, TikTok is
   Social, not Video.
4. **Popular worldwide, not just in the US.** There are regional leaders for China, Japan, Korea,
   India, Latin America, Europe and Southeast Asia.
5. **Alive.** Every domain must resolve. The checker fails on dead ones, so the list gets pruned
   when services shut down.

## Known limits

- **Host-based only.** Services that live under a path of a bigger site can't be blocked on
  their own yet: BBC iPlayer (`bbc.co.uk/iplayer`), Facebook Marketplace, NYT Games, YouTube
  Shorts. Adding path rules is a later step.
- **Adult and gambling can't be complete by hand.** These lists cover the large, well-known
  sites. Full coverage needs a maintained feed; see "Large feeds" below.

## Counts (2026-11-03)

14 categories, 502 services, 641 domains:
- Social media
- Video
- TV & film streaming
- News
- Forums & communities
- Entertainment & memes
- Shopping
- Games
- Sports
- Gambling & betting
- Trading & crypto
- Dating
- Messaging & chat
- Adult

## Large feeds (Adult, Gambling)

Surveyed 2026-11-03 against the live repos.

| Feed | Licence | Size | Updated | Verdict |
|---|---|---|---|---|
| [HaGeZi NSFW](https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/nsfw-onlydomains.txt) | GPL-3.0 | 92k root domains | several times a day | **Adult: recommended** |
| [HaGeZi Gambling mini](https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/gambling.mini-onlydomains.txt) | GPL-3.0 | 113k (medium 181k, full 627k) | several times a day | **Gambling: recommended** |
| OISD nsfw / nsfw_small | GPL-3.0 | 486k / 18k | daily | Adult alternative, broader |
| StevenBlack porn-only / gambling-only | MIT + CC BY 4.0 sources | 77k / 6.7k | every few days | Permissive option; gambling too thin |
| Block List Project | Unlicense/MIT (inconsistent) | 953k / 279k | **archived** | Avoid: many open false positives |
| UT1 (Toulouse) | CC BY-SA 4.0 | adult 4.6M, gambling 48k, … | site down, mirror weekly | Cross-check only |
| NextDNS services | MIT | 42 service lists | stale | Cross-check our service domains |

Plan:
- **Ship each feed as its own data file.** Keep the feeds out of `blocklist.json`.
- **Bundle the licence and source URL with each feed** (GPL-3.0 for HaGeZi).
- **Keep the browser code separate from the feeds.** The browser code stays BSD; the feed files are aggregated data alongside it.
- **Refresh feeds through Mast's update channel.** No browser release needed.
- **Hand-curated entries win on conflicts.** Example: kalshi.com and polymarket.com sit in Gambling here and also appear in HaGeZi.

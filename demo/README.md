# Mast — browser demo

Interactive SvelteKit mock of **Mast**, a browser that blocks distractions for real. Pick
categories or sites, choose how long (an hour up to forever), and nothing inside the browser
can unblock them early.

```sh
bun install
bun run dev
```

Block something from the new tab page, or with **Block** in the toolbar. Open the **Demo**
drawer (bottom-right) to time-travel and try to escape.

Product rules: [docs/SPEC.md](docs/SPEC.md). Design language: [docs/DESIGN.md](docs/DESIGN.md).
How it maps onto a Chromium fork: [docs/CHROMIUM.md](docs/CHROMIUM.md).

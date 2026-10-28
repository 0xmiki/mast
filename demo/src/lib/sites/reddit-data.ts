/** Fake Reddit (reddit.com) content. Everything is local and deterministic.
 *  `name` on subs and `sub` on posts are stored without the "r/" prefix. */

export type Sub = { name: string; members: string; about: string };

export type RPost = {
	id: string;
	sub: string;
	author: string;
	title: string;
	body?: string;
	votes: string;
	comments: number;
	age: string;
	flair?: string;
	media?: boolean;
};

export type RComment = { author: string; text: string; votes: string; depth: 0 | 1 | 2 };

export const subs: Sub[] = [
	{
		name: 'AskReddit',
		members: '42.1M',
		about: 'Ask a question, get a hundred answers. Keep it open-ended and be kind.'
	},
	{
		name: 'programming',
		members: '6.2M',
		about: 'Discussion and news about programming. Be specific and show your work.'
	},
	{
		name: 'todayilearned',
		members: '32.8M',
		about: 'Share something you just learned. Include a source.'
	},
	{
		name: 'productivity',
		members: '1.7M',
		about: 'Systems, habits, and the occasional app that survives longer than a week.'
	},
	{
		name: 'nosleep',
		members: '17.4M',
		about: 'Original first-person stories of the strange and the frightening.'
	},
	{
		name: 'Damnthatsinteresting',
		members: '28.9M',
		about: 'Things that are really, really interesting. Sources are required.'
	},
	{
		name: 'rust',
		members: '310K',
		about: 'A place to talk about the Rust programming language, from first build to production.'
	},
	{
		name: 'cooking',
		members: '5.4M',
		about: 'Recipes, techniques, and kitchen disasters are all welcome here.'
	}
];

export const posts: RPost[] = [
	{
		id: 'p01',
		sub: 'AskReddit',
		author: 'mossgrid',
		title: 'What is a small habit that quietly changed your life?',
		body: "Not a big reset. Something you do in under a minute. Mine is writing tomorrow's first task before I close the laptop.",
		votes: '48.2k',
		comments: 1204,
		age: '3h',
		flair: 'Discussion'
	},
	{
		id: 'p02',
		sub: 'programming',
		author: 'ferncloud',
		title: 'Why does every "simple" config file eventually become a programming language?',
		body: 'YAML starts out declarative, then someone adds templating, then conditionals, then a loop. At what point do we admit it is code and test it like code?',
		votes: '9.1k',
		comments: 612,
		age: '5h',
		flair: 'Discussion'
	},
	{
		id: 'p03',
		sub: 'programming',
		author: 'brickfield',
		title: 'Our test suite went from 40 minutes to 4 after we deleted the tests nobody understood',
		votes: '22.6k',
		comments: 1180,
		age: '8h',
		flair: 'Experience',
		media: true
	},
	{
		id: 'p04',
		sub: 'todayilearned',
		author: 'quietmath',
		title: 'TIL the Sahara was a green savanna with lakes and hippos within the last 10,000 years.',
		body: 'Lake beds, rock art, and fossil hippo remains all point to a wetter Sahara during the African Humid Period. The desert spread as the monsoon shifted south.',
		votes: '64.8k',
		comments: 1870,
		age: '1h',
		flair: 'Science'
	},
	{
		id: 'p05',
		sub: 'productivity',
		author: 'latebloomer',
		title: 'The only task manager I have stuck with is a paper index card. Did anyone else give up on apps?',
		body: 'Three apps, two years, zero completed reviews. A card with five lines and a rubber band works better. Curious what others use when the system is the problem.',
		votes: '3.4k',
		comments: 288,
		age: '6h',
		flair: 'Question'
	},
	{
		id: 'p06',
		sub: 'nosleep',
		author: 'nightbaker',
		title: 'The elevator in our building has a floor that is not on the panel. I pressed it once.',
		body: 'It was labelled with a single dash. The doors opened onto a hallway lit the same yellow as ours, except the carpet was wet, and somewhere a sound like someone brushing their teeth in the next room.',
		votes: '18.9k',
		comments: 934,
		age: '10h',
		flair: 'Part 1'
	},
	{
		id: 'p07',
		sub: 'Damnthatsinteresting',
		author: 'ohmyscone',
		title: 'A single tardigrade survived 30 days in vacuum on the outside of a satellite and was revived with water.',
		votes: '51.0k',
		comments: 1430,
		age: '2h',
		media: true
	},
	{
		id: 'p08',
		sub: 'rust',
		author: 'pfeiffer_rs',
		title: 'Stop using unwrap() in library code. A short guide to error types that do not make your users cry.',
		body: 'thiserror for libraries, anyhow for binaries, and one rule: no panics on user input. Examples in the thread.',
		votes: '2.7k',
		comments: 134,
		age: '4h',
		flair: 'Tutorial'
	},
	{
		id: 'p09',
		sub: 'rust',
		author: 'amberlinks',
		title: 'Refactoring to async and getting lost in lifetimes. Looking for war stories.',
		body: 'Context: a tokio service, a shared cache, and a function that has three lifetime annotations too many.',
		votes: '1.4k',
		comments: 96,
		age: '12h',
		flair: 'Help'
	},
	{
		id: 'p10',
		sub: 'cooking',
		author: 'sourdough_sam',
		title: 'Crumb shot at 78 percent hydration. First real success after a year of dense bricks.',
		votes: '8.6k',
		comments: 402,
		age: '7h',
		flair: 'Photo',
		media: true
	},
	{
		id: 'p11',
		sub: 'cooking',
		author: 'ohmyscone',
		title: 'Why does my pan sauce always split? Looking for the one technique that finally fixed it.',
		body: 'Usually I pull the pan off the heat too early and add cold butter all at once. Tips welcome.',
		votes: '1.9k',
		comments: 221,
		age: '20h',
		flair: 'Question'
	},
	{
		id: 'p12',
		sub: 'AskReddit',
		author: 'ferncloud',
		title: 'What is something people still get wrong about moving to a new city?',
		body: 'Everyone plans the apartment and forgets the boring errands. Which pharmacy is open late? Which doctor takes new patients? Those take a year to learn.',
		votes: '12.3k',
		comments: 3401,
		age: '9h',
		flair: 'Discussion'
	},
	{
		id: 'p13',
		sub: 'AskReddit',
		author: 'tidepool_89',
		title: 'Teachers of Reddit, what is the most honest thing a student has ever written on a test?',
		votes: '29.7k',
		comments: 2580,
		age: '15h'
	},
	{
		id: 'p14',
		sub: 'todayilearned',
		author: 'brickfield',
		title: 'TIL octopuses have three hearts, and two of them stop beating while they swim.',
		body: 'The two branchial hearts pause while the systemic heart keeps going, which is why they tire quickly when swimming and tend to crawl instead.',
		votes: '33.5k',
		comments: 1011,
		age: '1d',
		flair: 'Biology'
	},
	{
		id: 'p15',
		sub: 'productivity',
		author: 'mossgrid',
		title: 'Time blocking, but for the boring stuff: laundry, dishes, email. Worked better than I expected.',
		votes: '4.9k',
		comments: 177,
		age: '1d',
		flair: 'Tips',
		media: true
	},
	{
		id: 'p16',
		sub: 'nosleep',
		author: 'latebloomer',
		title: 'My grandmother kept a guest room made up for someone who never arrived.',
		body: 'Every morning the sheets were turned back neatly. She said it was for the one who was coming to fix the radio. We never had a radio that worked.',
		votes: '7.7k',
		comments: 402,
		age: '2d'
	},
	{
		id: 'p17',
		sub: 'Damnthatsinteresting',
		author: 'quietmath',
		title: 'The mantis shrimp can punch so fast the water around the strike boils, leaving a tiny shock wave behind.',
		votes: '20.4k',
		comments: 512,
		age: '2d',
		media: true
	},
	{
		id: 'p18',
		sub: 'cooking',
		author: 'nightbaker',
		title: 'Cast iron seasoning myth-busting: flaxseed oil is not the only way, and your pan is probably fine.',
		body: 'Any high smoke point oil, heated repeatedly, polymerizes. The rest is patience and a lot of hot wiping.',
		votes: '3.1k',
		comments: 244,
		age: '3d',
		flair: 'Tips'
	},
	{
		id: 'p19',
		sub: 'rust',
		author: 'mossgrid',
		title: 'Just shipped a CLI in Rust that starts in 4 ms. The secret was not the language. It was lazy config loading.',
		body: 'Parse only what the subcommand needs, and keep a cached index. Happy to share the flamegraph.',
		votes: '5.5k',
		comments: 189,
		age: '3d',
		flair: 'Show'
	},
	{
		id: 'p20',
		sub: 'productivity',
		author: 'yukit',
		title: 'My weekly review takes 15 minutes. These are the three questions I ask every Sunday.',
		body: 'What moved? What stalled? What am I ignoring on purpose this week? That is the whole template.',
		votes: '2.2k',
		comments: 98,
		age: '4d',
		flair: 'Routine'
	}
];

/** Shared comment thread shown under every post. Depth drives indentation. */
export const comments: RComment[] = [
	{
		author: 'quietmath',
		text: 'Honest answer: writing a two-line list before opening the laptop changed my year. Boring, but it worked for about six months.',
		votes: '4.2k',
		depth: 0
	},
	{
		author: 'brickfield',
		text: 'Six months is a long run for anything. What broke the streak?',
		votes: '312',
		depth: 1
	},
	{
		author: 'quietmath',
		text: 'A new job with a 9am call every day. The list survived, but I stopped reading it.',
		votes: '198',
		depth: 2
	},
	{
		author: 'nightbaker',
		text: 'Same here, except mine is a sticky note. It has survived three apartments.',
		votes: '455',
		depth: 1
	},
	{
		author: 'ferncloud',
		text: 'Counterpoint: the tool matters less than the first ten minutes of the day. Protect those and everything else follows.',
		votes: '1.1k',
		depth: 0
	},
	{
		author: 'ohmyscone',
		text: 'This is the same argument people have about standups. The ritual is the point.',
		votes: '87',
		depth: 1
	},
	{
		author: 'mossgrid',
		text: 'Agreed, but a ritual nobody believes in turns into theatre quickly.',
		votes: '61',
		depth: 2
	},
	{
		author: 'latebloomer',
		text: 'Tried every app. Ended up with paper. The only thing I would add is a timer, so the list does not run forever.',
		votes: '940',
		depth: 0
	}
];

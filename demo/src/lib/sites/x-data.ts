/** Fake X (x.com) content. Everything is local and deterministic. */

export type Post = {
	id: string;
	name: string;
	handle: string;
	text: string;
	age: string;
	replies: number;
	reposts: number;
	likes: number;
	views: string;
	media?: boolean;
};

export type Trend = { category: string; topic: string; posts: string };

export const posts: Post[] = [
	{
		id: '1001',
		name: 'Dana Okafor',
		handle: 'danaokafor',
		text: 'Hot take: the "10x engineers" I have worked with were mostly the ones who wrote things down. Nobody puts "wrote the runbook" on a performance review.',
		age: '12m',
		replies: 84,
		reposts: 212,
		likes: 1900,
		views: '96K'
	},
	{
		id: '1002',
		name: 'Marcus Lindqvist',
		handle: 'mlindqvist',
		text: 'Moved our build from Node scripts to a Rust binary. Cold builds went from 14 minutes to 3. Team morale went from fine to "who wants to be the one who touches the config."\n\nTradeoffs are real. So is the speed.',
		age: '38m',
		replies: 61,
		reposts: 140,
		likes: 1100,
		views: '58.2K'
	},
	{
		id: '1003',
		name: 'Priya Raman',
		handle: 'priyacodes',
		text: 'me explaining to the intern that a merge conflict is "just a conflict, not a moral failing" [image: a golden retriever in a hard hat staring at a diff with the face of a man who has seen things]',
		age: '1h',
		replies: 33,
		reposts: 520,
		likes: 4800,
		views: '310K'
	},
	{
		id: '1004',
		name: 'Harbor Transit Desk',
		handle: 'harbortransit',
		text: 'BREAKING: Line 4 is suspended between Pier 9 and Eastgate after a signal fault near the tunnel. Crews expect repairs by 6pm. Shuttle buses are running at every stop. Updates to follow.',
		age: '3m',
		replies: 412,
		reposts: 2300,
		likes: 5400,
		views: '1.8M',
		media: true
	},
	{
		id: '1005',
		name: 'Jonah Feld',
		handle: 'jonahbuilds',
		text: 'Unpopular: TypeScript is the best thing that happened to frontend, and also the reason a coworker is currently writing a 40-line generic to center a div.',
		age: '1h',
		replies: 58,
		reposts: 97,
		likes: 880,
		views: '41K'
	},
	{
		id: '1006',
		name: 'Ren Adeyemi',
		handle: 'renwrites',
		text: '"It works on my machine" is a complete sentence and also a cry for help.',
		age: '2h',
		replies: 22,
		reposts: 188,
		likes: 1400,
		views: '73K'
	},
	{
		id: '1007',
		name: 'Tomás Ibarra',
		handle: 'tomasi',
		text: 'Finished the first proper render of the new lighting pass. Left is before, right is after. I still do not feel allowed to be this happy about one shadow.',
		age: '4h',
		replies: 19,
		reposts: 64,
		likes: 702,
		views: '22.9K',
		media: true
	},
	{
		id: '1008',
		name: 'Hannah Brooks',
		handle: 'hbrooks',
		text: 'Saying "I don\'t know, let me check and get back to you" in a meeting is a skill. Scarce. Protect it.',
		age: '5h',
		replies: 17,
		reposts: 126,
		likes: 960,
		views: '35K'
	},
	{
		id: '1009',
		name: 'Kofi Mensah',
		handle: 'kofimakes',
		text: 'Argued for two hours that tabs are obviously superior. Lost the argument to a spreadsheet of 900 PRs. The spreadsheet had no opinion, which is the scary part.',
		age: '6h',
		replies: 44,
		reposts: 71,
		likes: 655,
		views: '29K'
	},
	{
		id: '1010',
		name: 'Lena Vogt',
		handle: 'lenavogt',
		text: 'Got ratio\'d by a guy whose entire bio is "opinions are my own, PRs are not." I have never felt more seen.',
		age: '7h',
		replies: 2300,
		reposts: 88,
		likes: 410,
		views: '180K'
	},
	{
		id: '1011',
		name: 'Grace Oduya',
		handle: 'graceo',
		text: 'Sunday pottery. The bowl is lopsided. The bowl is mine. Nobody tell my cat.',
		age: '8h',
		replies: 27,
		reposts: 45,
		likes: 1200,
		views: '51K',
		media: true
	},
	{
		id: '1012',
		name: 'Sam Achterberg',
		handle: 'samachter',
		text: 'Reinstalled my OS for the ninth time this year. Not because it was broken. I wanted to finally get the dotfiles right. I have not.',
		age: '1d',
		replies: 12,
		reposts: 33,
		likes: 349,
		views: '14.6K'
	},
	{
		id: '1013',
		name: 'Naveen Rao',
		handle: 'naveenr',
		text: 'Not even a hot take: write the boring migration first, then the feature. Every single time.',
		age: '1d',
		replies: 31,
		reposts: 102,
		likes: 777,
		views: '39.9K'
	},
	{
		id: '1014',
		name: 'Inês Duarte',
		handle: 'inesd',
		text: 'The move fast and break things era gave us a lot. Mostly it gave us the hold music on customer support lines, which now plays a recording of a chatbot apologizing.',
		age: '1d',
		replies: 68,
		reposts: 260,
		likes: 2100,
		views: '120K'
	},
	{
		id: '1015',
		name: 'Owen Park',
		handle: 'owenpark',
		text: 'Ran a 5k in the rain. My watch thinks I climbed a small mountain. Not mad. Just impressed by the GPS.',
		age: '1d',
		replies: 9,
		reposts: 14,
		likes: 203,
		views: '8.1K',
		media: true
	},
	{
		id: '1016',
		name: 'Yuki Tanaka',
		handle: 'yukit',
		text: 'Standups are 15 minutes that could have been an email. The standup after the standup is 45 minutes that could have been a sentence.',
		age: '2d',
		replies: 40,
		reposts: 85,
		likes: 690,
		views: '27K'
	},
	{
		id: '1017',
		name: 'Ada Lindgren',
		handle: 'adalindgren',
		text: 'Found a comment in our codebase: "// TODO: remove before launch". The launch was in 2021. The TODO is now load-bearing.',
		age: '2d',
		replies: 55,
		reposts: 310,
		likes: 2600,
		views: '144K'
	},
	{
		id: '1018',
		name: 'Marco Bellini',
		handle: 'marcobuilds',
		text: 'Built a tiny weather station out of a Raspberry Pi and an old bread box. It reports kitchen humidity to 0.1 percent. Nobody asked. Everyone is tolerating it.',
		age: '2d',
		replies: 24,
		reposts: 58,
		likes: 830,
		views: '36K',
		media: true
	},
	{
		id: '1019',
		name: 'Ravi Kapoor',
		handle: 'ravik',
		text: 'Hot take part 2: the best feature in any app is undo, and almost nobody ships it well.',
		age: '3d',
		replies: 19,
		reposts: 47,
		likes: 412,
		views: '20.3K'
	},
	{
		id: '1020',
		name: 'Rosa Mendes',
		handle: 'rosamendes',
		text: 'Stop writing "just" in documentation. "Just run the migration" is how I spent a Saturday.',
		age: '3d',
		replies: 14,
		reposts: 38,
		likes: 501,
		views: '26.7K'
	}
];

export const trends: Trend[] = [
	{ category: 'Technology · Trending', topic: 'Rust', posts: '24.1K posts' },
	{ category: 'Trending in Transit', topic: 'Line 4', posts: '9.8K posts' },
	{ category: 'Engineering · Trending', topic: 'Standups', posts: '6,120 posts' },
	{ category: 'Trending', topic: 'Pottery', posts: '3,407 posts' },
	{ category: 'Cooking · Trending', topic: 'Sourdough', posts: '18.6K posts' },
	{ category: 'Trending', topic: 'Migration', posts: '4,955 posts' }
];

export const suggested: { name: string; handle: string }[] = [
	{ name: 'Build Notes', handle: 'buildnotes' },
	{ name: 'Ferris Weekly', handle: 'ferrisweekly' },
	{ name: 'Dev Diary', handle: 'devdiary' }
];

/** Short bios picked deterministically for profile pages. */
export const bios: string[] = [
	'Engineer. Writing things down so future me can read them.',
	'Building small tools and writing about them. Opinions are my own.',
	'Makes bread, breaks bread, occasionally ships code.',
	'Designer who reads changelogs for fun.',
	'Here for the hot takes and the good PR descriptions.',
	'Coffee first, then commits.'
];

/** Replies shown under a status page, rotated per post. */
export const replyPool: Post[] = [
	{
		id: 'r1',
		name: 'Theo Marsh',
		handle: 'theomarsh',
		text: 'Strong agree. The boring part is the part that keeps working.',
		age: '1h',
		replies: 2,
		reposts: 4,
		likes: 31,
		views: '1.2K'
	},
	{
		id: 'r2',
		name: 'Nadia Haddad',
		handle: 'nadiah',
		text: 'Can confirm. Our runbook has a section titled "Do not touch the cron" and it is the most-read page in the wiki.',
		age: '52m',
		replies: 6,
		reposts: 19,
		likes: 214,
		views: '9.4K'
	},
	{
		id: 'r3',
		name: 'Ben Strauss',
		handle: 'bstrauss',
		text: 'Counterpoint: sometimes the boring version is the wrong version and you only find out late. Still better to find out.',
		age: '44m',
		replies: 11,
		reposts: 3,
		likes: 57,
		views: '3.3K'
	},
	{
		id: 'r4',
		name: 'Elif Yildiz',
		handle: 'elifyil',
		text: 'This is why I keep a scratch file open all day. Future me is always the audience.',
		age: '38m',
		replies: 3,
		reposts: 8,
		likes: 96,
		views: '4.0K'
	},
	{
		id: 'r5',
		name: 'Carlos Ortega',
		handle: 'carlosdev',
		text: 'Bookmarking this for the next time someone asks why the standup is still 30 minutes.',
		age: '25m',
		replies: 1,
		reposts: 5,
		likes: 48,
		views: '2.1K'
	},
	{
		id: 'r6',
		name: 'Mina Sato',
		handle: 'minasato',
		text: 'Honest question: how do you handle the people who will never read the docs? Asking for a friend who is also me.',
		age: '19m',
		replies: 14,
		reposts: 2,
		likes: 72,
		views: '5.6K'
	},
	{
		id: 'r7',
		name: 'Joel Brandt',
		handle: 'joelb',
		text: 'Ratio incoming, but I agree with the first half.',
		age: '12m',
		replies: 9,
		reposts: 1,
		likes: 17,
		views: '890'
	},
	{
		id: 'r8',
		name: 'Freya Lund',
		handle: 'freyalund',
		text: 'Saved. Sending this to my team, which is where good advice goes to be ignored.',
		age: '7m',
		replies: 0,
		reposts: 3,
		likes: 26,
		views: '1.4K'
	}
];

/** The signed-in account shown in composers and the Profile link. */
export const me = { name: 'Mast Reader', handle: 'mastreader' };

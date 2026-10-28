import { hash } from './types';

export interface Video {
	id: string;
	title: string;
	channel: string;
	views: string;
	age: string;
	duration: string;
}

export interface Short {
	id: string;
	title: string;
	views: string;
}

export interface FakeComment {
	handle: string;
	age: string;
	text: string;
	likes: string;
}

export const videos: Video[] = [
	{ id: 'aB3kR8nQ1zY', title: 'I tried living on $1 for 30 days', channel: 'Budget Bianca', views: '2.1M views', age: '3 weeks ago', duration: '18:42' },
	{ id: 'c9TqW2mLx0E', title: 'The most satisfying factory machines on Earth', channel: 'Process Lab', views: '48M views', age: '1 year ago', duration: '12:07' },
	{ id: 'dV5pJ1sHk8R', title: 'Learn Rust in 20 minutes', channel: 'Byte Sized Code', views: '880K views', age: '5 months ago', duration: '20:13' },
	{ id: 'eF7zN3cBw6Q', title: 'Lofi beats to study to (no ads, 3 hours)', channel: 'Night Desk Radio', views: '9.4M views', age: '2 years ago', duration: '3:02:55' },
	{ id: 'gH2yM9kLp4T', title: 'Why everyone is wrong about coffee', channel: 'Hot Take Lab', views: '1.6M views', age: '2 months ago', duration: '11:36' },
	{ id: 'hJ8rT4vXn2P', title: 'Building a treehouse with zero power tools', channel: 'Woodgrain Co.', views: '3.3M views', age: '9 months ago', duration: '24:18' },
	{ id: 'jK1wQ6sZm9B', title: 'The 10 smartest moves in chess history', channel: 'Gambit Gallery', views: '5.7M views', age: '7 months ago', duration: '16:50' },
	{ id: 'kL4eP0tYc7D', title: 'Cooking the perfect ramen from scratch', channel: 'Noodle Nerd', views: '2.8M views', age: '4 months ago', duration: '14:21' },
	{ id: 'mN6uR2hGa5F', title: 'Dogs meet their newborn sibling for the first time', channel: 'Happy Paws Diary', views: '12M views', age: '1 week ago', duration: '4:09' },
	{ id: 'nP9bV7fWq3H', title: 'Speedrunners break a 1990s game in 4 minutes', channel: 'Glitch Hunters', views: '4.2M views', age: '6 months ago', duration: '9:44' },
	{ id: 'pQ3gX5dRt8J', title: 'We rebuilt a dead city in Minecraft', channel: 'Blocky Builders', views: '7.8M views', age: '11 months ago', duration: '31:05' },
	{ id: 'qR8hY1eSu4K', title: 'Is your phone lying to you about battery life?', channel: 'Tech Tangent', views: '1.1M views', age: '2 weeks ago', duration: '10:27' },
	{ id: 'rS5jZ0fTv9L', title: 'Morning routine of a night-shift nurse', channel: 'Day In The Life', views: '6.5M views', age: '8 months ago', duration: '8:58' },
	{ id: 'tT2kA7gUw1M', title: 'Tokyo at 4am: the city that never sleeps', channel: 'Wanderwise', views: '2.9M views', age: '1 year ago', duration: '22:11' },
	{ id: 'uU7lB4hVx6N', title: 'Solving a 1,000-piece puzzle in one sitting', channel: 'Puzzle Patience', views: '540K views', age: '3 weeks ago', duration: '1:14:02' },
	{ id: 'vW9mC8iWy2P', title: "Why bridges don't collapse (usually)", channel: 'Engineering Explained', views: '4.9M views', age: '5 months ago', duration: '13:40' },
	{ id: 'wX1nD3jXz5Q', title: 'Podcast: the day my startup almost died', channel: 'Founders Unfiltered', views: '310K views', age: '4 days ago', duration: '58:19' },
	{ id: 'xY4oE6kYa0R', title: 'Ranking every Studio Ghibli opening', channel: 'Film Fridge', views: '1.9M views', age: '1 month ago', duration: '15:33' },
	{ id: 'yZ6pF9lZb3S', title: 'Live: rainy day sketching with me', channel: 'Ink and Ash', views: '1.2K watching', age: 'Streaming now', duration: 'LIVE' },
	{ id: 'zA8qG2mAc7T', title: 'Sourdough for beginners, no mistakes', channel: 'Crumb and Co.', views: '1.4M views', age: '2 years ago', duration: '17:02' },
	{ id: 'aAb0rH4nBd8', title: 'Pro gamer reacts to the worst ranked match', channel: 'Frame Perfect', views: '3.6M views', age: '10 days ago', duration: '11:19' },
	{ id: 'bBc2sJ7oCe1', title: 'Mechanical keyboards: is the hype real?', channel: 'Keys and Sounds', views: '760K views', age: '6 weeks ago', duration: '9:52' },
	{ id: 'cCd5tK9pDf3', title: 'Cooking with 3 ingredients from a gas station', channel: 'Roadside Kitchen', views: '5.2M views', age: '7 months ago', duration: '12:48' },
	{ id: 'dDe7uL1qEg6', title: 'The science of why songs get stuck in your head', channel: 'Mind Notes', views: '2.2M views', age: '1 year ago', duration: '14:05' }
];

export const shorts: Short[] = [
	{ id: 's0Lp3Qm8Rz1', title: 'Ice cube vs. a 500 degree pan', views: '4.1M' },
	{ id: 's1Ab7Cd2Ef9', title: 'This tiny trick fixes every tangled cable', views: '1.8M' },
	{ id: 's2Gh4Jk6Lm0', title: 'POV: you finally beat the boss', views: '920K' },
	{ id: 's3Nq8Rs1Tu5', title: 'Guess the country from the road', views: '2.5M' },
	{ id: 's4Vw2Xy7Za3', title: 'Watch the bread rise in 10 seconds', views: '3.3M' },
	{ id: 's5Bc9De4Fg8', title: 'My cat learned to open the fridge', views: '14M' },
	{ id: 's6Hi1Jk5Lm2', title: 'Three-ingredient fudge that actually sets', views: '640K' },
	{ id: 's7No6Pq0Rs4', title: 'The hallway illusion that broke my brain', views: '7.2M' }
];

export const comments: FakeComment[] = [
	{ handle: '@lunarmoth', age: '2 days ago', text: 'Okay the part near the end had me screaming. Subscribed.', likes: '1.2K' },
	{ handle: '@ceramicsbyjo', age: '5 days ago', text: 'Watched this three times and still cannot work out how they did the transition.', likes: '388' },
	{ handle: '@quietdesk', age: '1 week ago', text: 'Nobody talks about how long this actually takes. Respect.', likes: '712' },
	{ handle: '@pixelpants', age: '3 weeks ago', text: 'First time commenting. The pacing is perfect, not a single minute drags.', likes: '95' },
	{ handle: '@nightshift.nora', age: '1 month ago', text: 'This is the video I did not know I needed on a Tuesday night.', likes: '241' },
	{ handle: '@tinkerpath', age: '2 months ago', text: 'Part 2 when??', likes: '33' }
];

const descriptions = [
	'New video every week. Timestamps are in the comments. Thanks for watching, and see you in the next one.',
	'Everything in this video was filmed in one take unless noted. Gear list and sources are below.',
	'Join the newsletter for behind-the-scenes photos and early access to the next project.',
	'Chapters: 0:00 Intro, 1:12 The setup, 4:40 The part that went wrong, 9:03 Final result.'
];

/** Deterministic description for a video. */
export function descriptionFor(video: Video): string {
	return descriptions[hash(video.id) % descriptions.length];
}

/** Look up a video by id, falling back to a deterministic pick so every watch URL renders. */
export function getVideo(id: string): Video {
	return videos.find((v) => v.id === id) ?? videos[hash(id) % videos.length];
}

/** Look up a short by id, falling back to a deterministic pick. */
export function getShort(id: string): Short {
	return shorts.find((s) => s.id === id) ?? shorts[hash(id) % shorts.length];
}

/** Videos whose title contains any word of the query, padded with the rest to `count`. */
export function searchVideos(query: string, count = 8): Video[] {
	const words = query.toLowerCase().split(/\s+/).filter(Boolean);
	const matches = videos.filter((v) => {
		const title = v.title.toLowerCase();
		return words.some((w) => title.includes(w));
	});
	const rest = videos.filter((v) => !matches.includes(v));
	return [...matches, ...rest].slice(0, count);
}

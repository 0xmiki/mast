/**
 * Browser clock. In the real product this is monotonic + network time so changing the
 * system clock does nothing. In the demo, the time machine adds an offset.
 */
const OFFSET_KEY = 'mast.clockOffset';

class Clock {
	offset = $state(0);
	#real = $state(Date.now());

	constructor() {
		if (typeof localStorage !== 'undefined') {
			this.offset = Number(localStorage.getItem(OFFSET_KEY) ?? 0) || 0;
		}
		if (typeof window !== 'undefined') {
			setInterval(() => (this.#real = Date.now()), 250);
		}
	}

	get now() {
		return this.#real + this.offset;
	}

	get travelled() {
		return this.offset !== 0;
	}

	advance(ms: number) {
		this.#set(this.offset + ms);
	}

	/** Jump to the next occurrence of hh:mm. */
	jumpTo(hours: number, minutes = 0) {
		const d = new Date(this.now);
		d.setHours(hours, minutes, 0, 0);
		if (d.getTime() <= this.now) d.setDate(d.getDate() + 1);
		this.advance(d.getTime() - this.now);
	}

	reset() {
		this.#set(0);
	}

	#set(offset: number) {
		this.#real = Date.now();
		this.offset = offset;
		localStorage.setItem(OFFSET_KEY, String(offset));
	}
}

export const clock = new Clock();

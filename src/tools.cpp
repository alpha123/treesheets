#include "tools.h"

void MTRnd::SeedMT(uint seed) {
	uint x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
	int j;
	for (left = 0, *s++ = x, j = N; --j; *s++ = (x *= 69069U) & 0xFFFFFFFFU)
		;
}

uint MTRnd::ReloadMT() {
	uint *p0 = state, *p2 = state + 2, *pM = state + M, s0, s1;
	int j;
	if (left < -1) SeedMT(4357U);
	left = N - 1, next = state + 1;
	for (s0 = state[0], s1 = state[1], j = N - M + 1; --j; s0 = s1, s1 = *p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	for (pM = state, j = M; --j; s0 = s1, s1 = *p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	s1 = state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	s1 ^= (s1 >> 11);
	s1 ^= (s1 << 7) & 0x9D2C5680U;
	s1 ^= (s1 << 15) & 0xEFC60000U;
	return (s1 ^ (s1 >> 18));
}

uint MTRnd::RandomMT() {
	uint y;
	if (--left < 0) return (ReloadMT());
	y = *next++;
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9D2C5680U;
	y ^= (y << 15) & 0xEFC60000U;
	return (y ^ (y >> 18));
}

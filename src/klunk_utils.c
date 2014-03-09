#include "klunk_utils.h"
#include "errorcodes.h"

/* Calculate the size aligned to a 8-byte border */
uint16_t klunk_size8b(const uint16_t size)
{
	uint16_t s;
	if ((size % 8) == 0) {
		s = size;
	}
	else {
		s = ((size / 8) + 1) * 8;
	}
	return s;
}

uint32_t klunk_murmur3_32(const void *data, const size_t len)
{
    if (data == 0 || len == 0) {
    	return 0;
    }

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *)(data);
    const uint8_t *tail = (const uint8_t *)(data) + (nblocks * 4);

    uint32_t h = 0;

    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++) {
        k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = (h * 5) + 0xe6546b64;
    }

    k = 0;
    switch (len & 3) {
        case 3:
            k ^= tail[2] << 16;
        case 2:
            k ^= tail[1] << 8;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 13) | (k >> (32 - 15));
            k *= c2;
            h ^= k;
    };

    h ^= len;

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

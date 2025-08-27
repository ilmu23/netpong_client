// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<utils.c>>

#include <math.h>
#include <stdlib.h>

#include "utils.h"

#define _UTF8_BYTE_START	0x80U
#define _UTF8_LENGTH_2BS	(_UTF8_BYTE_START | 0x40U)
#define _UTF8_LENGTH_3BS	(_UTF8_BYTE_START | 0x60U)
#define _UTF8_LENGTH_4BS	(_UTF8_BYTE_START | 0x70U)

static inline size_t	_uintlen(u64 n);

char	*utoa16(u16 n, char *buf) {
	size_t	i;

	if (!buf)
		buf = malloc(6 * sizeof(*buf));
	if (buf) {
		for (i = _uintlen(n), buf[i] = '\0'; n > 9; n /= 10)
			buf[--i] = n % 10 + '0';
		buf[--i] = n + '0';
	}
	return buf;
}

i32	fputc_utf8(const u32 cp, FILE *stream) {
	char	buf[4];

	if (cp <= 0x7FU) {
		return fputc(cp, stream);
	} else if (cp <= 0x7FFU) {
		buf[0] = (_UTF8_LENGTH_2BS | ((cp & 0x07C0U) >> 6));
		buf[1] = (_UTF8_BYTE_START | (cp & 0x003FU));
		return (fwrite(buf, 1, 2, stream) == 2) ? cp : EOF;
	} else if (cp <= 0xFFFFU) {
		buf[0] = (_UTF8_LENGTH_3BS | ((cp & 0xF000U) >> 12));
		buf[1] = (_UTF8_BYTE_START | ((cp & 0x0FC0U) >> 6));
		buf[2] = (_UTF8_BYTE_START | (cp & 0x003FU));
		return (fwrite(buf, 1, 3, stream) == 3) ? cp : EOF;
	} else if (cp <= 0x10FFFFU) {
		buf[0] = (_UTF8_LENGTH_4BS | ((cp & 0x1C0000U) >> 18));
		buf[1] = (_UTF8_BYTE_START | ((cp & 0x03F000U) >> 12));
		buf[2] = (_UTF8_BYTE_START | ((cp & 0x000FC0U) >> 6));
		buf[3] = (_UTF8_BYTE_START | (cp & 0x00003FU));
		return (fwrite(buf, 1, 4, stream) == 4) ? cp : EOF;
	}
	return EOF;
}

f32	roundf_f(const f32 n, const f32 factor) {
	f32	remainder;

	remainder = fmodf(n, factor);
	if (remainder >= factor / 2)
		return n - remainder + factor;
	return n - remainder;
}

static inline size_t	_uintlen(u64 n) {
	size_t	len;

	for (len = 1; n > 9; n /= 10)
		len++;
	return len;
}

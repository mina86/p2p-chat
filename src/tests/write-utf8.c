/** \file
 * An UTF-8 encoder tester.
 * $Id: write-utf8.c,v 1.1 2008/01/21 01:27:57 mina86 Exp $
 */

#include <stdio.h>


static char *writeUTF8(char *wr, unsigned long value) {
	if (value < 128) {
		*wr = value;
	} else {
		char buffer[20];
		unsigned char *ch, *end, mask = 0x3f;
		ch = end = (unsigned char*)buffer + sizeof buffer;

		do {
			*--ch = 0x80 | (value & 0x3f);
			mask >>= 1;
		} while ((value >>= 6) & ~mask);

		for (*wr = (~mask << 1) | value; ch != end; ++ch) {
			*++wr = *ch;
		}
	}
	return wr + 1;
}


int main(void) {
	static const char *const nibbles[16] = {
		"0000", "0001", "0010", "0011",
		"0100", "0101", "0110", "0111",
		"1000", "1001", "1010", "1011",
		"1100", "1101", "1110", "1111"
	};

	unsigned long value;
	char buffer[20], *end, *ch;
	unsigned shift;

	while (scanf("%lu", &value)==1) {
		end = writeUTF8(buffer, value);

		shift = 32;
		do {
			shift -= 4;
			printf("  %s" + !(shift & 4), nibbles[(value >> shift) & 15]);
		} while (shift);
		putchar('\n');

		for (ch = buffer; ch!=end; ++ch) {
			printf("  %s %s", nibbles[(unsigned char)*ch >> 4],
			       nibbles[*ch & 15]);
		}
		putchar('\n');
	}


	return 0;
}

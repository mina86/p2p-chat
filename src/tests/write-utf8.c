/** \file
 * An UTF-8 encoder tester.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

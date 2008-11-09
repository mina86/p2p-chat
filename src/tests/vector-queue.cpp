/** \file
 * A vector-queue implementation tester.
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
#include <stdlib.h>
#include <time.h>

#include <string>

#include "../vector-queue.hpp"


static int interactive(void);
static int noninteractive(void);


int main(int argc, char **argv) {
	(void)argv;
	return argc > 1 ? noninteractive() : interactive();
}


static int interactive() {
	std::queue< std::string, std::vector<std::string> > queue;
	char buffer[1024];

	fputs("> ", stdout);
	while (fgets(buffer, sizeof buffer, stdin)) {
		if (!strncmp(buffer, "push ", 5)) {
			queue.push(std::string(buffer + 5));
			printf(":: %s", queue.back().c_str());
		} else if (strcmp(buffer, "pop\n") && strcmp(buffer, "peek\n")) {
			/* nothing */
		} else if (queue.empty()) {
			puts(":: empty");
		} else {
			printf(":: %s", queue.front().c_str());
			if (buffer[1] == 'o') {
				queue.front().clear();
				queue.pop();
			}
		}
		fputs("> ", stdout);
	}

	putchar('\n');
	return 0;
}



struct nonint_data {
	std::queue< unsigned long, std::vector<unsigned long> > queue;
	unsigned long outer;
	unsigned long inner;
	unsigned long poped;
	unsigned long pushed;
	unsigned count;
	int ret;
};

static void check_size(struct nonint_data &data) {
	if (data.queue.size() != data.count) {
		fprintf(stderr, "%lu.%9lu: size %6u, expected %6u\n",
		        data.outer,data.inner, data.queue.size(), data.count);
		data.ret = 1;
	}
}

static void pop(struct nonint_data &data) {
	unsigned long val = data.queue.front();
	if (val != ++data.poped) {
		fprintf(stderr, "%lu.%9lu: poped %10lu, expected %10lu\n",
		        data.outer, data.inner, val, data.poped);
		data.ret = 1;
	}
	data.queue.pop();
	--data.count;
	check_size(data);
}

static void push(struct nonint_data &data) {
	unsigned long val;

	data.queue.push(++data.pushed);
	++data.count;
	if ((val = data.queue.back()) != data.pushed) {
		fprintf(stderr, "%lu.%9lu: pushed %10lu but there is %10lu\n",
		        data.outer, data.inner, data.pushed, val);
		data.ret = 1;
	}
	check_size(data);
}

static void debug(struct nonint_data &data, const char *delim = "...") {
	printf("%s %6u, %10lu, %10lu %s\n",
	       delim, data.count, data.poped, data.pushed, delim);
}

static int noninteractive() {
	struct nonint_data data;

	data.poped = data.pushed = -1;
	data.count = 0;
	data.ret = 0;

	{
		unsigned int seed = time(0);
		srand(seed);
		printf("seed: %u\n", seed);
	}

	data.outer = 0;
	do {
		data.inner = 0;
		do {
			if (rand() / (RAND_MAX + 1.0) * 500.0 >= data.count) {
				push(data);
			} else {
				pop(data);
			}

			if ((++data.inner % 1000000) == 0) {
				debug(data);
			}
		} while (data.inner < 10000000);

		debug(data, "---");
		while (data.count) {
			pop(data);
		}
		debug(data, "---");

	} while (++data.outer < 10);

	return data.ret;
}

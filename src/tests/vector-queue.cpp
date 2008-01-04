/** \file
 * A vector-queue implementation tester.
 * $Id: vector-queue.cpp,v 1.1 2008/01/04 14:01:04 mina86 Exp $.
 */

#include <stdio.h>

#include <string>

#include "../vector-queue.hpp"


int main(void) {
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

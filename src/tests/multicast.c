/** \file
 * UPF multicast tester.
 * $Id: multicast.c,v 1.2 2008/01/04 11:17:00 mina86 Exp $
 */


#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


static const int yes = 1;
static const char *argv0;


static int read_from_stdin(int fd, const struct sockaddr *addr, int size);
static int recvieve_from_socket(int fd);


int main(int argc, char **argv) {
	struct sockaddr_in addr;
	unsigned short group_port; /* this is is network byte order */
	unsigned long group_ip;    /* this is is network byte order */
	int fd, ret = 1;

	argv0 = argv[0];

	/* No multicast loop? */
	if (argc > 1 && !strcmp(argv[1], "-l")) {
		ret = 0;
		--argc;
		++argv;
	}

	/* Check arguments */
	if (argc != 3) {
		fprintf(stderr, "usage: %s [ -l ] <ip-address> <port>\n", argv0);
		return 1;
	}

	/* Parse first argument */
	group_ip = inet_addr(argv[1]);
	if (group_ip == INADDR_NONE) {
		fprintf(stderr, "%s: %s: %s\n", argv0, argv[1],
		        "invalid IP address");
		return 1;
	}

	/* Parse second argument */
	{
		unsigned long tmp;
		char *end;
		errno = 0;
		tmp = strtoul(argv[2], &end, 10);
		if (errno || *end || !tmp || tmp > 0xffff) {
			fprintf(stderr, "%s: %s: %s\n", argv0, argv[1],
			        "invalid port number");
			return 1;
		}
		group_port = htons(tmp);
	}

	/* Create socket */
	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "socket", strerror(errno));
		return 1;
	}

	/* So that many clients may use the same address */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "setsockopt: so_reuseaddr",
		        strerror(errno));
		return 1;
	}

	/* Bind to port */
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = group_port;
	if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "bind", strerror(errno));
		return 1;
	}

	/* Set multicast loop */
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &ret, sizeof ret) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0,
		        "setsockopt: ip_multicast_loop", strerror(errno));
		return 1;
	}

	/* Add socket to multicast group */
	{
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = group_ip;
		mreq.imr_interface.s_addr = 0;
		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		               &mreq, sizeof(mreq)) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0,
			        "setsockopt: ip_add_membership", strerror(errno));
			return 1;
		}
	}

	/* Main loop */
	addr.sin_addr.s_addr = group_ip;
	do {
		fd_set rd;

		FD_ZERO(&rd);
		FD_SET(0, &rd);
		FD_SET(fd, &rd);

		ret = select(fd + 1, &rd, 0, 0, 0);
		if (ret < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, "select",
			        strerror(errno));
			return 1;
		}

		if (FD_ISSET(0, &rd)) {
			ret = read_from_stdin(fd, (struct sockaddr*)&addr, sizeof addr);
		}

		if (ret > 0 && FD_ISSET(fd, &rd)) {
			if (recvieve_from_socket(fd) < 0) return 1;
		}
	} while (ret > 0);

	/* Finish */
	close(fd);
	return -ret;
}



static int read_from_stdin(int fd, const struct sockaddr *addr, int size) {
	char buffer[4096];
	ssize_t ret;

	ret = read(0, buffer, sizeof buffer);
	if (ret < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "read", strerror(errno));
		return -1;
	} else if (!ret) {
		return 0;
	}

	if (buffer[ret - 1] == '\n') {
		--ret;
	}

	if (!ret) {
		return 1;
	}

	if (sendto(fd, buffer, ret, 0, addr, size) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "sendto", strerror(errno));
		return -1;
	}

	return 1;
}


static int recvieve_from_socket(int fd) {
	struct sockaddr_in addr;
	socklen_t addrlen;
	char buffer[4096];
	ssize_t ret;

	addrlen = sizeof addr;
	ret = recvfrom(fd, buffer, sizeof buffer - 1, 0,
	               (struct sockaddr *)&addr, &addrlen);
	if (ret < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, "recvfrom", strerror(errno));
		return -1;
	}

	buffer[ret] = 0;
	printf("data from %s:%d: '%s'\n", inet_ntoa(addr.sin_addr),
	       ntohs(addr.sin_port), buffer);
	return 1;
}

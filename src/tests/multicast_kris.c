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
	struct sockaddr_in addr, our_addr;
	unsigned short group_port; /* this is is network byte order */
	unsigned long group_ip;    /* this is is network byte order */
	int fd_send, fd_recv, ret = 1;
	
	argv0 = argv[0];
	
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
	
	if ((fd_send = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		return 1;
	}
	if ((fd_recv = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		return 1;
	}

	if (setsockopt(fd_send, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
		return 1;
	}

	if (setsockopt(fd_recv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
		return 1;
	}


	memset(&addr, 0, sizeof addr);
	our_addr.sin_family = AF_INET;
	our_addr.sin_addr.s_addr = group_ip;
	our_addr.sin_port = group_port;
	if (bind(fd_send, (struct sockaddr *)&addr, sizeof addr) < 0) {
		return 1;
	}
	
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = group_port;
	if (bind(fd_recv, (struct sockaddr *)&addr, sizeof addr) < 0) {
		return 1;
	}

/*	if (setsockopt(fd_recv, IPPROTO_IP, IP_MULTICAST_LOOP, &ret, sizeof ret) < 0) {
		return 1;
	}*/
	
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = group_ip;
	mreq.imr_interface.s_addr = inet_addr("10.0.0.1");
	if (setsockopt(fd_recv, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		return 1;
	}

/***************************************************************************************/

	/* Main loop */
	addr.sin_addr.s_addr = group_ip;
	do {
		fd_set rd;

		FD_ZERO(&rd);
		FD_SET(0, &rd);
		FD_SET(fd_recv, &rd);

		ret = select(fd_recv + 1, &rd, 0, 0, 0);
		if (ret < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, "select",
			        strerror(errno));
			return 1;
		}

		if (FD_ISSET(0, &rd)) {
			ret = read_from_stdin(fd_send, (struct sockaddr*)&addr, sizeof addr);
		}

		if (ret > 0 && FD_ISSET(fd_recv, &rd)) {
			if (recvieve_from_socket(fd_recv) < 0) return 1;
		}
	} while (ret > 0);

	/* Finish */
	close(fd_recv);
	close(fd_send);
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

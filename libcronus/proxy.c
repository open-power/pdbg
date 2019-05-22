/* Copyright 2019 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define CRONUS_PORT	8192

static bool set_nonblocking(int fd)
{
	int val;

	val = fcntl(fd, F_GETFL, 0);
	if (val == -1) {
		perror("fcntl(F_GETFL)");
		return false;
	}

	val |= O_NONBLOCK;

	val = fcntl(fd, F_SETFL, val);
	if (val == -1) {
		perror("fcntl(F_SETFL)");
		return false;
	}

	return true;
}

static int listen_socket(unsigned short port)
{
	struct sockaddr_in addr;
	int fd, ret;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		goto fail;

	if (!set_nonblocking(fd))
		goto fail;

	addr = (struct sockaddr_in) {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = htonl(INADDR_ANY),
	};

	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		perror("bind");
		goto fail;
	}

	ret = listen(fd, 1);
	if (ret != 0) {
		perror("listen");
		goto fail;
	}

	return fd;

fail:
	if (fd != -1)
		close(fd);
	return -1;
}

static int connect_socket(struct sockaddr_in *addr)
{
	int fd, ret;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		goto fail;

	ret = connect(fd, (struct sockaddr *)addr, sizeof(*addr));
	if (ret == -1) {
		perror("connect");
		goto fail;
	}

	if (!set_nonblocking(fd))
		goto fail;

	return fd;

fail:
	if (fd != -1)
		close(fd);
	return -1;
}

static int copy_data(int src_fd, int dst_fd, const char *prefix)
{
	uint8_t data[1024];
	ssize_t n1, n2;
	unsigned int i;

	n1 = read(src_fd, data, 1024);
	if (n1 <= 0)
		return -1;

	fprintf(stderr, "read data fd=%d, len=%zi\n", src_fd, n1);

	for (i=0; i<n1; i++) {
		if (i % 16 == 0)
			fprintf(stderr, "%s: 0x%08x", prefix, i);

		fprintf(stderr, " %02x", data[i]);

		if ((i+1) % 16 == 0)
			fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");

	n2 = write(dst_fd, data, n1);
	if (n2 <= 0 || n1 != n2)
		return -1;

	return 0;
}

static int event_loop(int listen_fd, struct sockaddr_in *cro_addr)
{
	struct epoll_event ev;
	int client_fd = -1, proxy_fd = -1;
	int epollfd, ret;

	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epollfd");
		goto fail;
	}

	ev.events = EPOLLIN;
	ev.data.fd = listen_fd;

	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_fd, &ev);
	if (ret == -1) {
		perror("epoll_ctl(ADD, listen_fd)");
		goto fail;
	}

	while (1) {
		struct epoll_event event;
		int nfds, fd;

		nfds = epoll_wait(epollfd, &event, 1, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			goto fail;
		}

		if (event.data.fd == listen_fd) {
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);

			fd = accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
			if (fd == -1) {
				perror("accept");
				continue;
			}

			if (client_fd != -1) {
				fprintf(stderr, "Dropping connection\n");
				close(fd);
				continue;
			}

			proxy_fd = connect_socket(cro_addr);
			if (proxy_fd == -1) {
				close(fd);
				continue;
			}

			if (!set_nonblocking(fd)) {
				close(proxy_fd);
				close(fd);
				continue;
			}

			fprintf(stderr, "accepted connection fd=%d\n", fd);
			client_fd = fd;

			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = client_fd;

			ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev);
			if (ret == -1) {
				perror("epoll_ctl(ADD, client_fd)");
				close(client_fd);
				client_fd = -1;
				close(proxy_fd);
				proxy_fd = -1;

				continue;
			}
			fprintf(stderr, "added fd=%d\n", client_fd);

			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = proxy_fd;

			ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, proxy_fd, &ev);
			if (ret == -1) {
				perror("epoll_ctl(ADD, proxy_fd)");
				close(client_fd);
				client_fd = -1;
				close(proxy_fd);
				proxy_fd = -1;
			}

			continue;
		}

		ret = 0;
		if (event.data.fd == client_fd) {
			ret = copy_data(client_fd, proxy_fd, "REQUEST");
		} else if (event.data.fd == proxy_fd) {
			ret = copy_data(proxy_fd, client_fd, "REPLY");
		}
		if (ret != 0) {
			fprintf(stderr, "closed fd=%d\n", client_fd);
			epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL);
			close(client_fd);
			client_fd = -1;
			epoll_ctl(epollfd, EPOLL_CTL_DEL, proxy_fd, NULL);
			close(proxy_fd);
			proxy_fd = -1;
		}
	}


fail:
	if (epollfd != -1) {
		if (client_fd != -1) {
			epoll_ctl(epollfd, EPOLL_CTL_DEL, client_fd, NULL);
			close(client_fd);
		}
		if (proxy_fd != -1) {
			epoll_ctl(epollfd, EPOLL_CTL_DEL, proxy_fd, NULL);
			close(proxy_fd);
		}

		epoll_ctl(epollfd, EPOLL_CTL_DEL, listen_fd, NULL);
		close(listen_fd);
	}

	return -1;
}

int main(int argc, const char **argv)
{
	struct addrinfo hints, *result;
	struct sockaddr_in addr;
	const char *hostname;
	unsigned short port = CRONUS_PORT;
	int listen_fd, ret;

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <croserver-ip> [<croserver-port>]\n", argv[0]);
		exit(1);
	}

	hostname = argv[1];
	if (argc == 3) {
		port = atoi(argv[2]);
	}

	hints = (struct addrinfo) {
		.ai_family = AF_INET,
	};

	ret = getaddrinfo(hostname, NULL, &hints, &result);
	if (ret != 0) {
		perror("getaddrinfo");
		exit(1);
	}

	addr = *(struct sockaddr_in *)result->ai_addr;
	addr.sin_port = htons(port);

	freeaddrinfo(result);

	listen_fd = listen_socket(CRONUS_PORT);
	if (listen_fd == -1)
		exit(1);

	return event_loop(listen_fd, &addr);
}

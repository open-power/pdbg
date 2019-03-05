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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "libcronus_private.h"
#include "libcronus.h"

static int cronus_wait_for_feedbobo(struct cronus_context *cctx)
{
	uint32_t pkt[10];
	ssize_t n;
	uint32_t word;

	n = read(cctx->fd, pkt, sizeof(pkt));
	if (n == -1) {
		int ret = errno;
		perror("read");
		return ret;
	}

	if (n != 2 * sizeof(uint32_t)) {
		fprintf(stderr, "Insufficient data from server\n");
		return EPROTO;
	}

	word = ntohl(pkt[1]);
	if ((word & 0xFFFFFFF0) != 0xFEEDB0B0) {
		fprintf(stderr, "Invalid response 0x%08x from server\n", word);
		return EPROTO;
	}

	cctx->server_version = word & 0x0000000F;

	return 0;
}

int cronus_connect(const char *hostname, struct cronus_context **out)
{
	struct cronus_context *cctx;
	struct addrinfo hints;
	struct addrinfo *result;
	struct sockaddr_in addr;
	extern int h_errno;
	int fd, ret;

	if (!hostname || !out)
		return EINVAL;

	hints = (struct addrinfo) {
		.ai_family = AF_INET,
	};

	ret = getaddrinfo(hostname, NULL, &hints, &result);
	if (ret != 0) {
		herror("getaddrinfo");
		return EIO;
	}

	cctx = malloc(sizeof(struct cronus_context));
	if (!cctx) {
		fprintf(stderr, "Memory allocation error\n");
		return ENOMEM;
	}

	*cctx = (struct cronus_context) {
		.fd = -1,
		.key = 0x11111111,
	};

	addr = *(struct sockaddr_in *)result->ai_addr;
	addr.sin_port = htons(8192);

	freeaddrinfo(result);

	fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd == -1) {
		ret = errno;
		perror("socket");
		return ret;
	}

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		ret = errno;
		perror("connect");
		close(fd);
		return ret;
	}

	cctx->fd = fd;

	ret = cronus_wait_for_feedbobo(cctx);
	if (ret) {
		close(fd);
		return ret;
	}

	*out = cctx;
	return 0;
}

void cronus_disconnect(struct cronus_context *cctx)
{
	assert(cctx);

	if (cctx->fd != -1) {
		close(cctx->fd);
		cctx->fd = -1;
	}

	free(cctx);
}

uint32_t cronus_key(struct cronus_context *cctx)
{
	uint32_t key = cctx->key;

	cctx->key += 1;

	return key;
}

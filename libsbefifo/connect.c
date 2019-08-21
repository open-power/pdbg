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
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

#include "libsbefifo.h"
#include "sbefifo_private.h"

int sbefifo_connect(const char *fifo_path, struct sbefifo_context **out)
{
	struct sbefifo_context *sctx;
	int fd, rc;

	sctx = malloc(sizeof(struct sbefifo_context));
	if (!sctx) {
		fprintf(stderr, "Memory allocation error\n");
		return ENOMEM;
	}

	*sctx = (struct sbefifo_context) {
		.fd = -1,
	};

	fd = open(fifo_path, O_RDWR | O_SYNC);
	if (fd < 0) {
		rc = errno;
		fprintf(stderr, "Error opening fifo %s\n", fifo_path);
		free(sctx);
		return rc;
	}

	sctx->fd = fd;

	*out = sctx;
	return 0;
}

void sbefifo_disconnect(struct sbefifo_context *sctx)
{
	if (sctx->fd != -1)
		close(sctx->fd);

	if (sctx->ffdc)
		free(sctx->ffdc);

	free(sctx);
}

void sbefifo_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

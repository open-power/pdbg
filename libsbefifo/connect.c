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
#include <stdbool.h>
#include <sys/ioctl.h>

#ifdef HAVE_LINUX_FSI_H
#include <linux/fsi.h>
#else
#include <linux/ioctl.h>
#endif /* HAVE_LINUX_FSI_H */

#ifndef FSI_SBEFIFO_READ_TIMEOUT
#define FSI_SBEFIFO_READ_TIMEOUT        _IOW('s', 0x00, unsigned int)
#endif

#include "libsbefifo.h"
#include "sbefifo_private.h"

static bool proc_valid(int proc)
{
	if (proc == SBEFIFO_PROC_P9 || proc == SBEFIFO_PROC_P10)
		return true;

	return false;
}

int sbefifo_connect(const char *fifo_path, int proc, struct sbefifo_context **out)
{
	struct sbefifo_context *sctx;
	int fd, rc;

	if (!proc_valid(proc))
		return EINVAL;

	sctx = malloc(sizeof(struct sbefifo_context));
	if (!sctx) {
		fprintf(stderr, "Memory allocation error\n");
		return ENOMEM;
	}

	*sctx = (struct sbefifo_context) {
		.fd = -1,
		.proc = proc,
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

int sbefifo_connect_transport(int proc, sbefifo_transport_fn transport, void *priv, struct sbefifo_context **out)
{
	struct sbefifo_context *sctx;

	if (!proc_valid(proc))
		return EINVAL;

	sctx = malloc(sizeof(struct sbefifo_context));
	if (!sctx) {
		fprintf(stderr, "Memory allocation error\n");
		return ENOMEM;
	}

	*sctx = (struct sbefifo_context) {
		.fd = -1,
		.proc = proc,
		.transport = transport,
		.priv = priv,
	};

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

int sbefifo_proc(struct sbefifo_context *sctx)
{
	return sctx->proc;
}

int sbefifo_set_timeout(struct sbefifo_context *sctx, unsigned int timeout_ms)
{
        unsigned int timeout = timeout_ms/1000;
        int rc;

        LOG("long_timeout: %u sec\n", timeout);
        rc = ioctl(sctx->fd, FSI_SBEFIFO_READ_TIMEOUT, &timeout);
        if (rc == -1 && errno == ENOTTY) {
                /* Do not fail if kernel does not implement ioctl */
                rc = 0;
        }

        return rc;
}

int sbefifo_set_long_timeout(struct sbefifo_context *sctx)
{
	unsigned int long_timeout = 30;
	int rc;

	LOG("long_timeout: %u sec\n", long_timeout);
	rc = ioctl(sctx->fd, FSI_SBEFIFO_READ_TIMEOUT, &long_timeout);
	if (rc == -1 && errno == ENOTTY) {
		/* Do not fail if kernel does not implement ioctl */
		rc = 0;
	}

	return rc;
}

int sbefifo_set_long_long_timeout(struct sbefifo_context *sctx)
{
	unsigned int long_timeout = 120;
	int rc;

	LOG("long_timeout: %u sec\n", long_timeout);
	rc = ioctl(sctx->fd, FSI_SBEFIFO_READ_TIMEOUT, &long_timeout);
	if (rc == -1 && errno == ENOTTY) {
		/* Do not fail if kernel does not implement ioctl */
		rc = 0;
	}

	return rc;
}
int sbefifo_reset_timeout(struct sbefifo_context *sctx)
{
	unsigned int timeout = 0;
	int rc;

	LOG("reset_timeout\n");
	rc = ioctl(sctx->fd, FSI_SBEFIFO_READ_TIMEOUT, &timeout);
	if (rc == -1 && errno == ENOTTY) {
		/* Do not fail if kernel does not implement ioctl */
		rc = 0;
	}

	return rc;
}

void sbefifo_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

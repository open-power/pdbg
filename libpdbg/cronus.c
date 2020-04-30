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
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <libcronus/libcronus.h>
#include <libsbefifo/libsbefifo.h>

#include "hwunit.h"
#include "debug.h"

static struct cronus_context *cctx;
static int cctx_refcount;

static int croserver_connect(const char *server)
{
	int ret = 0;

	if (!cctx)
		ret = cronus_connect(server, &cctx);

	if (ret == 0)
		cctx_refcount++;

	return ret;
}

static void croserver_disconnect(void)
{
	if (cctx) {
		cctx_refcount--;

		if (cctx_refcount <= 0) {
			cronus_disconnect(cctx);
			cctx = NULL;
		}
	}
}


/*
 * Initialise a cronus backend for the given server address.
 */
static int cronus_probe(struct pdbg_target *target)
{
	const char *tmp, *server = NULL;

	tmp = pdbg_get_backend_option();
	if (tmp) {
		tmp = strchr(tmp, '@');
		if (tmp && tmp[1] != '\0')
			server = tmp + 1;
	}

	if (!server)
		server = pdbg_target_property(target, "server", NULL);

	if (!server) {
		PR_ERROR("CRONUS backend requires server\n");
		return -1;
	}

	return croserver_connect(server);
}

static void cronus_release(struct pdbg_target *target)
{
	croserver_disconnect();
}

static int cronus_pib_read(struct pib *pib, uint64_t addr, uint64_t *value)
{
	int ret;

	ret = cronus_getscom(cctx, pdbg_target_index(&pib->target), addr, value);
	if (ret) {
		PR_ERROR("cronus: getscom failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

static int cronus_pib_write(struct pib *pib, uint64_t addr, uint64_t value)
{
	int ret;

	ret = cronus_putscom(cctx, pdbg_target_index(&pib->target), addr, value);
	if (ret) {
		PR_ERROR("cronus: putscom failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

static int cronus_fsi_read(struct fsi *fsi, uint32_t addr, uint32_t *value)
{
	int ret;

	ret = cronus_getcfam(cctx, pdbg_target_index(&fsi->target), addr, value);
	if (ret) {
		PR_ERROR("cronus: getcfam failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

static int cronus_fsi_write(struct fsi *fsi, uint32_t addr, uint32_t value)
{
	int ret;

	ret = cronus_putcfam(cctx, pdbg_target_index(&fsi->target), addr, value);
	if (ret) {
		PR_ERROR("cronus: putcfam failed, ret=%d\n", ret);
		return -1;
	}

	return 0;
}

static struct sbefifo_context *cronus_sbefifo_context(struct sbefifo *sbefifo)
{
	return sbefifo->sf_ctx;
}

static int cronus_sbefifo_transport(uint8_t *msg, uint32_t msg_len,
				    uint8_t *out, uint32_t *out_len,
				    void *priv)
{
	struct sbefifo *sf = (struct sbefifo *)priv;

	return cronus_submit(cctx, pdbg_target_index(&sf->target),
			     msg, msg_len, out, out_len);
}

static int cronus_sbefifo_probe(struct pdbg_target *target)
{
	struct sbefifo *sf = target_to_sbefifo(target);
	int rc;

	rc = cronus_probe(target);
	if (rc)
		return rc;

	rc = sbefifo_connect_transport(cronus_sbefifo_transport, sf, &sf->sf_ctx);
	if (rc) {
		PR_ERROR("Unable to initialize sbefifo driver\n");
		return rc;
	}

	return 0;
}

static void cronus_sbefifo_release(struct pdbg_target *target)
{
	struct sbefifo *sf = target_to_sbefifo(target);

	sbefifo_disconnect(sf->sf_ctx);
	cronus_release(target);
}

static struct pib cronus_pib = {
	.target = {
		.name =	"Cronus Client based PIB",
		.compatible = "ibm,cronus-pib",
		.class = "pib",
		.probe = cronus_probe,
		.release = cronus_release,
	},
	.read = cronus_pib_read,
	.write = cronus_pib_write,
};
DECLARE_HW_UNIT(cronus_pib);

static struct fsi cronus_fsi = {
	.target = {
		.name =	"Cronus Client based FSI",
		.compatible = "ibm,cronus-fsi",
		.class = "fsi",
		.probe = cronus_probe,
		.release = cronus_release,
	},
	.read = cronus_fsi_read,
	.write = cronus_fsi_write,
};
DECLARE_HW_UNIT(cronus_fsi);

static struct sbefifo cronus_sbefifo = {
	.target = {
		.name =	"Cronus Client based SBE FIFO",
		.compatible = "ibm,cronus-sbefifo",
		.class = "sbefifo_transport",
		.probe = cronus_sbefifo_probe,
		.release = cronus_sbefifo_release,
	},
	.get_sbefifo_context = cronus_sbefifo_context,
};
DECLARE_HW_UNIT(cronus_sbefifo);

__attribute__((constructor))
static void register_cronus(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &cronus_pib_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &cronus_fsi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &cronus_sbefifo_hw_unit);
}

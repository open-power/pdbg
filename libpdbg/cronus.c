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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libcronus/libcronus.h>

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

	tmp = strchr(pdbg_get_backend_option(), '@');
	if (tmp && tmp[1] != '\0')
		server = tmp + 1;

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

__attribute__((constructor))
static void register_cronus(void)
{
	pdbg_hwunit_register(&cronus_pib_hw_unit);
	pdbg_hwunit_register(&cronus_fsi_hw_unit);
}

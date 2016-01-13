/* Copyright 2016 IBM Corp.
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
#ifndef __OPERATIONS_H
#define __OPERATIONS_H

/* Error codes */
#define EFSI 1

#define PR_DEBUG(x, args...) \
	//fprintf(stderr, x, ##args)
#define PR_INFO(x, args...) \
	fprintf(stderr, x, ##args)
#define PR_ERROR(x, args...) \
	fprintf(stderr, "%s: " x, __FUNCTION__, ##args)

#define CHECK_ERR(x) do {					\
	if (x) {	       					\
		PR_DEBUG("%s: %d\n", __FUNCTION__, __LINE__);	\
		return -EFSI;					\
	}							\
	} while(0)

#define THREADS_PER_CORE	8

/* Structure to allow alternative backend implentations */
struct scom_backend {
	void (*destroy)(struct scom_backend *backend);
	int (*getscom)(struct scom_backend *backend, int processor_id, uint64_t *value, uint32_t addr);
	int (*putscom)(struct scom_backend *backend, int processor_id, uint64_t value, uint32_t addr);
	int (*getcfam)(struct scom_backend *backend, int processor_id, uint32_t *value, uint32_t addr);
	int (*putcfam)(struct scom_backend *backend, int processor_id, uint32_t value, uint32_t addr);
	int processor_id;
	void *priv;
};

/* Alter display unit functions */
int adu_getmem(uint64_t addr, uint8_t *output, uint64_t size, int block_size);

/* Functions to ram instructions */
int ram_getgpr(int chip, int thread, int gpr, uint64_t *value);
int ram_getnia(int chip, int thread, uint64_t *value);
int ram_getspr(int chip, int thread, int spr, uint64_t *value);
int ram_running_threads(uint64_t chip, uint64_t *active_threads);
int ram_stop_chip(uint64_t chip, uint64_t *thread_active);
int ram_start_chip(uint64_t chip, uint64_t thread_active);

/* GDB server functionality */
int gdbserver_start(uint16_t port);

/* scom backend functions. Most other operations use these. */
int backend_init(int processor_id);
void backend_destroy(void);
void backend_set_processor(int processor_id);
int getscom(uint64_t *value, uint32_t addr);
int putscom(uint64_t value, uint32_t addr);
int getcfam(uint32_t *value, uint32_t addr);
int putcfam(uint32_t value, uint32_t addr);

#endif

/* Copyright 2018 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 */

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include "progress.h"

static uint64_t progress_pcent;
static uint64_t progress_n_upd;
static time_t progress_prevsec;
static struct timespec progress_start;
static bool shutup;

#define PROGRESS_CHARS	50

static void progress_bar(unsigned int percent)
{
	unsigned int i, progress;

	assert(percent <= 100);

	if (shutup)
		return;

	progress = (percent * PROGRESS_CHARS) / 101;

	fprintf(stderr, "\r[");
	for (i = 0; i <= progress; i++)
		fprintf(stderr, "=");
	for (; i < PROGRESS_CHARS; i++)
		fprintf(stderr, " ");
	fprintf(stderr, "] %u%%", percent);
	fflush(stderr);
}

void progress_shutup(void)
{
	shutup = true;
}

void progress_init(void)
{
	progress_pcent = 0;
	progress_n_upd = ULONG_MAX;
	progress_prevsec = ULONG_MAX;

	progress_bar(0);

	clock_gettime(CLOCK_MONOTONIC, &progress_start);
}

void progress_tick(uint64_t cur, uint64_t end)
{
	struct timespec now;
	unsigned int pcent;
	double sec;

	if (shutup)
		return;

	pcent = (unsigned int)((cur * 100) / end);
	if (progress_pcent == pcent && cur < progress_n_upd &&
	    cur < end)
		return;
	progress_pcent = pcent;
	clock_gettime(CLOCK_MONOTONIC, &now);

	progress_bar(pcent);

	sec = difftime(now.tv_sec, progress_start.tv_sec);
	if (sec >= 5 && pcent > 0) {
		uint64_t persec = cur / sec;
		uint64_t rem_sec;

		if (!persec)
			persec = 1;
		progress_n_upd = cur + persec;
		rem_sec = ((sec * 100) + (pcent / 2)) / pcent - sec;
		if (rem_sec > progress_prevsec)
			rem_sec = progress_prevsec;
		progress_prevsec = rem_sec;
		if (rem_sec < 60)
			fprintf(stderr, " ETA:%" PRIu64 "s     ", rem_sec);
		else {
			fprintf(stderr, " ETA:%" PRIu64 ":%02" PRIu64 ":%02" PRIu64 " ",
				rem_sec / 3600,
				(rem_sec / 60) % 60,
				rem_sec % 60);
		}
	}

	fflush(stderr);
}

void progress_end(void)
{
	if (shutup)
		return;

	fprintf(stderr, "\n");
}

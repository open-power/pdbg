#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "libpdbg.h"

uint64_t *parse_number64(const char *argv)
{
	uint64_t *n = malloc(sizeof(*n));
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	*n = strtoull(argv, &endptr, 0);
	if (errno || *endptr != '\0')
		return NULL;

	return n;
}

uint32_t *parse_number32(const char *argv)
{
	unsigned long long tmp;
	uint32_t *n = malloc(sizeof(*n));
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	tmp = strtoul(argv, &endptr, 0);
	if (errno || *endptr != '\0' || tmp > UINT32_MAX)
		return NULL;

	*n = tmp;
	return n;
}

uint16_t *parse_number16(const char *argv)
{
	unsigned long long tmp;
	uint16_t *n = malloc(sizeof(*n));
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	tmp = strtoul(argv, &endptr, 0);
	if (errno || *endptr != '\0' || tmp > UINT16_MAX)
		return NULL;

	*n = tmp;
	return n;
}

uint8_t *parse_number8(const char *argv)
{
	unsigned long long tmp;
	uint8_t *n = malloc(sizeof(*n));
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	tmp = strtoul(argv, &endptr, 0);
	if (errno || *endptr != '\0' || tmp > UINT8_MAX)
		return NULL;

	*n = tmp;
	return n;
}

/* Parse an 8-bit number that is a power of 2 */
uint8_t *parse_number8_pow2(const char *argv)
{
	uint8_t *n;
	unsigned long int tmp, i;
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	tmp = strtoul(argv, &endptr, 0);
	if (tmp >= 256)
		return NULL;

	for (i = tmp; i; i >>= 1)
		if ((i & 1) && (i >> 1))
			return NULL;

	n = malloc(sizeof(*n));
	*n = tmp;

	return n;
}

/* Parse a GPR number, returning an error if it's greater than 32 */
int *parse_gpr(const char *argv)
{
	int *gpr = malloc(sizeof(*gpr));
	char *endptr;

	if (!argv)
		return NULL;

	errno = 0;
	*gpr = strtoul(argv, &endptr, 0);
	if (errno || *endptr != '\0' || *gpr > 32)
		return NULL;

	return gpr;
}

/* Parse an SPR name (eg. lr) */
int *parse_spr(const char *argv)
{
	int *spr = malloc(sizeof(*spr));

	if (!argv)
		return NULL;

	*spr = pdbg_spr_by_name(argv);
	if (*spr == -1)
		return NULL;

	return spr;
}


/* A special parser that always returns true. Allows for boolean flags which
 * don't take arguments. Sets the associated field to true if specified,
 * otherwise sets it to the default value (usually false). */
bool *parse_flag_noarg(const char *argv)
{
	bool *result = malloc(sizeof(*result));

	*result = true;
	return result;
}

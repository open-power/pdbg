/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (C) 2024 IBM Corporation
 *
 * Device Tree Blob (DTB) Validator
 * 
 * This utility validates device tree binary files using libfdt.
 * It checks:
 * - Valid FDT magic number
 * - Proper FDT structure
 * - Valid header fields
 * - Structural integrity
 *
 * Exit codes:
 *   0 - Valid device tree
 *   1 - Invalid device tree or validation error
 *   2 - Usage error
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libfdt.h>

#define PROGRAM_NAME "validate_dtb"

static void print_usage(void)
{
    fprintf(stderr, "Usage: %s <device-tree-file>\n", PROGRAM_NAME);
    fprintf(stderr, "Validates a device tree binary file.\n");
    fprintf(stderr, "\nExit codes:\n");
    fprintf(stderr, "  0 - Valid device tree\n");
    fprintf(stderr, "  1 - Invalid device tree or validation error\n");
    fprintf(stderr, "  2 - Usage error\n");
}

static int validate_dtb_file(const char *filename)
{
    int fd = -1;
    struct stat st;
    void *fdt = NULL;
    int ret = 1;
    ssize_t bytes_read;

    /* Open the file */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", 
                filename, strerror(errno));
        return 1;
    }

    /* Get file size */
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Error: Cannot stat file '%s': %s\n", 
                filename, strerror(errno));
        goto cleanup;
    }

    /* Check for empty or zero-filled file */
    if (st.st_size == 0) {
        fprintf(stderr, "Error: File '%s' is empty\n", filename);
        goto cleanup;
    }

    /* Allocate memory for the DTB */
    fdt = malloc(st.st_size);
    if (!fdt) {
        fprintf(stderr, "Error: Cannot allocate memory for DTB\n");
        goto cleanup;
    }

    /* Read the entire file */
    bytes_read = read(fd, fdt, st.st_size);
    if (bytes_read != st.st_size) {
        fprintf(stderr, "Error: Failed to read complete file '%s'\n", filename);
        goto cleanup;
    }

    /* Perform full structure check - this validates everything:
     * - Magic number
     * - Header structure
     * - Size consistency
     * - Node structure
     * - String table integrity
     */
    ret = fdt_check_full(fdt, st.st_size);
    if (ret != 0) {
        fprintf(stderr, "Error: Invalid device tree in '%s': %s\n",
                filename, fdt_strerror(ret));
        goto cleanup;
    }

    /* Success - silent mode, only exit code 0 indicates success */
    ret = 0;

cleanup:
    if (fdt) {
        free(fdt);
    }
    if (fd >= 0) {
        close(fd);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        print_usage();
        return 2;
    }

    return validate_dtb_file(argv[1]);
}

#include <stdio.h>
#include <stdlib.h>

#include "../util.h"

static void test1(uint8_t *bytes, int size)
{
	hexdump(0, bytes, size, 1);
	hexdump(0, bytes, size, 2);
	hexdump(0, bytes, size, 4);
	hexdump(0, bytes, size, 8);
}

static void test2(uint8_t *bytes)
{
	int i;

	for (i=16; i>0; i--) {
		hexdump(0x1000, bytes, i, 1);
	}
}

static void test3(uint8_t *bytes)
{
	int i;

	for (i=15; i>=0; i--) {
		hexdump(0x1000+i, bytes, 16-i, 1);
	}
}

static void test4(uint8_t *bytes)
{
	int i;

	for (i=0; i<16; i++) {
		hexdump(0x2000+i, bytes, 16, 1);
	}
}

int main(int argc, const char **argv)
{
	uint8_t bytes[1000];
	int test_num, i;
	int count = 0;

	for (i=0; i<1000; i++)
		bytes[i] = i % 0xff;

	if (argc < 2) {
		exit(1);
	}

	test_num = atoi(argv[1]);

	switch (test_num) {
	case 1:
		if (argc == 3)
			count = atoi(argv[2]);
		if (count <= 0 || count > 1000)
			count = 100;

		test1(bytes, count);
		break;

	case 2:
		test2(bytes);
		break;

	case 3:
		test3(bytes);
		break;

	case 4:
		test4(bytes);
		break;

	default:
		exit(1);
	}

	return 0;
}

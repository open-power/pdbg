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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <assert.h>

#include "bitutils.h"
#include "operations.h"
#include "target.h"

#define FSI_CLK		4	//GPIOA4
#define FSI_DAT		5	//GPIOA5
#define CRONUS_SEL	6	//GPIOA6
#define PCIE_RST_N	13	//GPIOB5
#define PEX_PERST_N	14	//GPIOB6
#define POWER		33    	//GPIOE1
#define PGOOD		23    	//GPIOC7
#define FSI_ENABLE      24      //GPIOD0
#define FSI_DAT_EN	62	//GPIOH6

#define GPIO_BASE	0x1e780000
#define GPIO_DATA	0x0
#define GPIO_DIR	0x4
#define GPIOS_PER_REGBLOCK	(8 * 4)

#define CRC_LEN		4

/* FSI result symbols */
enum fsi_result {
	FSI_MERR_TIMEOUT = -2,
	FSI_MERR_C = -1,
	FSI_ACK = 0x0,
	FSI_BUSY = 0x1,
	FSI_ERR_A = 0x2,
	FSI_ERR_C = 0x3,
};

#define FSI_DATA0_REG	0x1000
#define FSI_DATA1_REG	0x1001
#define FSI_CMD_REG	0x1002
#define  FSI_CMD_REG_WRITE PPC_BIT32(0)

#define FSI_RESET_REG	0x1006
#define  FSI_RESET_CMD PPC_BIT32(0)

#define FSI_SET_PIB_RESET_REG 0x1007
#define  FSI_SET_PIB_RESET PPC_BIT32(0)

/* Clock delay in a for loop, determined by trial and error with
 * -O2 */
#define CLOCK_DELAY	3

/* For some reason the FSI2PIB engine dies with frequent
 * access. Letting it have a bit of a rest seems to stop the
 * problem. This sets the number of usecs to sleep between SCOM
 * accesses. */
#define FSI2PIB_RELAX	50

/* FSI private data */
static void *gpio_reg = NULL;
static int mem_fd = 0;

static void fsi_reset(struct target *target);

static uint32_t readl(void *addr)
{
	asm volatile("" : : : "memory");
	return *(volatile uint32_t *) addr;
}

static void writel(uint32_t val, void *addr)
{
	asm volatile("" : : : "memory");
	*(volatile uint32_t *) addr = val;
}

static const uintptr_t regblock_offsets[] = {
	0x000,		/* GPIO_A/B/C/D */
	0x020,		/* GPIO_E/F/G/H */
	0x070,		/* GPIO_I/J/K/L */
	0x078,		/* GPIO_M/N/O/P */
	0x080,		/* GPIO_Q/R/S/T */
	0x088,		/* GPIO_U/V/W/X */
	0x1e0,		/* GPIO_Y/Z/AA/AB */
	0x1e8,		/* GPIO_AC */
};
static const size_t num_regblocks = sizeof(regblock_offsets) /
	sizeof(*regblock_offsets);

static void *get_gpio_reg(int gpio)
{
	int block_index = gpio / GPIOS_PER_REGBLOCK;
	if (block_index >= num_regblocks || block_index < 0)
		fprintf(stderr, "Error: no GPIO regblock for GPIO #%d\n", gpio);
	return gpio_reg + regblock_offsets[block_index];
}

static int get_gpio_bit(int gpio)
{
	/* GPIO_AC has only one eight-GPIO block */
	if (gpio >= num_regblocks * GPIOS_PER_REGBLOCK - 24)
		fprintf(stderr, "Error: GPIO #%d out of bounds\n", gpio);
	return gpio % GPIOS_PER_REGBLOCK;
}

static int __attribute__((unused)) get_direction(int gpio)
{
	void *reg = get_gpio_reg(gpio) + GPIO_DIR;
	int bit = get_gpio_bit(gpio);

	return !!(readl(reg) & (1ULL << bit));
}

static void set_direction_out(int gpio)
{
	uint32_t x;
	void *reg = get_gpio_reg(gpio) + GPIO_DIR;
	int bit = get_gpio_bit(gpio);

	x = readl(reg);
	x |= 1ULL << bit;
	writel(x, reg);
}

static void set_direction_in(int gpio)
{
	uint32_t x;
	void *reg = get_gpio_reg(gpio) + GPIO_DIR;
	int bit = get_gpio_bit(gpio);

	x = readl(reg);
	x &= ~(1ULL << bit);
	writel(x, reg);
}

static int read_gpio(int gpio)
{
	void *reg = get_gpio_reg(gpio) + GPIO_DATA;
	int bit = get_gpio_bit(gpio);

	return (readl(reg) >> bit) & 0x1;
}

static void write_gpio(int gpio, int val)
{
	uint32_t x;
	void *reg = get_gpio_reg(gpio) + GPIO_DATA;
	int bit = get_gpio_bit(gpio);

	x = readl(reg);
	if (val)
		x |= 1ULL << bit;
	else
		x &= ~(1ULL << bit);
	writel(x, reg);
}

static inline void clock_cycle(int gpio, int num_clks)
{
        int i;
	volatile int j;

	/* Need to introduce delays when inlining this function */
	for (j = 0; j < CLOCK_DELAY; j++);
        for (i = 0; i < num_clks; i++) {
                write_gpio(gpio, 0);
                write_gpio(gpio, 1);
        }
	for (j = 0; j < CLOCK_DELAY; j++);
}

static uint8_t crc4(uint8_t c, int b)
{
	uint8_t m = 0;

	c &= 0xf;
	m = b ^ ((c >> 3) & 0x1);
	m = (m << 2) | (m << 1) | (m);
	c <<= 1;
	c ^= m;

	return c & 0xf;
}

/* FSI bits should be reading on the falling edge. Read a bit and
 * clock the next one out. */
static inline unsigned int fsi_read_bit(void)
{
	int x;

	x = read_gpio(FSI_DAT);
	clock_cycle(FSI_CLK, 1);

	/* The FSI hardware is active low (ie. inverted) */
	return !(x & 1);
}

static inline void fsi_send_bit(uint64_t bit)
{
	write_gpio(FSI_DAT, !bit);
	clock_cycle(FSI_CLK, 1);
}

/* Format a CFAM address into an FSI slaveId, command and address. */
static uint64_t fsi_abs_ar(uint32_t addr, int read)
{
	uint32_t slave_id = (addr >> 21) & 0x3;

	/* Reformat the address. I'm not sure I fully understand this
	 * yet but we basically shift the bottom byte and add 0b01
	 * (for the write word?) */
       	addr = ((addr & 0x1fff00) | ((addr & 0xff) << 2)) << 1;
	addr |= 0x3;
	addr |= slave_id << 26;
	addr |= (0x8ULL | !!(read)) << 22;

	return addr;
}

static uint64_t fsi_d_poll(uint8_t slave_id)
{
	return slave_id << 3 | 0x2;
}

static void fsi_break(void)
{
	set_direction_out(FSI_CLK);
	set_direction_out(FSI_DAT);
	write_gpio(FSI_DAT_EN, 1);

	/* Crank things - not sure if we need this yet */
	write_gpio(FSI_CLK, 1);
	write_gpio(FSI_DAT, 1); /* Data standby state */

	/* Send break command */
	write_gpio(FSI_DAT, 0);
	clock_cycle(FSI_CLK, 256);
}

/* Send a sequence, including start bit and crc */
static void fsi_send_seq(uint64_t seq, int len)
{
	int i;
	uint8_t crc;

	set_direction_out(FSI_CLK);
	set_direction_out(FSI_DAT);
	write_gpio(FSI_DAT_EN, 1);

	write_gpio(FSI_DAT, 1);
	clock_cycle(FSI_CLK, 50);

	/* Send the start bit */
	write_gpio(FSI_DAT, 0);
	clock_cycle(FSI_CLK, 1);

	/* crc includes start bit */
	crc = crc4(0, 1);

	for (i = 63; i >= 64 - len; i--) {
		crc = crc4(crc, !!(seq & (1ULL << i)));
		fsi_send_bit(seq & (1ULL << i));
	}

	/* Send the CRC */
	for (i = 3; i >= 0; i--)
		fsi_send_bit(crc & (1ULL << i));

	write_gpio(FSI_CLK, 0);
}

/* Read a response. Only supports upto 60 bits at the moment. */
static enum fsi_result fsi_read_resp(uint64_t *result, int len)
{
	int i, x;
	uint8_t crc;
	uint64_t resp = 0;
	uint8_t ack = 0;

	write_gpio(FSI_DAT_EN, 0);
	set_direction_in(FSI_DAT);

	/* Wait for start bit */
	for (i = 0; i < 512; i++) {
		x = fsi_read_bit();
		if (x)
			break;
	}

	if (i == 512) {
		PR_DEBUG("Timeout waiting for start bit\n");
		return FSI_MERR_TIMEOUT;
	}

	crc = crc4(0, 1);

	/* Read the response code (ACK, ERR_A, etc.) */
	for (i = 0; i < 4; i++) {
		ack <<= 1;
		ack |= fsi_read_bit();
		crc = crc4(crc, ack & 0x1);
	}

	/* A non-ACK response has no data but should include a CRC */
	if (ack != FSI_ACK)
		len = 7;

	for (; i < len + CRC_LEN; i++) {
		resp <<= 1;
		resp |= fsi_read_bit();
		crc = crc4(crc, resp & 0x1);
	}

	if (crc != 0) {
		printf("CRC error: 0x%llx\n", resp);
		return FSI_MERR_C;
	}

	write_gpio(FSI_CLK, 0);

	/* Strip the CRC off */
	*result = resp >> 4;

	return ack & 0x3;
}

static enum fsi_result fsi_d_poll_wait(uint8_t slave_id, uint64_t *resp, int len)
{
	int i;
	uint64_t seq;
	enum fsi_result rc;

	/* Poll for response if busy */
	for (i = 0; i < 512; i++) {
		seq = fsi_d_poll(slave_id) << 59;
		fsi_send_seq(seq, 5);

		if ((rc = fsi_read_resp(resp, len)) != FSI_BUSY)
			break;
	}

	return rc;
}

static int fsi_getcfam(struct target *target, uint64_t addr, uint64_t *value)
{
	uint64_t seq;
	uint64_t resp;
	enum fsi_result rc;

	/* This must be the last target in a chain */
	assert(!target->next);

	/* Format of the read sequence is:
	 *  6666555555555544444444443333333333222222222211111111110000000000
	 *  3210987654321098765432109876543210987654321098765432109876543210
	 *
	 *  ii1001aaaaaaaaaaaaaaaaaaa011cccc
	 *
	 * Where:
	 *  ii  = slaveId
	 *  a   = address bit
	 *  011 = write word size
	 *  d   = data bit
	 *  c   = crc bit
	 *
	 * When applying the sequence it should be inverted (active
	 * low)
	 */
	seq = fsi_abs_ar(addr, 1) << 36;
	fsi_send_seq(seq, 28);

	if ((rc = fsi_read_resp(&resp, 36)) == FSI_BUSY)
		rc = fsi_d_poll_wait(0, &resp, 36);

	if (rc != FSI_ACK) {
		PR_DEBUG("getcfam error. Response: 0x%01x\n", rc);
		rc = -1;
	}

	*value = resp & 0xffffffff;

	return rc;
}

static int fsi_putcfam(struct target *target, uint64_t addr, uint64_t data)
{
	uint64_t seq;
	uint64_t resp;
	enum fsi_result rc;

	/* This must be the last target in a chain */
	assert(!target->next);

	/* Format of the sequence is:
	 *  6666555555555544444444443333333333222222222211111111110000000000
	 *  3210987654321098765432109876543210987654321098765432109876543210
	 *
	 *  ii1000aaaaaaaaaaaaaaaaaaa011ddddddddddddddddddddddddddddddddcccc
	 *
	 * Where:
	 *  ii  = slaveId
	 *  a   = address bit
	 *  011 = write word size
	 *  d   = data bit
	 *  c   = crc bit
	 *
	 * When applying the sequence it should be inverted (active
	 * low)
	 */
	seq = fsi_abs_ar(addr, 0) << 36;
	seq |= ((uint64_t) data & 0xffffffff) << (4);

	fsi_send_seq(seq, 60);
	if ((rc = fsi_read_resp(&resp, 4)) == FSI_BUSY)
		rc = fsi_d_poll_wait(0, &resp, 4);

	if (rc != FSI_ACK)
		PR_DEBUG("putcfam error. Response: 0x%01x\n", rc);
	else
		rc = 0;

	return rc;
}

static void fsi_reset(struct target *target)
{
	uint64_t val64, old_base = target->base;

	target->base = 0;

	fsi_break();

	/* Clear own id on the master CFAM to access hMFSI ports */
	fsi_getcfam(target, 0x800, &val64);
	val64 &= ~(PPC_BIT32(6) | PPC_BIT32(7));
	fsi_putcfam(target, 0x800, val64);
	target->base = old_base;
}

static void fsi_destroy(struct target *target)
{
	set_direction_out(FSI_CLK);
	set_direction_out(FSI_DAT);
	write_gpio(FSI_DAT_EN, 1);

	/* Crank things - this is needed to use this tool for kicking off system boot  */
	write_gpio(FSI_CLK, 1);
	write_gpio(FSI_DAT, 1); /* Data standby state */
	clock_cycle(FSI_CLK, 5000);
	write_gpio(FSI_DAT_EN, 0);

	write_gpio(FSI_CLK, 0);
	write_gpio(FSI_ENABLE, 0);
	write_gpio(CRONUS_SEL, 0);  //Set Cronus control to FSP2
}

int fsi_target_init(struct target *target, const char *name, uint64_t base, struct target *next)
{
	if (!mem_fd) {
		mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (mem_fd < 0) {
			perror("Unable to open /dev/mem");
			exit(1);
		}
	}

	if (!gpio_reg) {
		/* We only have to do this init once per backend */
		gpio_reg = mmap(NULL, getpagesize(),
				PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
		if (gpio_reg == MAP_FAILED) {
			perror("Unable to map GPIO register memory");
			exit(1);
		}

		set_direction_out(CRONUS_SEL);
		set_direction_out(FSI_ENABLE);
		set_direction_out(FSI_DAT_EN);

		write_gpio(FSI_ENABLE, 1);
		write_gpio(CRONUS_SEL, 1);  //Set Cronus control to BMC

		fsi_reset(target);
	}

	/* No cascaded devices after this one. */
	assert(next == NULL);
	target_init(target, name, base, fsi_getcfam, fsi_putcfam, fsi_destroy,
		    next);

	return 0;
}

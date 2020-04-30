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
#include <inttypes.h>

#include "bitutils.h"
#include "operations.h"
#include "hwunit.h"
#include "debug.h"

#define GPIO_BASE	0x1e780000
#define GPIO_DATA	0x0
#define GPIO_DIR	0x4

#define CRC_LEN		4

/* Defines a GPIO. The Aspeed devices dont have consistent stride
 * between registers so we need to encode register bit number and base
 * address offset */
struct gpio_pin {
	uint32_t offset;
	uint32_t bit;
};

enum gpio {
	GPIO_FSI_CLK = 0,
	GPIO_FSI_DAT = 1,
	GPIO_FSI_DAT_EN = 2,
	GPIO_FSI_ENABLE = 3,
	GPIO_CRONUS_SEL = 4,
};

/* Pointer to the GPIO pins to use for this system */
#define FSI_CLK		&gpio_pins[GPIO_FSI_CLK]
#define FSI_DAT		&gpio_pins[GPIO_FSI_DAT]
#define FSI_DAT_EN	&gpio_pins[GPIO_FSI_DAT_EN]
#define FSI_ENABLE      &gpio_pins[GPIO_FSI_ENABLE]
#define CRONUS_SEL	&gpio_pins[GPIO_CRONUS_SEL]
static struct gpio_pin gpio_pins[GPIO_CRONUS_SEL + 1];

/* FSI result symbols */
enum fsi_result {
	FSI_MERR_TIMEOUT = -2,
	FSI_MERR_C = -1,
	FSI_ACK = 0x0,
	FSI_BUSY = 0x1,
	FSI_ERR_A = 0x2,
	FSI_ERR_C = 0x3,
};

static uint32_t clock_delay  = 0;

#define FSI_DATA0_REG	0x1000
#define FSI_DATA1_REG	0x1001
#define FSI_CMD_REG	0x1002
#define  FSI_CMD_REG_WRITE PPC_BIT32(0)

#define FSI_RESET_REG	0x1006
#define  FSI_RESET_CMD PPC_BIT32(0)

#define FSI_SET_PIB_RESET_REG 0x1007
#define  FSI_SET_PIB_RESET PPC_BIT32(0)

/* For some reason the FSI2PIB engine dies with frequent
 * access. Letting it have a bit of a rest seems to stop the
 * problem. This sets the number of usecs to sleep between SCOM
 * accesses. */
#define FSI2PIB_RELAX	50

/* FSI private data */
static void *gpio_reg = NULL;
static int mem_fd = 0;

static void fsi_reset(struct fsi *fsi);

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

static int __attribute__((unused)) get_direction(struct gpio_pin *pin)
{
	void *offset = gpio_reg + pin->offset + GPIO_DIR;

	return !!(readl(offset) & (1ULL << pin->bit));
}

static void set_direction_out(struct gpio_pin *pin)
{
	uint32_t x;
	void *offset = gpio_reg + pin->offset + GPIO_DIR;

	x = readl(offset);
	x |= 1ULL << pin->bit;
	writel(x, offset);
}

static void set_direction_in(struct gpio_pin *pin)
{
	uint32_t x;
	void *offset = gpio_reg + pin->offset + GPIO_DIR;

	x = readl(offset);
	x &= ~(1ULL << pin->bit);
	writel(x, offset);
}

static int read_gpio(struct gpio_pin *pin)
{
	void *offset = gpio_reg + pin->offset + GPIO_DATA;

	return (readl(offset) >> pin->bit) & 0x1;
}

static void write_gpio(struct gpio_pin *pin, int val)
{
	uint32_t x;
	void *offset = gpio_reg + pin->offset + GPIO_DATA;

	x = readl(offset);
	if (val)
		x |= 1ULL << pin->bit;
	else
		x &= ~(1ULL << pin->bit);
	writel(x, offset);
}

static inline void clock_cycle(struct gpio_pin *pin, int num_clks)
{
        int i;
	volatile int j;

	/* Need to introduce delays when inlining this function */
	for (j = 0; j < clock_delay; j++)
		;
        for (i = 0; i < num_clks; i++) {
                write_gpio(pin, 0);
                write_gpio(pin, 1);
        }
	for (j = 0; j < clock_delay; j++);
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
       	addr = ((addr & 0x1ffc00) | ((addr & 0x3ff) << 2)) << 1;
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
		PR_ERROR("CRC error: 0x%" PRIx64 "\n", resp);
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

static int fsi_getcfam(struct fsi *fsi, uint32_t addr, uint32_t *value)
{
	uint64_t seq;
	uint64_t resp;
	enum fsi_result rc;

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

static int fsi_putcfam(struct fsi *fsi, uint32_t addr, uint32_t data)
{
	uint64_t seq;
	uint64_t resp;
	enum fsi_result rc;

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

static void fsi_reset(struct fsi *fsi)
{
	uint32_t val;

	fsi_break();

	/* Clear own id on the master CFAM to access hMFSI ports */
	fsi_getcfam(fsi, 0x800, &val);
	val &= ~(PPC_BIT32(6) | PPC_BIT32(7));
	fsi_putcfam(fsi, 0x800, val);
}

void fsi_destroy(struct pdbg_target *target)
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
	write_gpio(CRONUS_SEL, 0);
}

int bmcfsi_probe(struct pdbg_target *target)
{
	struct fsi *fsi = target_to_fsi(target);

	if (!mem_fd) {
		mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (mem_fd < 0) {
			perror("Unable to open /dev/mem");
			exit(1);
		}
	}

	if (!gpio_reg) {
		assert(!(pdbg_target_u32_index(target, "fsi_clk", 0,
			 &gpio_pins[GPIO_FSI_CLK].offset)));
		assert(!(pdbg_target_u32_index(target, "fsi_clk", 1,
			 &gpio_pins[GPIO_FSI_CLK].bit)));
		assert(!(pdbg_target_u32_index(target, "fsi_dat", 0,
			 &gpio_pins[GPIO_FSI_DAT].offset)));
		assert(!(pdbg_target_u32_index(target, "fsi_dat", 1,
			 &gpio_pins[GPIO_FSI_DAT].bit)));
		assert(!(pdbg_target_u32_index(target, "fsi_dat_en", 0,
			 &gpio_pins[GPIO_FSI_DAT_EN].offset)));
		assert(!(pdbg_target_u32_index(target, "fsi_dat_en", 1,
			 &gpio_pins[GPIO_FSI_DAT_EN].bit)));
		assert(!(pdbg_target_u32_index(target, "fsi_enable", 0,
			 &gpio_pins[GPIO_FSI_ENABLE].offset)));
		assert(!(pdbg_target_u32_index(target, "fsi_enable", 1,
			 &gpio_pins[GPIO_FSI_ENABLE].bit)));
		assert(!(pdbg_target_u32_index(target, "cronus_sel", 0,
			 &gpio_pins[GPIO_CRONUS_SEL].offset)));
		assert(!(pdbg_target_u32_index(target, "cronus_sel", 1,
			 &gpio_pins[GPIO_CRONUS_SEL].bit)));
		assert(!(pdbg_target_u32_property(target, "clock_delay",
			 &clock_delay)));

		/* We only have to do this init once per backend */
		gpio_reg = mmap(NULL, getpagesize(),
				PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
		if (gpio_reg == MAP_FAILED) {
			perror("Unable to map GPIO register memory");
			exit(-1);
		}

		set_direction_out(CRONUS_SEL);
		set_direction_out(FSI_ENABLE);
		set_direction_out(FSI_DAT_EN);

		write_gpio(FSI_ENABLE, 1);
		write_gpio(CRONUS_SEL, 1);

		fsi_reset(fsi);
	}

	return 0;
}

static struct fsi bmcfsi = {
	.target = {
		.name = "BMC GPIO bit-banging FSI master",
		.compatible = "ibm,bmcfsi",
		.class = "fsi",
		.probe = bmcfsi_probe,

	},
	.read = fsi_getcfam,
	.write = fsi_putcfam,
};
DECLARE_HW_UNIT(bmcfsi);

__attribute__((constructor))
static void register_bmcfsi(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &bmcfsi_hw_unit);
}

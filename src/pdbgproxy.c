#define _GNU_SOURCE
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <byteswap.h>
#include <ccan/array_size/array_size.h>

#include <bitutils.h>
#include <hwunit.h>

#include "pdbgproxy.h"
#include "main.h"
#include "optcmd.h"
#include "debug.h"
#include "path.h"
#include "sprs.h"

#ifndef DISABLE_GDBSERVER

/*
 * If the client sents qSupported then it can handle unlimited length response.
 * Shouldn't need more than 8k for now. If we had to handle old clients, 512
 * should be a lower limit.
 */
#define MAX_RESP_LEN	8192

/*
 * Maximum packet size. We don't advertise this to the client (TODO).
 */
#define BUFFER_SIZE    	8192

/* GDB packets */
#define STR(e) "e"
#define ACK "+"
#define NACK "-"
#define OK "OK"
#define ERROR(e) "E"STR(e)

#define TEST_SKIBOOT_ADDR 0x40000000

static bool all_stopped = false;
static struct pdbg_target *thread_target = NULL;
static struct pdbg_target *adu_target;
static struct timeval timeout;
static int poll_interval = 100;
static int fd = -1;
enum client_state {IDLE, SIGNAL_WAIT};
static enum client_state state = IDLE;

/* Attached to thread->gdbserver_priv */
struct gdb_thread {
	uint64_t pir;
	bool attn_set;
	bool initial_stopped;
	bool stop_attn;
	bool stop_sstep;
	bool stop_ctrlc;
};

static void destroy_client(int dead_fd);

static uint8_t gdbcrc(char *data)
{
	uint8_t crc = 0;
	int i;

	for (i = 0; i < strlen(data); i++)
		crc += data[i];

	return crc;
}

static void send_response(int fd, char *response)
{
	int len;
	char *result;

	len = asprintf(&result, "$%s#%02x", response, gdbcrc(response));
	PR_INFO("Send: %s\n", result);
	send(fd, result, len, 0);
	free(result);
}

void send_nack(void *priv)
{
	PR_INFO("Send: -\n");
	send(fd, NACK, 1, 0);
}

void send_ack(void *priv)
{
	PR_INFO("Send: +\n");
	send(fd, ACK, 1, 0);
}

static void get_thread(uint64_t *stack, void *priv)
{
	struct thread *thread = target_to_thread(thread_target);
	struct gdb_thread *gdb_thread = thread->gdbserver_priv;
	char data[3+4+2+1]; /* 'QCp' + 4 hex digits + '.0' + NUL */

	snprintf(data, sizeof(data), "QC%04" PRIx64, gdb_thread->pir);

	PR_INFO("get_thread pir=%"PRIx64"\n", gdb_thread->pir);

	send_response(fd, data);
}

static void set_thread(uint64_t *stack, void *priv)
{
	struct pdbg_target *target;
	uint64_t pir = stack[0];

	PR_INFO("set_thread pir=%"PRIx64"\n", pir);

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		gdb_thread = thread->gdbserver_priv;

		if (gdb_thread->pir == pir) {
			thread_target = target;
			send_response(fd, OK);
			return;
		}
	}
	send_response(fd, ERROR(EEXIST));
}

static int threadinfo_iterator;

static void qs_threadinfo(uint64_t *stack, void *priv)
{
	struct pdbg_target *target;
	char data[MAX_RESP_LEN] = "m";
	size_t s = 1;
	int iter = 0;
	bool found = false;

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!found && iter < threadinfo_iterator) {
			iter++;
			continue;
		}

		found = true;

		if (iter == threadinfo_iterator)
			s += snprintf(data + s, sizeof(data) - s,
				      "%04" PRIx64, gdb_thread->pir);
		else
			s += snprintf(data + s, sizeof(data) - s,
				      ",%04" PRIx64, gdb_thread->pir);
		threadinfo_iterator++;

		if (sizeof(data) - s < 9) /* comma, 7 digits, NUL */
			break;

	}

	PR_INFO("qf_threadinfo %s\n", data);

	if (s > 1)
		send_response(fd, data);
	else
		send_response(fd, "l");
}

static void qf_threadinfo(uint64_t *stack, void *priv)
{
	threadinfo_iterator = 0;

	qs_threadinfo(stack, priv);
}

static void send_stop_for_thread(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);
	struct gdb_thread *gdb_thread = thread->gdbserver_priv;
	uint64_t pir = gdb_thread->pir;
	char data[MAX_RESP_LEN];
	size_t s = 0;
	struct thread_regs regs;
	uint64_t value;
	uint32_t value32;
	int sig;
	int i;

	if (gdb_thread->stop_attn || gdb_thread->stop_sstep)
		sig = 5; /* TRAP */
	else if (gdb_thread->stop_ctrlc)
		sig = 2; /* INT */
	else
		sig = 0; /* default / initial stop reason */

	s += snprintf(data + s, sizeof(data) - s, "T%02uthread:%04" PRIx64 ";%s", sig, pir, sig == 5 ? "swbreak:;" : "");

	if (thread_getregs(target, &regs))
		PR_ERROR("Error reading gprs\n");

	for (i = 0; i < 32; i++)
		s += snprintf(data + s, sizeof(data) - s, "%x:%016" PRIx64 ";", i, be64toh(regs.gprs[i]));

	thread_getnia(target, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%016" PRIx64 ";", 0x40, be64toh(value));

	thread_getmsr(target, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%016" PRIx64 ";", 0x41, be64toh(value));

	thread_getcr(target, &value32);
	s += snprintf(data + s, sizeof(data) - s, "%x:%08" PRIx32 ";", 0x42, be32toh(value32));

	thread_getspr(target, SPR_LR, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%016" PRIx64 ";", 0x43, be64toh(value));

	thread_getspr(target, SPR_CTR, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%016" PRIx64 ";", 0x44, be64toh(value));

	thread_getspr(target, SPR_XER, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%08" PRIx32 ";", 0x45, be32toh(value));

	thread_getspr(target, SPR_FPSCR, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%08" PRIx32 ";", 0x46, be32toh(value));

	thread_getspr(target, SPR_VSCR, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%08" PRIx32 ";", 0x67, be32toh(value));

	thread_getspr(target, SPR_VRSAVE, &value);
	s += snprintf(data + s, sizeof(data) - s, "%x:%08" PRIx32 ";", 0x68, be32toh(value));

	send_response(fd, data);
}

static void stop_reason(uint64_t *stack, void *priv)
{
	PR_INFO("stop_reason\n");

	send_stop_for_thread(thread_target);
}

static void start_all(void);

static void detach(uint64_t *stack, void *priv)
{
	PR_INFO("Detach debug session with client. pid %16" PRIi64 "\n", stack[0]);
	start_all();

	send_response(fd, OK);
}

#define POWER8_HID_ENABLE_ATTN			PPC_BIT(31)

#define POWER9_HID_ENABLE_ATTN			PPC_BIT(3)
#define POWER9_HID_FLUSH_ICACHE			PPC_BIT(2)

#define POWER10_HID_ENABLE_ATTN			PPC_BIT(3)
#define POWER10_HID_FLUSH_ICACHE		PPC_BIT(2)

static int thread_set_attn(struct pdbg_target *target, bool enable)
{
	struct thread *thread = target_to_thread(target);
	struct gdb_thread *gdb_thread = thread->gdbserver_priv;
	uint64_t hid;

	if (!enable && !gdb_thread->attn_set) {
		/* Don't clear attn if we didn't enable it */
		return 0;
	}

	if (thread_getspr(target, SPR_HID, &hid))
		return -1;

	if (pdbg_target_compatible(target, "ibm,power8-thread")) {
		if (enable) {
			if (hid & POWER8_HID_ENABLE_ATTN)
				return 0;
			hid |= POWER8_HID_ENABLE_ATTN;
			gdb_thread->attn_set = true;
		} else {
			if (!(hid & POWER8_HID_ENABLE_ATTN))
				return 0;
			hid &= ~POWER8_HID_ENABLE_ATTN;
		}
		if (thread_putspr(target, SPR_HID, hid))
			return -1;

	} else if (pdbg_target_compatible(target, "ibm,power9-thread")) {
		if (enable) {
			if (hid & POWER9_HID_ENABLE_ATTN)
				return 0;
			hid |= POWER9_HID_ENABLE_ATTN;
			gdb_thread->attn_set = true;
		} else {
			if (!(hid & POWER9_HID_ENABLE_ATTN))
				return 0;
			hid &= ~POWER9_HID_ENABLE_ATTN;
		}

		if (hid & POWER9_HID_FLUSH_ICACHE) {
			PR_WARNING("set_attn found HID flush icache bit unexpectedly set\n");
			hid &= ~POWER9_HID_FLUSH_ICACHE;
		}

		/* Change the ENABLE_ATTN bit in the register */
		if (thread_putspr(target, SPR_HID, hid))
			return -1;
		/* FLUSH_ICACHE 0->1 to flush */
		if (thread_putspr(target, SPR_HID, hid | POWER9_HID_FLUSH_ICACHE))
			return -1;
		/* Restore FLUSH_ICACHE back to 0 */
		if (thread_putspr(target, SPR_HID, hid))
			return -1;

	} else if (pdbg_target_compatible(target, "ibm,power10-thread")) {
		if (enable) {
			if (hid & POWER10_HID_ENABLE_ATTN)
				return 0;
			hid |= POWER10_HID_ENABLE_ATTN;
			gdb_thread->attn_set = true;
		} else {
			if (!(hid & POWER10_HID_ENABLE_ATTN))
				return 0;
			hid &= ~POWER10_HID_ENABLE_ATTN;
		}
		if (hid & POWER10_HID_FLUSH_ICACHE) {
			PR_WARNING("set_attn found HID flush icache bit unexpectedly set\n");
			hid &= ~POWER10_HID_FLUSH_ICACHE;
		}

		/* Change the ENABLE_ATTN bit in the register */
		if (thread_putspr(target, SPR_HID, hid))
			return -1;
		/* FLUSH_ICACHE 0->1 to flush */
		if (thread_putspr(target, SPR_HID, hid | POWER10_HID_FLUSH_ICACHE))
			return -1;
		/* Restore FLUSH_ICACHE back to 0 */
		if (thread_putspr(target, SPR_HID, hid))
			return -1;
	} else {
		return -1;
	}

	return 0;
}

static int set_attn(bool enable)
{
	struct pdbg_target *target;
	int err = 0;

	for_each_path_target_class("thread", target) {
		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		err = thread_set_attn(target, enable);
		if (err)
			break;
	}

	return err;
}

/* 32 registers represented as 16 char hex numbers with null-termination */
#define REG_DATA_SIZE (32*16+1)
static void get_gprs(uint64_t *stack, void *priv)
{
	char data[REG_DATA_SIZE] = "";
	struct thread_regs regs;
	int i;

	if(thread_getregs(thread_target, &regs))
		PR_ERROR("Error reading gprs\n");

	for (i = 0; i < 32; i++) {
		PR_INFO("r%d = 0x%016" PRIx64 "\n", i, regs.gprs[i]);
		snprintf(data + i*16, 17, "%016" PRIx64 , be64toh(regs.gprs[i]));
	}

	send_response(fd, data);
}

static void get_spr(uint64_t *stack, void *priv)
{
	char data[REG_DATA_SIZE];
	uint64_t value;
	uint32_t value32;

	switch (stack[0]) {
	case 0x40:
		if (thread_getnia(thread_target, &value))
			PR_ERROR("Error reading NIA\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	case 0x41:
		if (thread_getmsr(thread_target, &value))
			PR_ERROR("Error reading MSR\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	case 0x42:
		if (thread_getcr(thread_target, &value32))
			PR_ERROR("Error reading CR \n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh((uint64_t)value32));
		send_response(fd, data);
		break;

	case 0x43:
		if (thread_getspr(thread_target, SPR_LR, &value))
			PR_ERROR("Error reading LR\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	case 0x44:
		if (thread_getspr(thread_target, SPR_CTR, &value))
			PR_ERROR("Error reading CTR\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	case 0x45:
		/*
		 * Not all XER register bits may be recoverable with RAM
		 * mode accesses, so this may be not entirely accurate.
		 */
		if (thread_getspr(thread_target, SPR_XER, &value))
			PR_ERROR("Error reading XER\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	case 0x46:
		if (thread_getspr(thread_target, SPR_FPSCR, &value))
			PR_ERROR("Error reading FPSCR\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;
	case 0x67:
		if (thread_getspr(thread_target, SPR_VSCR, &value))
			PR_ERROR("Error reading VSCR\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;
	case 0x68:
		if (thread_getspr(thread_target, SPR_VRSAVE, &value))
			PR_ERROR("Error reading VRSAVE\n");
		snprintf(data, REG_DATA_SIZE, "%016" PRIx64 , be64toh(value));
		send_response(fd, data);
		break;

	default:
		send_response(fd, "xxxxxxxxxxxxxxxx");
		break;
	}
}

#define MAX_DATA 0x1000

/* Returns a real address to use with mem_read or -1UL if we
 * couldn't determine a real address. At the moment we only deal with
 * kernel linear mapping but in future we could walk that page
 * tables. */
static uint64_t get_real_addr(uint64_t addr)
{
	if (GETFIELD(PPC_BITMASK(0, 3), addr) == 0xc)
		/* Assume all 0xc... addresses are part of the linux linear map */
		addr &= ~PPC_BITMASK(0, 1);
	else if (addr < TEST_SKIBOOT_ADDR)
		return addr;
	else
		addr = -1UL;

	return addr;
}

/*
 * TODO: Move write_memory and read_memory into libpdbg helper and advertise
 * alignment requirement of a target, or have the back-ends use it directly
 * (latter may have the problem that implicit R-M-W is undesirable at very low
 * level operations if we only wanted to write).
 */

/* WARNING: _a *MUST* be a power of two */
#define ALIGN_UP(_v, _a)	(((_v) + (_a) - 1) & ~((_a) - 1))
#define ALIGN_DOWN(_v, _a)	((_v) & ~((_a) - 1))

static int write_memory(uint64_t addr, uint64_t len, void *buf, size_t align)
{
	uint64_t start_addr, end_addr;
	void *tmpbuf = NULL;
	void *src;
	int err = 0;

	start_addr = ALIGN_DOWN(addr, align);
	end_addr = ALIGN_UP(addr + len, align);

	if (addr != start_addr || (addr + len) != end_addr) {
		tmpbuf = malloc(end_addr - start_addr);
		if (mem_read(adu_target, start_addr, tmpbuf, end_addr - start_addr, 0, false)) {
			PR_ERROR("Unable to read memory for RMW\n");
			err = -1;
			goto out;
		}
		memcpy(tmpbuf + (addr - start_addr), buf, len);
		src = tmpbuf;
	} else {
		src = buf;
	}

	if (mem_write(adu_target, start_addr, src, end_addr - start_addr, 0, false)) {
		PR_ERROR("Unable to write memory\n");
		err = -1;
	}

out:
	if (tmpbuf)
		free(tmpbuf);

	return err;
}

static int read_memory(uint64_t addr, uint64_t len, void *buf, size_t align)
{
	uint64_t start_addr, end_addr;
	void *tmpbuf = NULL;
	void *dst;
	int err = 0;

	start_addr = ALIGN_DOWN(addr, align);
	end_addr = ALIGN_UP(addr + len, align);

	if (addr != start_addr || (addr + len) != end_addr) {
		tmpbuf = malloc(end_addr - start_addr);
		dst = tmpbuf;
	} else {
		dst = buf;
	}

	if (mem_read(adu_target, start_addr, dst, end_addr - start_addr, 0, false)) {
		PR_ERROR("Unable to read memory\n");
		err = -1;
		goto out;
	}

	if (addr != start_addr || (addr + len) != end_addr)
		memcpy(buf, tmpbuf + (addr - start_addr), len);

out:
	if (tmpbuf)
		free(tmpbuf);

	return err;
}


static void get_mem(uint64_t *stack, void *priv)
{
	uint64_t addr, len, linear_map;
	int i, err = 0;
	uint64_t data[MAX_DATA/sizeof(uint64_t)];
	char result[2*MAX_DATA];

	/* stack[0] is the address and stack[1] is the length */
	addr = stack[0];
	len = stack[1];

	if (len > MAX_DATA) {
		PR_INFO("Too much memory requested, truncating\n");
		len = MAX_DATA;
	}

	if (!addr) {
		err = 2;
		goto out;
	}

	linear_map = get_real_addr(addr);
	if (linear_map != -1UL) {
		if (read_memory(linear_map, len, data, 1)) {
			PR_ERROR("Unable to read memory\n");
			err = 1;
		}
	} else {
		/* Virtual address */
		for (i = 0; i < len; i += sizeof(uint64_t)) {
			if (thread_getmem(thread_target, addr, &data[i/sizeof(uint64_t)])) {
				PR_ERROR("Fault reading memory\n");
				err = 2;
				break;
			}
		}
	}

out:
	if (!err)
		for (i = 0; i < len; i ++) {
			sprintf(&result[i*2], "%02x", *(((uint8_t *) data) + i));
		}
	else
		sprintf(result, "E%02x", err);

	send_response(fd, result);
}

static void put_mem(uint64_t *stack, void *priv)
{
	uint64_t addr, len;
	uint8_t *data;
	int err = 0;

	addr = stack[0];
	len = stack[1];
	data = (uint8_t *)(unsigned long)stack[2];

	addr = get_real_addr(addr);
	if (addr == -1UL) {
		PR_ERROR("TODO: No virtual address support for putmem\n");
		err = 1;
		goto out;
	}

	if (write_memory(addr, len, data, 8)) {
		PR_ERROR("Unable to write memory\n");
		err = 3;
	}

out:
	free(data); // allocated by gdb_parser.rl

	if (err)
		send_response(fd, ERROR(EPERM));
	else
		send_response(fd, OK);
}

#define MAX_BPS 64
static uint64_t bps[MAX_BPS];
static uint32_t saved_insn[MAX_BPS];
static int nr_bps = 0;

static int set_bp(uint64_t addr)
{
	int i;

	if (nr_bps == MAX_BPS)
		return -1;

	if (addr == -1ULL)
		return -1;

	for (i = 0; i < MAX_BPS; i++) {
		if (bps[i] == -1ULL) {
			bps[i] = addr;
			nr_bps++;
			return 0;
		}
	}

	return -1;
}

static int clear_bp(uint64_t addr)
{
	int i;

	if (nr_bps == 0)
		return -1;

	if (addr == -1ULL)
		return -1;

	for (i = 0; i < MAX_BPS; i++) {
		if (bps[i] == addr) {
			bps[i] = -1ULL;
			nr_bps--;
			return 0;
		}
	}

	return -1;
}

static int get_insn(uint64_t addr, uint32_t *insn)
{
	uint64_t linear_map;

	linear_map = get_real_addr(addr);
	if (linear_map != -1UL) {
		if (read_memory(linear_map, 4, insn, 1)) {
			PR_ERROR("Unable to read memory\n");
			return -1;
		}
	} else {
		/* Virtual address */
		return -1;
	}

	return 0;
}

static int put_insn(uint64_t addr, uint32_t insn)
{
	uint64_t linear_map;

	linear_map = get_real_addr(addr);
	if (linear_map != -1UL) {
		if (write_memory(linear_map, 4, &insn, 8)) {
			PR_ERROR("Unable to write memory\n");
			return -1;
		}
	} else {
		/* Virtual address */
		return -1;
	}

	return 0;
}

static void init_breakpoints(void)
{
	int i;

	for (i = 0; i < MAX_BPS; i++)
		bps[i] = -1ULL;
}

static bool match_breakpoint(uint64_t addr)
{
	int i;

	if (nr_bps == 0)
		return false;

	for (i = 0; i < MAX_BPS; i++) {
		if (bps[i] == addr)
			return true;
	}

	return false;
}

static int install_breakpoints(void)
{
	uint8_t attn_opcode[] = {0x00, 0x00, 0x02, 0x00};
	uint32_t *attn = (uint32_t *)&attn_opcode[0];
	uint64_t msr;
	int i;

	if (!nr_bps)
		return 0;

	/* Check endianess in MSR */
	if (thread_getmsr(thread_target, &msr)) {
		PR_ERROR("Couldn't read the MSR to set breakpoint endian");
		return -1;
	}

	if (msr & 1) /* currently little endian */
		*attn = bswap_32(*attn);

	for (i = 0; i < MAX_BPS; i++) {
		if (bps[i] == -1ULL)
			continue;
		if (get_insn(bps[i], &saved_insn[i]))
			continue; /* XXX: handle properly */
		if (put_insn(bps[i], *attn))
			continue; /* XXX: handle properly */
	}

	set_attn(true);

	return 0;
}

static int uninstall_breakpoints(void)
{
	int i;

	if (!nr_bps)
		return 0;

	for (i = 0; i < MAX_BPS; i++) {
		if (bps[i] == -1ULL)
			continue;
		if (put_insn(bps[i], saved_insn[i]))
			continue; /* XXX: handle properly */

	}

	set_attn(false);

	return 0;
}


static void set_break(uint64_t *stack, void *priv)
{
	uint64_t addr;

	/* stack[0] is the address */
	addr = stack[0];

	if (!set_bp(addr))
		send_response(fd, OK);
	else
		send_response(fd, ERROR(ENOMEM));
}

static void clear_break(uint64_t *stack, void *priv)
{
	uint64_t addr;

	/* stack[0] is the address */
	addr = stack[0];

	if (!clear_bp(addr))
		send_response(fd, OK);
	else
		send_response(fd, ERROR(ENOENT));
}

static void v_conts(uint64_t *stack, void *priv)
{
	struct thread *thread = target_to_thread(thread_target);
	struct gdb_thread *gdb_thread = thread->gdbserver_priv;

	PR_INFO("thread_step\n");

	thread_step(thread_target, 1);

	gdb_thread->stop_attn = false;
	gdb_thread->stop_sstep = true;
	gdb_thread->stop_ctrlc = false;

	send_stop_for_thread(thread_target);
}

static void __start_all(void)
{
	struct pdbg_target *target;

	if (!all_stopped)
		PR_ERROR("starting while not all stopped\n");

	if (path_target_all_selected("thread", NULL)) {
		if (thread_start_all()) {
			PR_ERROR("Could not start threads\n");
		}
	} else {
		for_each_path_target_class("thread", target) {
			if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
				continue;

			if (thread_start(target)) {
				PR_ERROR("Could not start thread %s\n",
					 pdbg_target_path(target));
			}
		}
	}

	all_stopped = false;
}

static void start_all(void)
{
	struct pdbg_target *target;

	install_breakpoints();

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		gdb_thread = thread->gdbserver_priv;

		gdb_thread->stop_attn = false;
		gdb_thread->stop_sstep = false;
		gdb_thread->stop_ctrlc = false;
	}

	__start_all();
}

#define VCONT_POLL_DELAY 100000
static void v_contc(uint64_t *stack, void *priv)
{
	start_all();

	state = SIGNAL_WAIT;
	poll_interval = 1;
}

#define P9_SPATTN_AND	0x20010A98
#define P9_SPATTN	0x20010A99

#define P10_SPATTN_AND	0x20028498
#define P10_SPATTN	0x20028499

static bool thread_check_attn(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);
	struct pdbg_target *core;
	uint64_t spattn;

	if (pdbg_target_compatible(target, "ibm,power8-thread")) {
		return true; /* XXX */
	} else if (pdbg_target_compatible(target, "ibm,power9-thread")) {
		core = pdbg_target_require_parent("core", target);
		if (pib_read(core, P9_SPATTN, &spattn)) {
			PR_ERROR("SPATTN read failed\n");
			return false;
		}

		if (spattn & PPC_BIT(1 + 4*thread->id)) {
			uint64_t mask = ~PPC_BIT(1 + 4*thread->id);

			if (pib_write(core, P9_SPATTN_AND, mask)) {
				PR_ERROR("SPATTN clear failed\n");
				return false;
			}

			return true;
		}
	} else if (pdbg_target_compatible(target, "ibm,power10-thread")) {
		core = pdbg_target_require_parent("core", target);
		if (pib_read(core, P10_SPATTN, &spattn)) {
			PR_ERROR("SPATTN read failed\n");
			return false;
		}

		if (spattn & PPC_BIT(1 + 4*thread->id)) {
			uint64_t mask = ~PPC_BIT(1 + 4*thread->id);

			if (pib_write(core, P10_SPATTN_AND, mask)) {
				PR_ERROR("SPATTN clear failed\n");
				return false;
			}

			return true;
		}
	}

	return false;
}

static void __stop_all(void)
{
	struct pdbg_target *target;

	if (all_stopped)
		PR_ERROR("stopping while all stopped\n");

	if (path_target_all_selected("thread", NULL)) {
		if (thread_stop_all()) {
			PR_ERROR("Could not stop threads\n");
		}
	} else {
		for_each_path_target_class("thread", target) {
			if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
				continue;

			if (thread_stop(target)) {
				PR_ERROR("Could not stop thread %s\n",
					 pdbg_target_path(target));
				/* How to fix? */
			}
		}
	}

	all_stopped = true;
}

static void stop_all(void)
{
	struct pdbg_target *target;

	__stop_all();

	for_each_path_target_class("thread", target) {
		struct thread_state status;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		target->probe(target);
		status = thread_status(target);
		if (!status.quiesced) {
			PR_ERROR("Could not quiesce thread\n");
			/* How to fix? */
		}

		if (thread_check_attn(target)) {
			struct thread *thread = target_to_thread(target);
			struct gdb_thread *gdb_thread = thread->gdbserver_priv;
			uint64_t nia;

			PR_INFO("thread pir=%"PRIx64" hit attn\n", gdb_thread->pir);

			gdb_thread->stop_attn = true;

			if (!(status.active))
				PR_ERROR("Error thread inactive after attn\n");

			if (thread_getnia(target, &nia))
				PR_ERROR("Error during getnia\n");

			/*
			 * If we hit a non-breakpoint attn we still want to
			 * switch to that thread, but we don't rewind the nip
			 * so as to advance over the attn.
			 */
			if (match_breakpoint(nia - 4)) {
				PR_INFO("thread pir=%"PRIx64" attn is breakpoint\n", gdb_thread->pir);
				/* Restore NIA to breakpoint address */
				if (thread_putnia(target, nia - 4))
					PR_ERROR("Error during putnia\n");
			}
		}
	}

	uninstall_breakpoints();
}

static void interrupt(uint64_t *stack, void *priv)
{
	PR_INFO("Interrupt from gdb client\n");
	if (state != IDLE) {
		struct thread *thread = target_to_thread(thread_target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		stop_all();
		if (!gdb_thread->stop_attn)
			gdb_thread->stop_ctrlc = true;

		state = IDLE;
		poll_interval = VCONT_POLL_DELAY;
	}

	send_stop_for_thread(thread_target);
}

static bool poll_threads(void)
{
	struct pdbg_target *target;

	for_each_path_target_class("thread", target) {
		struct thread_state status;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		target->probe(target);
		status = thread_status(target);
		if (status.quiesced)
			return true;
	}
	return false;
}

static void poll(void)
{
	struct pdbg_target *target;

	if (state != SIGNAL_WAIT)
		return;

	if (!poll_threads())
		return;

	/* Something hit a breakpoint */

	stop_all();

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		gdb_thread = thread->gdbserver_priv;

		if (gdb_thread->stop_attn) {
			thread_target = target;
			break;
		}
	}

	state = IDLE;
	poll_interval = VCONT_POLL_DELAY;

	send_stop_for_thread(thread_target);
}

static void cmd_default(uint64_t *stack, void *priv)
{
	uintptr_t tmp = stack[0];
	if (stack[0]) {
		send_response(fd, (char *) tmp);
	} else
		send_response(fd, "");
}

static void create_client(int new_fd)
{
	PR_INFO("Client connected\n");
	fd = new_fd;
	if (!all_stopped)
		stop_all();
}

static void destroy_client(int dead_fd)
{
	PR_INFO("Client disconnected\n");
	close(dead_fd);
	fd = -1;
}

static int read_from_client(int fd)
{
	char buffer[BUFFER_SIZE];
	int nbytes;

	nbytes = read(fd, buffer, sizeof(buffer) - 1);
	if (nbytes < 0) {
		perror(__FUNCTION__);
		return -1;
	} else if (nbytes == 0) {
		PR_INFO("0 bytes\n");
		return -1;
	} else {
		int i;

		buffer[nbytes] = '\0';
		PR_INFO("Recv: %s\n", buffer);
		pdbg_log(PDBG_DEBUG, " hex: ");
		for (i = 0; i < nbytes; i++)
			pdbg_log(PDBG_DEBUG, "%02x ", buffer[i]);
		pdbg_log(PDBG_DEBUG, "\n");

		parse_buffer(buffer, nbytes, &fd);
	}

	return 0;
}

static command_cb callbacks[LAST_CMD + 1] = {
	cmd_default,
	get_gprs,
	get_spr,
	stop_reason,
	get_thread,
	set_thread,
	v_contc,
	v_conts,
	qf_threadinfo,
	qs_threadinfo,
	get_mem,
	put_mem,
	set_break,
	clear_break,
	interrupt,
	detach,
	NULL};

static volatile bool gdbserver_running = true;

static void SIGINT_handler(int sig)
{
	gdbserver_running = false;
}

static int gdbserver_start(struct pdbg_target *adu, uint16_t port)
{
	int sock, i;
	struct sigaction sa;
	struct sockaddr_in name;
	fd_set active_fd_set, read_fd_set;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIGINT_handler;
	sa.sa_flags = SA_RESETHAND | SA_NODEFER;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}

	parser_init(callbacks);
	adu_target = adu;

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror(__FUNCTION__);
		return -1;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		perror(__FUNCTION__);
		return -1;
	}

	if (listen(sock, 1) < 0) {
		perror(__FUNCTION__);
		return -1;
	}

	printf("gdbserver: listening on port %d\n", port);

	FD_ZERO(&active_fd_set);
	FD_SET(sock, &active_fd_set);

	while (gdbserver_running) {
		read_fd_set = active_fd_set;
		timeout.tv_sec = 0;
		timeout.tv_usec = poll_interval;
		if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout) < 0) {
			perror(__FUNCTION__);
			return -1;
		}

		for (i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &read_fd_set)) {
				if (i == sock) {
					char host[NI_MAXHOST];
					struct sockaddr saddr;
					socklen_t slen;
					int new;

					new = accept(sock, &saddr, &slen);
					if (new < 0) {
						perror(__FUNCTION__);
						return -1;
					}

					if (getnameinfo(&saddr, slen,
							host, sizeof(host),
							NULL, 0,
							NI_NUMERICHOST) == 0) {
						printf("gdbserver: connection from gdb client %s\n", host);
					}

					if (fd > 0) {
						/* It only makes sense to accept a single client */
						printf("gdbserver: another client already connected\n");
						close(new);
					} else {
						create_client(new);
						FD_SET(new, &active_fd_set);
					}
				} else {
					if (read_from_client(i) < 0) {
						destroy_client(i);
						FD_CLR(i, &active_fd_set);
						printf("gdbserver: ended connection with gdb client\n");
					}
				}
			}
		}

		poll();
	}

	printf("gdbserver: got ctrl-C, cleaning up (second ctrl-C to kill immediately).\n");

	return 1;
}

static int gdbserver(uint16_t port)
{
	struct pdbg_target *target, *adu;
	struct pdbg_target *first_target = NULL;
	struct pdbg_target *first_stopped_target = NULL;
	struct pdbg_target *first_attn_target = NULL;

	init_breakpoints();

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread;
		struct thread_state status;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!pdbg_target_compatible(target, "ibm,power8-thread") &&
		    !pdbg_target_compatible(target, "ibm,power9-thread") &&
		    !pdbg_target_compatible(target, "ibm,power10-thread")) {
			PR_ERROR("GDBSERVER is only available on POWER8,9,10\n");
			return -1;
		}

		gdb_thread = malloc(sizeof(struct gdb_thread));
		memset(gdb_thread, 0, sizeof(*gdb_thread));
		thread->gdbserver_priv = gdb_thread;

		if (!first_target)
			first_target = target;

		status = thread_status(target);
		if (status.quiesced) {
			if (!first_stopped_target)
				first_stopped_target = target;
			gdb_thread->initial_stopped = true;
		}
	}

	if (!first_target) {
		fprintf(stderr, "No thread selected\n");
		return 0;
	}

	if (pdbg_target_compatible(first_target, "ibm,power9-thread")) {
		/*
		 * XXX: If we advertise no swbreak support on POWER9 does
		 * that prevent the client using them?
		 */
		PR_WARNING("Breakpoints may cause host crashes on POWER9 and should not be used\n");
	}

	if (!path_target_all_selected("thread", NULL)) {
		PR_WARNING("GDBSERVER works best when targeting all threads (-a)\n");
	}

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!gdb_thread->initial_stopped) {
			if (thread_stop(target)) {
				PR_ERROR("Could not stop thread %s\n",
					 pdbg_target_path(target));
				return -1;
			}
		}
	}

	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (thread_getspr(target, SPR_PIR, &gdb_thread->pir)) {
			PR_ERROR("Error reading PIR. Are all thread in the target cores quiesced?\n");
			goto out;
		}

		if (gdb_thread->pir & ~0xffffULL) {
			/* This limit is just due to some string array sizes */
			PR_ERROR("PIR exceeds 16-bits.");
			goto out;
		}

		if (thread_check_attn(target)) {
			PR_INFO("thread pir=%"PRIx64" hit attn\n", gdb_thread->pir);

			if (!first_attn_target)
				first_attn_target = target;
			gdb_thread->stop_attn = true;
			if (!gdb_thread->initial_stopped) {
				PR_WARNING("thread pir=%"PRIx64" hit attn but was not stopped\n", gdb_thread->pir);
				gdb_thread->initial_stopped = true;
			}
		}
	}

	/* Target attn as a priority, then any stopped, then first */
	if (first_attn_target)
		thread_target = first_target;
	else if (first_stopped_target)
		thread_target = first_stopped_target;
	else
		thread_target = first_target;

	/*
	 * Resume threads now, except those that were initially stopped,
	 * leave them so the client can inspect them.
	 */
	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!gdb_thread->initial_stopped) {
			if (thread_start(target)) {
				PR_ERROR("Could not start thread %s\n",
					 pdbg_target_path(target));
			}
		}
	}

	/* Select ADU target */
	pdbg_for_each_class_target("mem", adu) {
		if (pdbg_target_probe(adu) == PDBG_TARGET_ENABLED)
			break;
	}

	if (!adu) {
		fprintf(stderr, "No ADU found\n");
		goto out;
	}

	gdbserver_start(adu, port);

out:
	if (!all_stopped)
		stop_all();

	/*
	 * Only resume those which were not initially stopped
	 */
	for_each_path_target_class("thread", target) {
		struct thread *thread = target_to_thread(target);
		struct gdb_thread *gdb_thread = thread->gdbserver_priv;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!gdb_thread->initial_stopped) {
			if (thread_start(target)) {
				PR_ERROR("Could not start thread %s\n",
					 pdbg_target_path(target));
			}
		}
	}

	return 0;
}
#else

static int gdbserver(uint16_t port)
{
	return 0;
}
#endif
OPTCMD_DEFINE_CMD_WITH_ARGS(gdbserver, gdbserver, (DATA16));

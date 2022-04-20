#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"

%%{
	machine gdb;

	action reset {
		cmd = 0;
		rsp = NULL;
		data = stack;
		memset(stack, 0, sizeof(stack));
		crc = 0;
		PR_INFO("RAGEL: CRC reset\n");
	}

	action crc {
		crc += *p;
	}

	action push {
		data++;
		assert(data < &stack[10]);
	}

	action hex_digit {
		*data *= 16;

		if (*p >= '0' && *p <= '9')
			*data += *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			*data += *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			*data += *p - 'A' + 10;
	}

	action mem_hex_digit {
		uint8_t *d;
		uint64_t idx = mem_data_i / 2; /* 2 chars per byte */

		assert(idx < mem_data_length);
		d = &mem_data[idx];

		*d <<= 4;
		if (*p >= '0' && *p <= '9')
			*d += *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			*d += *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			*d += *p - 'A' + 10;

		mem_data_i++;
	}

	action end {
		/* *data should point to the CRC */
		if (crc != *data) {
			printf("CRC error cmd %d\n", cmd);
			if (ack_mode)
				send_nack(priv);
		} else {
			PR_INFO("Cmd %d\n", cmd);
			if (ack_mode)
				send_ack(priv);

			/* Push the response onto the stack */
			if (rsp)
				*data = (uintptr_t)rsp;
			else
				*data = 0;

			if (command_callbacks)
				command_callbacks[cmd](stack, priv);
		}
	}

	get_mem = ('m' @{cmd = GET_MEM;}
		   xdigit+ $hex_digit %push
		   ','
		   xdigit+ $hex_digit %push);

	put_mem = ('M' @{cmd = PUT_MEM;}
		   xdigit+ $hex_digit %push
		   ','
		   xdigit+ $hex_digit %push
		   ':'
		   @{	mem_data_length = *(data - 1);
			mem_data = calloc(1, mem_data_length); // handler frees
			*data = (unsigned long)mem_data;
			data++;
			assert(data < &stack[10]);
			mem_data_i = 0;
		   }
		   xdigit+ $mem_hex_digit);

	get_gprs = ('g' @{cmd = GET_GPRS;});

	get_spr = ('p' @{cmd = GET_SPR;}
		   xdigit+ $hex_digit %push);

	stop_reason = ('?' @{cmd = STOP_REASON;});

	detach = ('D' @{cmd = DETACH;}
		     xdigit+ $hex_digit %push);

	# TODO: We don't actually listen to what's supported
	q_attached = ('qAttached:' xdigit* @{rsp = "1";});
	q_C = ('qC' @{rsp = "QC1";});
	q_supported = ('qSupported:' any* >{rsp = "multiprocess+;vContSupported+;QStartNoAckMode+"; ack_mode = true;});
	qf_threadinfo = ('qfThreadInfo' @{rsp = "m1l";});
	q_start_noack = ('QStartNoAckMode' @{rsp = "OK"; send_ack(priv); ack_mode = false;});

	# thread info
	get_thread = ('qC' @{cmd = GET_THREAD;});
	set_thread = ('Hg' ('p' xdigit+ '.')+ xdigit+ $hex_digit %push @{cmd = SET_THREAD;});

	# vCont packet parsing
	v_contq = ('vCont?' @{rsp = "vCont;c;C;s;S";});
	v_contc = ('vCont;c' any* @{cmd = V_CONTC;});
	v_conts = ('vCont;s' any* @{cmd = V_CONTS;});
	unknown = (any*);

	interrupt = (3 @{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");});

	commands = (get_mem | get_gprs | get_spr | stop_reason | get_thread | set_thread |
		    q_attached | q_C | q_supported | qf_threadinfo | q_C |
		    q_start_noack | v_contq | v_contc | v_conts | put_mem |
		    detach | unknown );

	cmd = (('$' ((commands & ^'#'*) >reset $crc)
	      ('#' xdigit{2} $hex_digit @end)) >{PR_INFO("RAGEL:cmd\n");});

	# We ignore ACK/NACK for the moment
	ack = ('+' >{PR_INFO("RAGEL:ack\n");});
	nack = ('-' >{PR_INFO("RAGEL:nack\n");});

	main := (cmd | interrupt | ack | nack)*;

}%%

static enum gdb_command cmd = NONE;
static uint64_t stack[10], *data = stack;
static uint8_t *mem_data;
static uint64_t mem_data_length;
static uint64_t mem_data_i;
static char *rsp;
static uint8_t crc;
static int cs;

static command_cb *command_callbacks;

static bool ack_mode = true;

%%write data;

void parser_init(command_cb *callbacks)
{
	%%write init;

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	%%write exec;

	if (cs == gdb_error) {
		printf("parse error\n");
		return -1;
	}

	return 0;
}

#if 0
void send_nack(void *priv)
{
	printf("Send: -\n");
}

void send_ack(void *priv)
{
	printf("Send: +\n");
}

int main(int argc, char **argv)
{
	parser_init(NULL);

	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++)
			parse_buffer(argv[i], strlen(argv[i]), NULL);
	}

	return 0;
}
#endif

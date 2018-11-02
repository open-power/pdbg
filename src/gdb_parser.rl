#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"

%%{
	machine gdb;

	action reset {
		cmd = 0;
		rsp = NULL;
		data = stack;
		memset(stack, 0, sizeof(stack));
		crc = 0;
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

	action end {
		/* *data should point to the CRC */
		if (crc != *data) {
			printf("CRC error\n");
			send_nack(priv);
		} else {
			printf("Cmd %d\n", cmd);
			send_ack(priv);

			/* Push the response onto the stack */
			if (rsp)
				*data = (uintptr_t)rsp;
			else
				*data = 0;

			command_callbacks[cmd](stack, priv);
		}
	}

	get_mem = ('m' @{cmd = GET_MEM;}
		   xdigit+ $hex_digit %push
		   ','
		   xdigit+ $hex_digit %push);

	put_mem = ('M' any* @{cmd = PUT_MEM;}
		   xdigit+ $hex_digit %push
		   ','
		   xdigit+ $hex_digit %push
		   ':'
		   xdigit+ $hex_digit %push);

	get_gprs = ('g' @{cmd = GET_GPRS;});

	get_spr = ('p' @{cmd = GET_SPR;}
		   xdigit+ $hex_digit %push);

	stop_reason = ('?' @{cmd = STOP_REASON;});

	set_thread = ('H' any* @{cmd = SET_THREAD;});

	disconnect = ('D' @{cmd = DISCONNECT;}
		     xdigit+ $hex_digit %push);

	# TODO: We don't actually listen to what's supported
	q_attached = ('qAttached:' xdigit* @{rsp = "1";});
	q_C = ('qC' @{rsp = "QC1";});
	q_supported = ('qSupported:' any* @{rsp = "multiprocess+;vContSupported+";});
	qf_threadinfo = ('qfThreadInfo' @{rsp = "m1l";});

	# vCont packet parsing
	v_contq = ('vCont?' @{rsp = "vCont;c;C;s;S";});
	v_contc = ('vCont;c' any* @{cmd = V_CONTC;});
	v_conts = ('vCont;s' any* @{cmd = V_CONTS;});

	interrupt = (3 @{command_callbacks[INTERRUPT](stack, priv);});

	commands = (get_mem | get_gprs | get_spr | stop_reason | set_thread |
		    q_attached | q_C | q_supported | qf_threadinfo | q_C |
		    v_contq | v_contc | v_conts | put_mem | disconnect );

	cmd = ((commands & ^'#'*) | ^'#'*) $crc
	      ('#' xdigit{2} $hex_digit @end);

	# We ignore ACK/NACK for the moment
	ack = ('+');
	nack = ('-');

	main := (( ^('$' | interrupt)*('$' | interrupt) @reset) (cmd | ack | nack))*;

}%%

static enum gdb_command cmd = NONE;
static uint64_t stack[10], *data = stack;
static char *rsp;
static uint8_t crc;
static int cs;

command_cb *command_callbacks;

%%write data;

void parser_init(command_cb *callbacks)
{
	%%write init;

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len + 1;

	%%write exec;

	return 0;
}

#if 0
int main(int argc, char **argv)
{
	parser_init(NULL);

	if (argc > 1)
		parse_buffer(argv[1], strlen(argv[1]), NULL);
	return 0;
}
#endif

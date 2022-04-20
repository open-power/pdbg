
#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 139 "src/gdb_parser.rl"


static enum gdb_command cmd = NONE;
static uint64_t stack[10], *data = stack;
static uint8_t *mem_data;
static uint64_t mem_data_length;
static uint64_t mem_data_i;
static char *rsp;
static uint8_t crc;
static int cs;

static command_cb *command_callbacks;


#line 30 "src/gdb_parser_precompile.c"
static const char _gdb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 16, 1, 21, 1, 22, 1, 
	23, 1, 24, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 5, 2, 
	4, 1, 2, 12, 1, 2, 14, 1, 
	2, 15, 1, 2, 16, 1, 2, 17, 
	1, 2, 18, 1, 2, 19, 1, 2, 
	20, 1, 3, 0, 6, 1, 3, 0, 
	7, 1, 3, 0, 9, 1, 3, 0, 
	10, 1, 3, 0, 11, 1, 3, 0, 
	13, 1, 3, 2, 8, 1
};

static const unsigned char _gdb_key_offsets[] = {
	0, 0, 10, 11, 17, 23, 30, 37, 
	38, 45, 53, 60, 68, 75, 82, 90, 
	95, 97, 99, 101, 103, 105, 107, 109, 
	111, 118, 120, 122, 124, 126, 128, 130, 
	132, 134, 136, 137, 139, 141, 143, 145, 
	147, 149, 151, 153, 155, 157, 159, 161, 
	163, 165, 168, 171, 172, 173
};

static const char _gdb_trans_keys[] = {
	35, 63, 68, 72, 77, 103, 109, 112, 
	113, 118, 35, 48, 57, 65, 70, 97, 
	102, 48, 57, 65, 70, 97, 102, 35, 
	48, 57, 65, 70, 97, 102, 35, 48, 
	57, 65, 70, 97, 102, 35, 35, 48, 
	57, 65, 70, 97, 102, 35, 44, 48, 
	57, 65, 70, 97, 102, 35, 48, 57, 
	65, 70, 97, 102, 35, 58, 48, 57, 
	65, 70, 97, 102, 35, 48, 57, 65, 
	70, 97, 102, 35, 48, 57, 65, 70, 
	97, 102, 35, 44, 48, 57, 65, 70, 
	97, 102, 35, 65, 67, 83, 102, 35, 
	116, 35, 116, 35, 97, 35, 99, 35, 
	104, 35, 101, 35, 100, 35, 58, 35, 
	48, 57, 65, 70, 97, 102, 35, 117, 
	35, 112, 35, 112, 35, 111, 35, 114, 
	35, 116, 35, 101, 35, 100, 35, 58, 
	35, 35, 84, 35, 104, 35, 114, 35, 
	101, 35, 97, 35, 100, 35, 73, 35, 
	110, 35, 102, 35, 111, 35, 67, 35, 
	111, 35, 110, 35, 116, 35, 59, 63, 
	35, 99, 115, 35, 35, 3, 36, 43, 
	45, 0
};

static const char _gdb_single_lengths[] = {
	0, 10, 1, 0, 0, 1, 1, 1, 
	1, 2, 1, 2, 1, 1, 2, 5, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	1, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 1, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 3, 3, 1, 1, 4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	3, 3, 3, 3, 3, 3, 3, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0
};

static const unsigned char _gdb_index_offsets[] = {
	0, 0, 11, 13, 17, 21, 26, 31, 
	33, 38, 44, 49, 55, 60, 65, 71, 
	77, 80, 83, 86, 89, 92, 95, 98, 
	101, 106, 109, 112, 115, 118, 121, 124, 
	127, 130, 133, 135, 138, 141, 144, 147, 
	150, 153, 156, 159, 162, 165, 168, 171, 
	174, 177, 181, 185, 187, 189
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 0, 12, 11, 13, 13, 13, 
	14, 15, 15, 15, 14, 12, 16, 16, 
	16, 11, 17, 16, 16, 16, 11, 12, 
	18, 12, 19, 19, 19, 11, 12, 20, 
	19, 19, 19, 11, 12, 21, 21, 21, 
	11, 12, 22, 21, 21, 21, 11, 12, 
	23, 23, 23, 11, 12, 24, 24, 24, 
	11, 12, 25, 24, 24, 24, 11, 12, 
	26, 27, 28, 29, 11, 12, 30, 11, 
	12, 31, 11, 12, 32, 11, 12, 33, 
	11, 12, 34, 11, 12, 35, 11, 12, 
	36, 11, 12, 37, 11, 12, 38, 38, 
	38, 11, 12, 39, 11, 12, 40, 11, 
	12, 41, 11, 12, 42, 11, 12, 43, 
	11, 12, 44, 11, 12, 45, 11, 12, 
	46, 11, 12, 47, 11, 49, 48, 12, 
	50, 11, 12, 51, 11, 12, 52, 11, 
	12, 53, 11, 12, 54, 11, 12, 55, 
	11, 12, 56, 11, 12, 57, 11, 12, 
	58, 11, 12, 59, 11, 12, 60, 11, 
	12, 61, 11, 12, 62, 11, 12, 63, 
	11, 12, 64, 65, 11, 12, 66, 67, 
	11, 12, 68, 12, 69, 70, 71, 72, 
	73, 14, 0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 8, 2, 13, 
	5, 15, 45, 2, 3, 4, 0, 53, 
	6, 3, 7, 9, 10, 11, 12, 12, 
	14, 5, 16, 2, 25, 35, 17, 18, 
	19, 20, 21, 22, 23, 24, 24, 26, 
	27, 28, 29, 30, 31, 32, 33, 34, 
	2, 3, 36, 37, 38, 39, 40, 41, 
	42, 43, 44, 2, 46, 47, 48, 49, 
	50, 2, 51, 52, 51, 52, 53, 1, 
	53, 53
};

static const char _gdb_trans_actions[] = {
	19, 1, 74, 78, 19, 62, 66, 58, 
	70, 19, 19, 3, 0, 7, 0, 28, 
	25, 5, 34, 25, 22, 25, 82, 31, 
	25, 22, 3, 40, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 37, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	43, 9, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 46, 3, 3, 3, 3, 
	3, 49, 3, 3, 52, 55, 11, 13, 
	15, 17
};

static const int gdb_start = 53;
static const int gdb_first_final = 53;
static const int gdb_error = 0;

static const int gdb_en_main = 53;


#line 153 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 177 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 157 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 193 "src/gdb_parser_precompile.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _gdb_trans_keys + _gdb_key_offsets[cs];
	_trans = _gdb_index_offsets[cs];

	_klen = _gdb_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _gdb_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _gdb_indicies[_trans];
	cs = _gdb_trans_targs[_trans];

	if ( _gdb_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _gdb_actions + _gdb_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 14 "src/gdb_parser.rl"
	{
		cmd = 0;
		rsp = NULL;
		data = stack;
		memset(stack, 0, sizeof(stack));
		crc = 0;
		PR_INFO("RAGEL: CRC reset\n");
	}
	break;
	case 1:
#line 23 "src/gdb_parser.rl"
	{
		crc += *p;
	}
	break;
	case 2:
#line 27 "src/gdb_parser.rl"
	{
		data++;
		assert(data < &stack[10]);
	}
	break;
	case 3:
#line 32 "src/gdb_parser.rl"
	{
		*data *= 16;

		if (*p >= '0' && *p <= '9')
			*data += *p - '0';
		else if (*p >= 'a' && *p <= 'f')
			*data += *p - 'a' + 10;
		else if (*p >= 'A' && *p <= 'F')
			*data += *p - 'A' + 10;
	}
	break;
	case 4:
#line 43 "src/gdb_parser.rl"
	{
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
	break;
	case 5:
#line 61 "src/gdb_parser.rl"
	{
		/* *data should point to the CRC */
		if (crc != *data) {
			printf("CRC error cmd %d\n", cmd);
			send_nack(priv);
		} else {
			PR_INFO("Cmd %d\n", cmd);
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
	break;
	case 6:
#line 81 "src/gdb_parser.rl"
	{cmd = GET_MEM;}
	break;
	case 7:
#line 86 "src/gdb_parser.rl"
	{cmd = PUT_MEM;}
	break;
	case 8:
#line 91 "src/gdb_parser.rl"
	{	mem_data_length = *(data - 1);
			mem_data = calloc(1, mem_data_length); // handler frees
			*data = (unsigned long)mem_data;
			data++;
			assert(data < &stack[10]);
			mem_data_i = 0;
		   }
	break;
	case 9:
#line 100 "src/gdb_parser.rl"
	{cmd = GET_GPRS;}
	break;
	case 10:
#line 102 "src/gdb_parser.rl"
	{cmd = GET_SPR;}
	break;
	case 11:
#line 105 "src/gdb_parser.rl"
	{cmd = STOP_REASON;}
	break;
	case 12:
#line 107 "src/gdb_parser.rl"
	{cmd = SET_THREAD;}
	break;
	case 13:
#line 109 "src/gdb_parser.rl"
	{cmd = DETACH;}
	break;
	case 14:
#line 113 "src/gdb_parser.rl"
	{rsp = "1";}
	break;
	case 15:
#line 114 "src/gdb_parser.rl"
	{rsp = "QC1";}
	break;
	case 16:
#line 115 "src/gdb_parser.rl"
	{rsp = "multiprocess+;vContSupported+";}
	break;
	case 17:
#line 116 "src/gdb_parser.rl"
	{rsp = "m1l";}
	break;
	case 18:
#line 119 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 19:
#line 120 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 20:
#line 121 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 21:
#line 124 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 22:
#line 131 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 23:
#line 134 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 24:
#line 135 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 428 "src/gdb_parser_precompile.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 167 "src/gdb_parser.rl"

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

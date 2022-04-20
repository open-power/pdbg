
#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 145 "src/gdb_parser.rl"


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


#line 32 "src/gdb_parser_precompile.c"
static const char _gdb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 15, 1, 23, 1, 24, 1, 
	25, 1, 26, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 5, 2, 
	4, 1, 2, 13, 1, 2, 15, 1, 
	2, 16, 1, 2, 17, 1, 2, 20, 
	1, 2, 21, 1, 2, 22, 1, 3, 
	0, 6, 1, 3, 0, 7, 1, 3, 
	0, 9, 1, 3, 0, 10, 1, 3, 
	0, 11, 1, 3, 0, 12, 1, 3, 
	2, 8, 1, 3, 3, 19, 1, 3, 
	18, 14, 1
};

static const unsigned char _gdb_key_offsets[] = {
	0, 0, 11, 12, 18, 24, 31, 38, 
	40, 42, 49, 57, 65, 72, 79, 87, 
	94, 102, 109, 111, 113, 115, 117, 119, 
	121, 123, 125, 127, 129, 131, 133, 135, 
	137, 144, 152, 157, 159, 161, 163, 165, 
	167, 169, 171, 173, 180, 182, 184, 186, 
	188, 190, 192, 194, 196, 198, 199, 201, 
	203, 205, 207, 209, 211, 213, 215, 217, 
	219, 221, 223, 225, 227, 230, 233, 234, 
	235
};

static const char _gdb_trans_keys[] = {
	35, 63, 68, 72, 77, 81, 103, 109, 
	112, 113, 118, 35, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	48, 57, 65, 70, 97, 102, 35, 103, 
	35, 112, 35, 48, 57, 65, 70, 97, 
	102, 35, 46, 48, 57, 65, 70, 97, 
	102, 35, 112, 48, 57, 65, 70, 97, 
	102, 35, 48, 57, 65, 70, 97, 102, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	44, 48, 57, 65, 70, 97, 102, 35, 
	48, 57, 65, 70, 97, 102, 35, 58, 
	48, 57, 65, 70, 97, 102, 35, 48, 
	57, 65, 70, 97, 102, 35, 83, 35, 
	116, 35, 97, 35, 114, 35, 116, 35, 
	78, 35, 111, 35, 65, 35, 99, 35, 
	107, 35, 77, 35, 111, 35, 100, 35, 
	101, 35, 48, 57, 65, 70, 97, 102, 
	35, 44, 48, 57, 65, 70, 97, 102, 
	35, 65, 67, 83, 102, 35, 116, 35, 
	116, 35, 97, 35, 99, 35, 104, 35, 
	101, 35, 100, 35, 58, 35, 48, 57, 
	65, 70, 97, 102, 35, 117, 35, 112, 
	35, 112, 35, 111, 35, 114, 35, 116, 
	35, 101, 35, 100, 35, 58, 35, 35, 
	84, 35, 104, 35, 114, 35, 101, 35, 
	97, 35, 100, 35, 73, 35, 110, 35, 
	102, 35, 111, 35, 67, 35, 111, 35, 
	110, 35, 116, 35, 59, 63, 35, 99, 
	115, 35, 35, 3, 36, 43, 45, 0
};

static const char _gdb_single_lengths[] = {
	0, 11, 1, 0, 0, 1, 1, 2, 
	2, 1, 2, 2, 1, 1, 2, 1, 
	2, 1, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	1, 2, 5, 2, 2, 2, 2, 2, 
	2, 2, 2, 1, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 1, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 3, 3, 1, 1, 
	4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	0, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const short _gdb_index_offsets[] = {
	0, 0, 12, 14, 18, 22, 27, 32, 
	35, 38, 43, 49, 55, 60, 65, 71, 
	76, 82, 87, 90, 93, 96, 99, 102, 
	105, 108, 111, 114, 117, 120, 123, 126, 
	129, 134, 140, 146, 149, 152, 155, 158, 
	161, 164, 167, 170, 175, 178, 181, 184, 
	187, 190, 193, 196, 199, 202, 204, 207, 
	210, 213, 216, 219, 222, 225, 228, 231, 
	234, 237, 240, 243, 246, 250, 254, 256, 
	258
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 11, 0, 13, 12, 14, 14, 
	14, 15, 16, 16, 16, 15, 13, 17, 
	17, 17, 12, 18, 17, 17, 17, 12, 
	13, 19, 12, 13, 20, 12, 13, 21, 
	21, 21, 12, 13, 22, 21, 21, 21, 
	12, 13, 20, 23, 23, 23, 12, 18, 
	23, 23, 23, 12, 13, 24, 24, 24, 
	12, 13, 25, 24, 24, 24, 12, 13, 
	26, 26, 26, 12, 13, 27, 26, 26, 
	26, 12, 13, 28, 28, 28, 12, 13, 
	29, 12, 13, 30, 12, 13, 31, 12, 
	13, 32, 12, 13, 33, 12, 13, 34, 
	12, 13, 35, 12, 13, 36, 12, 13, 
	37, 12, 13, 38, 12, 13, 39, 12, 
	13, 40, 12, 13, 41, 12, 13, 42, 
	12, 13, 43, 43, 43, 12, 13, 44, 
	43, 43, 43, 12, 13, 45, 46, 47, 
	48, 12, 13, 49, 12, 13, 50, 12, 
	13, 51, 12, 13, 52, 12, 13, 53, 
	12, 13, 54, 12, 13, 55, 12, 13, 
	56, 12, 13, 57, 57, 57, 12, 13, 
	58, 12, 13, 59, 12, 13, 60, 12, 
	13, 61, 12, 13, 62, 12, 13, 63, 
	12, 13, 64, 12, 13, 65, 12, 13, 
	66, 12, 68, 67, 13, 69, 12, 13, 
	70, 12, 13, 71, 12, 13, 72, 12, 
	13, 73, 12, 13, 74, 12, 13, 75, 
	12, 13, 76, 12, 13, 77, 12, 13, 
	78, 12, 13, 79, 12, 13, 80, 12, 
	13, 81, 12, 13, 82, 12, 13, 83, 
	84, 12, 13, 85, 86, 12, 13, 87, 
	13, 88, 89, 90, 91, 92, 15, 0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 13, 18, 2, 
	32, 5, 34, 64, 2, 3, 4, 0, 
	72, 6, 3, 8, 9, 10, 11, 12, 
	14, 15, 16, 17, 17, 19, 20, 21, 
	22, 23, 24, 25, 26, 27, 28, 29, 
	30, 31, 2, 33, 5, 35, 2, 44, 
	54, 36, 37, 38, 39, 40, 41, 42, 
	43, 43, 45, 46, 47, 48, 49, 50, 
	51, 52, 53, 2, 3, 55, 56, 57, 
	58, 59, 60, 61, 62, 63, 2, 65, 
	66, 67, 68, 69, 2, 70, 71, 70, 
	71, 72, 1, 72, 72
};

static const char _gdb_trans_actions[] = {
	19, 1, 71, 75, 19, 59, 19, 63, 
	55, 67, 19, 19, 3, 0, 7, 0, 
	28, 25, 5, 3, 3, 3, 3, 83, 
	25, 22, 25, 79, 31, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 43, 25, 22, 3, 87, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 34, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 37, 9, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 40, 3, 
	3, 3, 3, 3, 46, 3, 3, 49, 
	52, 11, 13, 15, 17
};

static const int gdb_start = 72;
static const int gdb_first_final = 72;
static const int gdb_error = 0;

static const int gdb_en_main = 72;


#line 161 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 211 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 165 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 227 "src/gdb_parser_precompile.c"
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
	break;
	case 6:
#line 83 "src/gdb_parser.rl"
	{cmd = GET_MEM;}
	break;
	case 7:
#line 88 "src/gdb_parser.rl"
	{cmd = PUT_MEM;}
	break;
	case 8:
#line 93 "src/gdb_parser.rl"
	{	mem_data_length = *(data - 1);
			mem_data = calloc(1, mem_data_length); // handler frees
			*data = (unsigned long)mem_data;
			data++;
			assert(data < &stack[10]);
			mem_data_i = 0;
		   }
	break;
	case 9:
#line 102 "src/gdb_parser.rl"
	{cmd = GET_GPRS;}
	break;
	case 10:
#line 104 "src/gdb_parser.rl"
	{cmd = GET_SPR;}
	break;
	case 11:
#line 107 "src/gdb_parser.rl"
	{cmd = STOP_REASON;}
	break;
	case 12:
#line 109 "src/gdb_parser.rl"
	{cmd = DETACH;}
	break;
	case 13:
#line 113 "src/gdb_parser.rl"
	{rsp = "1";}
	break;
	case 14:
#line 114 "src/gdb_parser.rl"
	{rsp = "QC1";}
	break;
	case 15:
#line 115 "src/gdb_parser.rl"
	{rsp = "multiprocess+;vContSupported+;QStartNoAckMode+"; ack_mode = true;}
	break;
	case 16:
#line 116 "src/gdb_parser.rl"
	{rsp = "m1l";}
	break;
	case 17:
#line 117 "src/gdb_parser.rl"
	{rsp = "OK"; send_ack(priv); ack_mode = false;}
	break;
	case 18:
#line 120 "src/gdb_parser.rl"
	{cmd = GET_THREAD;}
	break;
	case 19:
#line 121 "src/gdb_parser.rl"
	{cmd = SET_THREAD;}
	break;
	case 20:
#line 124 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 21:
#line 125 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 22:
#line 126 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 23:
#line 129 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 24:
#line 137 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 25:
#line 140 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 26:
#line 141 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 472 "src/gdb_parser_precompile.c"
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

#line 175 "src/gdb_parser.rl"

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

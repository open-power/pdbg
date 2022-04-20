
#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 143 "src/gdb_parser.rl"


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
	3, 1, 16, 1, 22, 1, 23, 1, 
	24, 1, 25, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 5, 2, 
	4, 1, 2, 12, 1, 2, 14, 1, 
	2, 15, 1, 2, 16, 1, 2, 17, 
	1, 2, 18, 1, 2, 19, 1, 2, 
	20, 1, 2, 21, 1, 3, 0, 6, 
	1, 3, 0, 7, 1, 3, 0, 9, 
	1, 3, 0, 10, 1, 3, 0, 11, 
	1, 3, 0, 13, 1, 3, 2, 8, 
	1
};

static const unsigned char _gdb_key_offsets[] = {
	0, 0, 11, 12, 18, 24, 31, 38, 
	39, 46, 54, 61, 69, 76, 78, 80, 
	82, 84, 86, 88, 90, 92, 94, 96, 
	98, 100, 102, 104, 111, 119, 124, 126, 
	128, 130, 132, 134, 136, 138, 140, 147, 
	149, 151, 153, 155, 157, 159, 161, 163, 
	165, 166, 168, 170, 172, 174, 176, 178, 
	180, 182, 184, 186, 188, 190, 192, 194, 
	197, 200, 201, 202
};

static const char _gdb_trans_keys[] = {
	35, 63, 68, 72, 77, 81, 103, 109, 
	112, 113, 118, 35, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	48, 57, 65, 70, 97, 102, 35, 35, 
	48, 57, 65, 70, 97, 102, 35, 44, 
	48, 57, 65, 70, 97, 102, 35, 48, 
	57, 65, 70, 97, 102, 35, 58, 48, 
	57, 65, 70, 97, 102, 35, 48, 57, 
	65, 70, 97, 102, 35, 83, 35, 116, 
	35, 97, 35, 114, 35, 116, 35, 78, 
	35, 111, 35, 65, 35, 99, 35, 107, 
	35, 77, 35, 111, 35, 100, 35, 101, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	44, 48, 57, 65, 70, 97, 102, 35, 
	65, 67, 83, 102, 35, 116, 35, 116, 
	35, 97, 35, 99, 35, 104, 35, 101, 
	35, 100, 35, 58, 35, 48, 57, 65, 
	70, 97, 102, 35, 117, 35, 112, 35, 
	112, 35, 111, 35, 114, 35, 116, 35, 
	101, 35, 100, 35, 58, 35, 35, 84, 
	35, 104, 35, 114, 35, 101, 35, 97, 
	35, 100, 35, 73, 35, 110, 35, 102, 
	35, 111, 35, 67, 35, 111, 35, 110, 
	35, 116, 35, 59, 63, 35, 99, 115, 
	35, 35, 3, 36, 43, 45, 0
};

static const char _gdb_single_lengths[] = {
	0, 11, 1, 0, 0, 1, 1, 1, 
	1, 2, 1, 2, 1, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 1, 2, 5, 2, 2, 
	2, 2, 2, 2, 2, 2, 1, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	1, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 3, 
	3, 1, 1, 4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	3, 3, 3, 3, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 3, 3, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 3, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _gdb_index_offsets[] = {
	0, 0, 12, 14, 18, 22, 27, 32, 
	34, 39, 45, 50, 56, 61, 64, 67, 
	70, 73, 76, 79, 82, 85, 88, 91, 
	94, 97, 100, 103, 108, 114, 120, 123, 
	126, 129, 132, 135, 138, 141, 144, 149, 
	152, 155, 158, 161, 164, 167, 170, 173, 
	176, 178, 181, 184, 187, 190, 193, 196, 
	199, 202, 205, 208, 211, 214, 217, 220, 
	224, 228, 230, 232
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 11, 0, 13, 12, 14, 14, 
	14, 15, 16, 16, 16, 15, 13, 17, 
	17, 17, 12, 18, 17, 17, 17, 12, 
	13, 19, 13, 20, 20, 20, 12, 13, 
	21, 20, 20, 20, 12, 13, 22, 22, 
	22, 12, 13, 23, 22, 22, 22, 12, 
	13, 24, 24, 24, 12, 13, 25, 12, 
	13, 26, 12, 13, 27, 12, 13, 28, 
	12, 13, 29, 12, 13, 30, 12, 13, 
	31, 12, 13, 32, 12, 13, 33, 12, 
	13, 34, 12, 13, 35, 12, 13, 36, 
	12, 13, 37, 12, 13, 38, 12, 13, 
	39, 39, 39, 12, 13, 40, 39, 39, 
	39, 12, 13, 41, 42, 43, 44, 12, 
	13, 45, 12, 13, 46, 12, 13, 47, 
	12, 13, 48, 12, 13, 49, 12, 13, 
	50, 12, 13, 51, 12, 13, 52, 12, 
	13, 53, 53, 53, 12, 13, 54, 12, 
	13, 55, 12, 13, 56, 12, 13, 57, 
	12, 13, 58, 12, 13, 59, 12, 13, 
	60, 12, 13, 61, 12, 13, 62, 12, 
	64, 63, 13, 65, 12, 13, 66, 12, 
	13, 67, 12, 13, 68, 12, 13, 69, 
	12, 13, 70, 12, 13, 71, 12, 13, 
	72, 12, 13, 73, 12, 13, 74, 12, 
	13, 75, 12, 13, 76, 12, 13, 77, 
	12, 13, 78, 12, 13, 79, 80, 12, 
	13, 81, 82, 12, 13, 83, 13, 84, 
	85, 86, 87, 88, 15, 0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 8, 13, 2, 
	27, 5, 29, 59, 2, 3, 4, 0, 
	67, 6, 3, 7, 9, 10, 11, 12, 
	12, 14, 15, 16, 17, 18, 19, 20, 
	21, 22, 23, 24, 25, 26, 2, 28, 
	5, 30, 2, 39, 49, 31, 32, 33, 
	34, 35, 36, 37, 38, 38, 40, 41, 
	42, 43, 44, 45, 46, 47, 48, 2, 
	3, 50, 51, 52, 53, 54, 55, 56, 
	57, 58, 2, 60, 61, 62, 63, 64, 
	2, 65, 66, 65, 66, 67, 1, 67, 
	67
};

static const char _gdb_trans_actions[] = {
	19, 1, 77, 81, 19, 65, 19, 69, 
	61, 73, 19, 19, 3, 0, 7, 0, 
	28, 25, 5, 34, 25, 22, 25, 85, 
	31, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 49, 25, 
	22, 3, 40, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 37, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 43, 
	9, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 46, 3, 3, 3, 3, 3, 
	52, 3, 3, 55, 58, 11, 13, 15, 
	17
};

static const int gdb_start = 67;
static const int gdb_first_final = 67;
static const int gdb_error = 0;

static const int gdb_en_main = 67;


#line 159 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 200 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 163 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 216 "src/gdb_parser_precompile.c"
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
	{cmd = SET_THREAD;}
	break;
	case 13:
#line 111 "src/gdb_parser.rl"
	{cmd = DETACH;}
	break;
	case 14:
#line 115 "src/gdb_parser.rl"
	{rsp = "1";}
	break;
	case 15:
#line 116 "src/gdb_parser.rl"
	{rsp = "QC1";}
	break;
	case 16:
#line 117 "src/gdb_parser.rl"
	{rsp = "multiprocess+;vContSupported+;QStartNoAckMode+"; ack_mode = true;}
	break;
	case 17:
#line 118 "src/gdb_parser.rl"
	{rsp = "m1l";}
	break;
	case 18:
#line 119 "src/gdb_parser.rl"
	{rsp = "OK"; send_ack(priv); ack_mode = false;}
	break;
	case 19:
#line 122 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 20:
#line 123 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 21:
#line 124 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 22:
#line 127 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 23:
#line 135 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 24:
#line 138 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 25:
#line 139 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 457 "src/gdb_parser_precompile.c"
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

#line 173 "src/gdb_parser.rl"

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

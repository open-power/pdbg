
#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 149 "src/gdb_parser.rl"


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
	3, 1, 14, 1, 24, 1, 25, 1, 
	26, 1, 27, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 5, 2, 
	4, 1, 2, 13, 1, 2, 14, 1, 
	2, 15, 1, 2, 16, 1, 2, 17, 
	1, 2, 19, 1, 2, 20, 1, 2, 
	21, 1, 2, 22, 1, 2, 23, 1, 
	3, 0, 6, 1, 3, 0, 7, 1, 
	3, 0, 9, 1, 3, 0, 10, 1, 
	3, 0, 11, 1, 3, 0, 12, 1, 
	3, 2, 8, 1, 3, 3, 18, 1
	
};

static const short _gdb_key_offsets[] = {
	0, 0, 12, 13, 19, 25, 32, 39, 
	41, 43, 50, 58, 66, 73, 80, 88, 
	95, 103, 110, 112, 114, 116, 118, 120, 
	122, 124, 126, 128, 130, 132, 134, 136, 
	138, 140, 147, 155, 163, 170, 177, 185, 
	191, 193, 195, 197, 199, 201, 203, 205, 
	207, 214, 216, 218, 220, 222, 224, 226, 
	228, 230, 232, 233, 235, 237, 239, 241, 
	243, 245, 247, 249, 251, 253, 255, 257, 
	259, 261, 263, 265, 267, 269, 271, 273, 
	275, 277, 279, 281, 284, 287, 288, 289
};

static const char _gdb_trans_keys[] = {
	35, 63, 68, 72, 77, 81, 84, 103, 
	109, 112, 113, 118, 35, 48, 57, 65, 
	70, 97, 102, 48, 57, 65, 70, 97, 
	102, 35, 48, 57, 65, 70, 97, 102, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	103, 35, 112, 35, 48, 57, 65, 70, 
	97, 102, 35, 46, 48, 57, 65, 70, 
	97, 102, 35, 112, 48, 57, 65, 70, 
	97, 102, 35, 48, 57, 65, 70, 97, 
	102, 35, 48, 57, 65, 70, 97, 102, 
	35, 44, 48, 57, 65, 70, 97, 102, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	58, 48, 57, 65, 70, 97, 102, 35, 
	48, 57, 65, 70, 97, 102, 35, 83, 
	35, 116, 35, 97, 35, 114, 35, 116, 
	35, 78, 35, 111, 35, 65, 35, 99, 
	35, 107, 35, 77, 35, 111, 35, 100, 
	35, 101, 35, 112, 35, 48, 57, 65, 
	70, 97, 102, 35, 46, 48, 57, 65, 
	70, 97, 102, 35, 112, 48, 57, 65, 
	70, 97, 102, 35, 48, 57, 65, 70, 
	97, 102, 35, 48, 57, 65, 70, 97, 
	102, 35, 44, 48, 57, 65, 70, 97, 
	102, 35, 65, 67, 83, 102, 115, 35, 
	116, 35, 116, 35, 97, 35, 99, 35, 
	104, 35, 101, 35, 100, 35, 58, 35, 
	48, 57, 65, 70, 97, 102, 35, 117, 
	35, 112, 35, 112, 35, 111, 35, 114, 
	35, 116, 35, 101, 35, 100, 35, 58, 
	35, 35, 84, 35, 104, 35, 114, 35, 
	101, 35, 97, 35, 100, 35, 73, 35, 
	110, 35, 102, 35, 111, 35, 84, 35, 
	104, 35, 114, 35, 101, 35, 97, 35, 
	100, 35, 73, 35, 110, 35, 102, 35, 
	111, 35, 67, 35, 111, 35, 110, 35, 
	116, 35, 59, 63, 35, 99, 115, 35, 
	35, 3, 36, 43, 45, 0
};

static const char _gdb_single_lengths[] = {
	0, 12, 1, 0, 0, 1, 1, 2, 
	2, 1, 2, 2, 1, 1, 2, 1, 
	2, 1, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 1, 2, 2, 1, 1, 2, 6, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	1, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 1, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 3, 3, 1, 1, 4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	0, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 3, 3, 3, 3, 3, 3, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	3, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0
};

static const short _gdb_index_offsets[] = {
	0, 0, 13, 15, 19, 23, 28, 33, 
	36, 39, 44, 50, 56, 61, 66, 72, 
	77, 83, 88, 91, 94, 97, 100, 103, 
	106, 109, 112, 115, 118, 121, 124, 127, 
	130, 133, 138, 144, 150, 155, 160, 166, 
	173, 176, 179, 182, 185, 188, 191, 194, 
	197, 202, 205, 208, 211, 214, 217, 220, 
	223, 226, 229, 231, 234, 237, 240, 243, 
	246, 249, 252, 255, 258, 261, 264, 267, 
	270, 273, 276, 279, 282, 285, 288, 291, 
	294, 297, 300, 303, 307, 311, 313, 315
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 11, 12, 0, 14, 13, 15, 
	15, 15, 16, 17, 17, 17, 16, 14, 
	18, 18, 18, 13, 19, 18, 18, 18, 
	13, 14, 20, 13, 14, 21, 13, 14, 
	22, 22, 22, 13, 14, 23, 22, 22, 
	22, 13, 14, 21, 24, 24, 24, 13, 
	19, 24, 24, 24, 13, 14, 25, 25, 
	25, 13, 14, 26, 25, 25, 25, 13, 
	14, 27, 27, 27, 13, 14, 28, 27, 
	27, 27, 13, 14, 29, 29, 29, 13, 
	14, 30, 13, 14, 31, 13, 14, 32, 
	13, 14, 33, 13, 14, 34, 13, 14, 
	35, 13, 14, 36, 13, 14, 37, 13, 
	14, 38, 13, 14, 39, 13, 14, 40, 
	13, 14, 41, 13, 14, 42, 13, 14, 
	43, 13, 14, 44, 13, 14, 45, 45, 
	45, 13, 14, 46, 45, 45, 45, 13, 
	14, 44, 47, 47, 47, 13, 14, 47, 
	47, 47, 13, 14, 48, 48, 48, 13, 
	14, 49, 48, 48, 48, 13, 14, 50, 
	51, 52, 53, 54, 13, 14, 55, 13, 
	14, 56, 13, 14, 57, 13, 14, 58, 
	13, 14, 59, 13, 14, 60, 13, 14, 
	61, 13, 14, 62, 13, 14, 63, 63, 
	63, 13, 14, 64, 13, 14, 65, 13, 
	14, 66, 13, 14, 67, 13, 14, 68, 
	13, 14, 69, 13, 14, 70, 13, 14, 
	71, 13, 14, 72, 13, 74, 73, 14, 
	75, 13, 14, 76, 13, 14, 77, 13, 
	14, 78, 13, 14, 79, 13, 14, 80, 
	13, 14, 81, 13, 14, 82, 13, 14, 
	83, 13, 14, 84, 13, 14, 85, 13, 
	14, 86, 13, 14, 87, 13, 14, 88, 
	13, 14, 89, 13, 14, 90, 13, 14, 
	91, 13, 14, 92, 13, 14, 93, 13, 
	14, 94, 13, 14, 95, 13, 14, 96, 
	13, 14, 97, 13, 14, 98, 13, 14, 
	99, 100, 13, 14, 101, 102, 13, 14, 
	103, 14, 104, 105, 106, 107, 108, 16, 
	0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 13, 18, 32, 
	2, 37, 5, 39, 79, 2, 3, 4, 
	0, 87, 6, 3, 8, 9, 10, 11, 
	12, 14, 15, 16, 17, 17, 19, 20, 
	21, 22, 23, 24, 25, 26, 27, 28, 
	29, 30, 31, 2, 33, 34, 35, 36, 
	38, 5, 40, 2, 49, 59, 69, 41, 
	42, 43, 44, 45, 46, 47, 48, 48, 
	50, 51, 52, 53, 54, 55, 56, 57, 
	58, 2, 3, 60, 61, 62, 63, 64, 
	65, 66, 67, 68, 2, 70, 71, 72, 
	73, 74, 75, 76, 77, 78, 2, 80, 
	81, 82, 83, 84, 2, 85, 86, 85, 
	86, 87, 1, 87, 87
};

static const char _gdb_trans_actions[] = {
	19, 1, 80, 84, 19, 68, 19, 19, 
	72, 64, 76, 19, 19, 3, 0, 7, 
	0, 28, 25, 5, 3, 3, 3, 3, 
	92, 25, 22, 25, 88, 31, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 40, 3, 3, 3, 43, 
	25, 22, 3, 46, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 34, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 37, 9, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 49, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 52, 3, 
	3, 3, 3, 3, 55, 3, 3, 58, 
	61, 11, 13, 15, 17
};

static const int gdb_start = 87;
static const int gdb_first_final = 87;
static const int gdb_error = 0;

static const int gdb_en_main = 87;


#line 165 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 235 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 169 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 251 "src/gdb_parser_precompile.c"
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
	{rsp = "multiprocess+;swbreak+;hwbreak-;qRelocInsn-;vContSupported+;QThreadEvents-;no-resumed-;QStartNoAckMode+"; ack_mode = true;}
	break;
	case 15:
#line 115 "src/gdb_parser.rl"
	{rsp = "OK"; send_ack(priv); ack_mode = false;}
	break;
	case 16:
#line 118 "src/gdb_parser.rl"
	{rsp = "OK";}
	break;
	case 17:
#line 119 "src/gdb_parser.rl"
	{cmd = GET_THREAD;}
	break;
	case 18:
#line 120 "src/gdb_parser.rl"
	{cmd = SET_THREAD;}
	break;
	case 19:
#line 121 "src/gdb_parser.rl"
	{cmd = QF_THREADINFO;}
	break;
	case 20:
#line 122 "src/gdb_parser.rl"
	{cmd = QS_THREADINFO;}
	break;
	case 21:
#line 125 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 22:
#line 126 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 23:
#line 127 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 24:
#line 130 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 25:
#line 141 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 26:
#line 144 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 27:
#line 145 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 500 "src/gdb_parser_precompile.c"
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

#line 179 "src/gdb_parser.rl"

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

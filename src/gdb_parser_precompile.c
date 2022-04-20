
#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 154 "src/gdb_parser.rl"


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
	3, 1, 14, 1, 26, 1, 27, 1, 
	28, 1, 29, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 5, 2, 
	4, 1, 2, 13, 1, 2, 14, 1, 
	2, 15, 1, 2, 16, 1, 2, 17, 
	1, 2, 19, 1, 2, 20, 1, 2, 
	21, 1, 2, 22, 1, 2, 23, 1, 
	2, 24, 1, 2, 25, 1, 3, 0, 
	6, 1, 3, 0, 7, 1, 3, 0, 
	9, 1, 3, 0, 10, 1, 3, 0, 
	11, 1, 3, 0, 12, 1, 3, 2, 
	8, 1, 3, 3, 18, 1
};

static const short _gdb_key_offsets[] = {
	0, 0, 14, 15, 21, 27, 34, 41, 
	43, 45, 52, 60, 68, 75, 82, 90, 
	97, 105, 112, 114, 116, 118, 120, 122, 
	124, 126, 128, 130, 132, 134, 136, 138, 
	140, 142, 149, 157, 165, 172, 174, 176, 
	183, 191, 193, 200, 208, 214, 216, 218, 
	220, 222, 224, 226, 228, 230, 237, 239, 
	241, 243, 245, 247, 249, 251, 253, 255, 
	256, 258, 260, 262, 264, 266, 268, 270, 
	272, 274, 276, 278, 280, 282, 284, 286, 
	288, 290, 292, 294, 296, 298, 300, 302, 
	304, 307, 310, 311, 312, 314, 316, 323, 
	331, 333
};

static const char _gdb_trans_keys[] = {
	35, 63, 68, 72, 77, 81, 84, 90, 
	103, 109, 112, 113, 118, 122, 35, 48, 
	57, 65, 70, 97, 102, 48, 57, 65, 
	70, 97, 102, 35, 48, 57, 65, 70, 
	97, 102, 35, 48, 57, 65, 70, 97, 
	102, 35, 103, 35, 112, 35, 48, 57, 
	65, 70, 97, 102, 35, 46, 48, 57, 
	65, 70, 97, 102, 35, 112, 48, 57, 
	65, 70, 97, 102, 35, 48, 57, 65, 
	70, 97, 102, 35, 48, 57, 65, 70, 
	97, 102, 35, 44, 48, 57, 65, 70, 
	97, 102, 35, 48, 57, 65, 70, 97, 
	102, 35, 58, 48, 57, 65, 70, 97, 
	102, 35, 48, 57, 65, 70, 97, 102, 
	35, 83, 35, 116, 35, 97, 35, 114, 
	35, 116, 35, 78, 35, 111, 35, 65, 
	35, 99, 35, 107, 35, 77, 35, 111, 
	35, 100, 35, 101, 35, 112, 35, 48, 
	57, 65, 70, 97, 102, 35, 46, 48, 
	57, 65, 70, 97, 102, 35, 112, 48, 
	57, 65, 70, 97, 102, 35, 48, 57, 
	65, 70, 97, 102, 35, 48, 35, 44, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	44, 48, 57, 65, 70, 97, 102, 35, 
	52, 35, 48, 57, 65, 70, 97, 102, 
	35, 44, 48, 57, 65, 70, 97, 102, 
	35, 65, 67, 83, 102, 115, 35, 116, 
	35, 116, 35, 97, 35, 99, 35, 104, 
	35, 101, 35, 100, 35, 58, 35, 48, 
	57, 65, 70, 97, 102, 35, 117, 35, 
	112, 35, 112, 35, 111, 35, 114, 35, 
	116, 35, 101, 35, 100, 35, 58, 35, 
	35, 84, 35, 104, 35, 114, 35, 101, 
	35, 97, 35, 100, 35, 73, 35, 110, 
	35, 102, 35, 111, 35, 84, 35, 104, 
	35, 114, 35, 101, 35, 97, 35, 100, 
	35, 73, 35, 110, 35, 102, 35, 111, 
	35, 67, 35, 111, 35, 110, 35, 116, 
	35, 59, 63, 35, 99, 115, 35, 35, 
	35, 48, 35, 44, 35, 48, 57, 65, 
	70, 97, 102, 35, 44, 48, 57, 65, 
	70, 97, 102, 35, 52, 3, 36, 43, 
	45, 0
};

static const char _gdb_single_lengths[] = {
	0, 14, 1, 0, 0, 1, 1, 2, 
	2, 1, 2, 2, 1, 1, 2, 1, 
	2, 1, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 1, 2, 2, 1, 2, 2, 1, 
	2, 2, 1, 2, 6, 2, 2, 2, 
	2, 2, 2, 2, 2, 1, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 1, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	3, 3, 1, 1, 2, 2, 1, 2, 
	2, 4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	0, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 3, 3, 3, 3, 0, 0, 3, 
	3, 0, 3, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 3, 3, 
	0, 0
};

static const short _gdb_index_offsets[] = {
	0, 0, 15, 17, 21, 25, 30, 35, 
	38, 41, 46, 52, 58, 63, 68, 74, 
	79, 85, 90, 93, 96, 99, 102, 105, 
	108, 111, 114, 117, 120, 123, 126, 129, 
	132, 135, 140, 146, 152, 157, 160, 163, 
	168, 174, 177, 182, 188, 195, 198, 201, 
	204, 207, 210, 213, 216, 219, 224, 227, 
	230, 233, 236, 239, 242, 245, 248, 251, 
	253, 256, 259, 262, 265, 268, 271, 274, 
	277, 280, 283, 286, 289, 292, 295, 298, 
	301, 304, 307, 310, 313, 316, 319, 322, 
	325, 329, 333, 335, 337, 340, 343, 348, 
	354, 357
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 11, 12, 13, 14, 0, 16, 
	15, 17, 17, 17, 18, 19, 19, 19, 
	18, 16, 20, 20, 20, 15, 21, 20, 
	20, 20, 15, 16, 22, 15, 16, 23, 
	15, 16, 24, 24, 24, 15, 16, 25, 
	24, 24, 24, 15, 16, 23, 26, 26, 
	26, 15, 21, 26, 26, 26, 15, 16, 
	27, 27, 27, 15, 16, 28, 27, 27, 
	27, 15, 16, 29, 29, 29, 15, 16, 
	30, 29, 29, 29, 15, 16, 31, 31, 
	31, 15, 16, 32, 15, 16, 33, 15, 
	16, 34, 15, 16, 35, 15, 16, 36, 
	15, 16, 37, 15, 16, 38, 15, 16, 
	39, 15, 16, 40, 15, 16, 41, 15, 
	16, 42, 15, 16, 43, 15, 16, 44, 
	15, 16, 45, 15, 16, 46, 15, 16, 
	47, 47, 47, 15, 16, 48, 47, 47, 
	47, 15, 16, 46, 49, 49, 49, 15, 
	16, 49, 49, 49, 15, 16, 50, 15, 
	16, 51, 15, 16, 52, 52, 52, 15, 
	16, 53, 52, 52, 52, 15, 16, 54, 
	15, 16, 55, 55, 55, 15, 16, 56, 
	55, 55, 55, 15, 16, 57, 58, 59, 
	60, 61, 15, 16, 62, 15, 16, 63, 
	15, 16, 64, 15, 16, 65, 15, 16, 
	66, 15, 16, 67, 15, 16, 68, 15, 
	16, 69, 15, 16, 70, 70, 70, 15, 
	16, 71, 15, 16, 72, 15, 16, 73, 
	15, 16, 74, 15, 16, 75, 15, 16, 
	76, 15, 16, 77, 15, 16, 78, 15, 
	16, 79, 15, 81, 80, 16, 82, 15, 
	16, 83, 15, 16, 84, 15, 16, 85, 
	15, 16, 86, 15, 16, 87, 15, 16, 
	88, 15, 16, 89, 15, 16, 90, 15, 
	16, 91, 15, 16, 92, 15, 16, 93, 
	15, 16, 94, 15, 16, 95, 15, 16, 
	96, 15, 16, 97, 15, 16, 98, 15, 
	16, 99, 15, 16, 100, 15, 16, 101, 
	15, 16, 102, 15, 16, 103, 15, 16, 
	104, 15, 16, 105, 15, 16, 106, 107, 
	15, 16, 108, 109, 15, 16, 110, 16, 
	111, 16, 112, 15, 16, 113, 15, 16, 
	114, 114, 114, 15, 16, 115, 114, 114, 
	114, 15, 16, 116, 15, 117, 118, 119, 
	120, 18, 0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 13, 18, 32, 
	37, 2, 42, 5, 44, 84, 92, 2, 
	3, 4, 0, 97, 6, 3, 8, 9, 
	10, 11, 12, 14, 15, 16, 17, 17, 
	19, 20, 21, 22, 23, 24, 25, 26, 
	27, 28, 29, 30, 31, 2, 33, 34, 
	35, 36, 38, 39, 40, 41, 2, 43, 
	5, 45, 2, 54, 64, 74, 46, 47, 
	48, 49, 50, 51, 52, 53, 53, 55, 
	56, 57, 58, 59, 60, 61, 62, 63, 
	2, 3, 65, 66, 67, 68, 69, 70, 
	71, 72, 73, 2, 75, 76, 77, 78, 
	79, 80, 81, 82, 83, 2, 85, 86, 
	87, 88, 89, 2, 90, 91, 90, 91, 
	93, 94, 95, 96, 2, 97, 1, 97, 
	97
};

static const char _gdb_trans_actions[] = {
	19, 1, 86, 90, 19, 74, 19, 19, 
	19, 78, 70, 82, 19, 19, 19, 3, 
	0, 7, 0, 28, 25, 5, 3, 3, 
	3, 3, 98, 25, 22, 25, 94, 31, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 40, 3, 3, 
	3, 43, 3, 3, 25, 22, 55, 25, 
	22, 3, 46, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 34, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	37, 9, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 49, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 52, 3, 3, 
	3, 3, 3, 61, 3, 3, 64, 67, 
	3, 3, 25, 22, 58, 11, 13, 15, 
	17
};

static const int gdb_start = 97;
static const int gdb_first_final = 97;
static const int gdb_error = 0;

static const int gdb_en_main = 97;


#line 170 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 258 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 174 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 274 "src/gdb_parser_precompile.c"
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
	{cmd = SET_BREAK;}
	break;
	case 22:
#line 126 "src/gdb_parser.rl"
	{cmd = CLEAR_BREAK;}
	break;
	case 23:
#line 129 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 24:
#line 130 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 25:
#line 131 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 26:
#line 134 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 27:
#line 146 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 28:
#line 149 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 29:
#line 150 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 531 "src/gdb_parser_precompile.c"
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

#line 184 "src/gdb_parser.rl"

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

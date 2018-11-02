
#line 1 "src/gdb_parser.rl"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "pdbgproxy.h"


#line 105 "src/gdb_parser.rl"


static enum gdb_command cmd = NONE;
static uint64_t stack[10], *data = stack;
static char *rsp;
static uint8_t crc;
static int cs;

command_cb *command_callbacks;


#line 24 "src/gdb_parser.c"
static const char _gdb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 2, 1, 0, 2, 2, 1, 2, 
	3, 1, 2, 3, 4, 2, 5, 1, 
	2, 6, 1, 2, 7, 1, 2, 8, 
	1, 2, 9, 1, 2, 10, 1, 2, 
	11, 1, 2, 12, 1, 2, 13, 1, 
	2, 14, 1, 2, 15, 1, 2, 16, 
	1, 2, 17, 1, 2, 18, 0, 3, 
	1, 18, 0, 3, 6, 2, 1, 3, 
	6, 3, 1
};

static const char _gdb_key_offsets[] = {
	0, 0, 2, 13, 14, 20, 26, 29, 
	37, 45, 46, 53, 61, 68, 77, 84, 
	92, 99, 107, 114, 121, 126, 128, 130, 
	132, 134, 136, 138, 140, 142, 149, 151, 
	153, 155, 157, 159, 161, 163, 165, 167, 
	168, 170, 172, 174, 176, 178, 180, 182, 
	184, 186, 188, 190, 192, 194, 196, 199, 
	202, 203, 204, 206
};

static const char _gdb_trans_keys[] = {
	3, 36, 35, 43, 45, 63, 72, 77, 
	103, 109, 112, 113, 118, 35, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 3, 35, 36, 3, 36, 48, 
	57, 65, 70, 97, 102, 3, 36, 48, 
	57, 65, 70, 97, 102, 35, 35, 48, 
	57, 65, 70, 97, 102, 35, 44, 48, 
	57, 65, 70, 97, 102, 35, 48, 57, 
	65, 70, 97, 102, 35, 44, 58, 48, 
	57, 65, 70, 97, 102, 35, 48, 57, 
	65, 70, 97, 102, 35, 44, 48, 57, 
	65, 70, 97, 102, 35, 48, 57, 65, 
	70, 97, 102, 35, 44, 48, 57, 65, 
	70, 97, 102, 35, 48, 57, 65, 70, 
	97, 102, 35, 48, 57, 65, 70, 97, 
	102, 35, 65, 67, 83, 102, 35, 116, 
	35, 116, 35, 97, 35, 99, 35, 104, 
	35, 101, 35, 100, 35, 58, 35, 48, 
	57, 65, 70, 97, 102, 35, 117, 35, 
	112, 35, 112, 35, 111, 35, 114, 35, 
	116, 35, 101, 35, 100, 35, 58, 35, 
	35, 84, 35, 104, 35, 114, 35, 101, 
	35, 97, 35, 100, 35, 73, 35, 110, 
	35, 102, 35, 111, 35, 67, 35, 111, 
	35, 110, 35, 116, 35, 59, 63, 35, 
	99, 115, 35, 35, 3, 36, 3, 35, 
	36, 0
};

static const char _gdb_single_lengths[] = {
	0, 2, 11, 1, 0, 0, 3, 2, 
	2, 1, 1, 2, 1, 3, 1, 2, 
	1, 2, 1, 1, 5, 2, 2, 2, 
	2, 2, 2, 2, 2, 1, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 1, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 3, 3, 
	1, 1, 2, 3
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 0, 3, 3, 0, 3, 
	3, 0, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 3, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0
};

static const short _gdb_index_offsets[] = {
	0, 0, 3, 15, 17, 21, 25, 29, 
	35, 41, 43, 48, 54, 59, 66, 71, 
	77, 82, 88, 93, 98, 104, 107, 110, 
	113, 116, 119, 122, 125, 128, 133, 136, 
	139, 142, 145, 148, 151, 154, 157, 160, 
	162, 165, 168, 171, 174, 177, 180, 183, 
	186, 189, 192, 195, 198, 201, 204, 208, 
	212, 214, 216, 219
};

static const char _gdb_indicies[] = {
	1, 2, 0, 4, 5, 5, 6, 7, 
	8, 9, 10, 11, 12, 13, 3, 4, 
	3, 14, 14, 14, 15, 16, 16, 16, 
	15, 18, 19, 20, 17, 1, 2, 21, 
	21, 21, 0, 1, 2, 16, 16, 16, 
	0, 4, 22, 4, 24, 24, 24, 23, 
	4, 25, 24, 24, 24, 23, 4, 26, 
	26, 26, 23, 4, 25, 27, 26, 26, 
	26, 23, 4, 28, 28, 28, 23, 29, 
	25, 28, 28, 28, 23, 4, 30, 30, 
	30, 3, 4, 31, 30, 30, 30, 3, 
	4, 32, 32, 32, 3, 29, 32, 32, 
	32, 3, 4, 33, 34, 35, 36, 3, 
	4, 37, 3, 4, 38, 3, 4, 39, 
	3, 4, 40, 3, 4, 41, 3, 4, 
	42, 3, 4, 43, 3, 4, 44, 3, 
	4, 45, 45, 45, 3, 4, 46, 3, 
	4, 47, 3, 4, 48, 3, 4, 49, 
	3, 4, 50, 3, 4, 51, 3, 4, 
	52, 3, 4, 53, 3, 4, 54, 3, 
	4, 55, 4, 56, 3, 4, 57, 3, 
	4, 58, 3, 4, 59, 3, 4, 60, 
	3, 4, 61, 3, 4, 62, 3, 4, 
	63, 3, 4, 64, 3, 4, 65, 3, 
	4, 66, 3, 4, 67, 3, 4, 68, 
	3, 4, 69, 3, 4, 70, 71, 3, 
	4, 72, 73, 3, 4, 74, 4, 75, 
	1, 2, 0, 18, 19, 20, 17, 0
};

static const char _gdb_trans_targs[] = {
	1, 2, 2, 3, 4, 59, 3, 9, 
	10, 3, 16, 18, 20, 50, 5, 0, 
	58, 6, 2, 7, 2, 8, 9, 10, 
	11, 12, 13, 14, 15, 4, 17, 18, 
	19, 21, 3, 30, 40, 22, 23, 24, 
	25, 26, 27, 28, 29, 29, 31, 32, 
	33, 34, 35, 36, 37, 38, 39, 39, 
	41, 42, 43, 44, 45, 46, 47, 48, 
	49, 3, 51, 52, 53, 54, 55, 3, 
	56, 57, 56, 57
};

static const char _gdb_trans_actions[] = {
	0, 60, 1, 3, 0, 3, 33, 3, 
	3, 27, 21, 30, 3, 3, 7, 0, 
	18, 3, 63, 0, 9, 7, 36, 24, 
	71, 67, 71, 67, 71, 5, 15, 12, 
	15, 3, 42, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 39, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 45, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 48, 3, 3, 3, 3, 3, 51, 
	3, 3, 54, 57
};

static const int gdb_start = 58;
static const int gdb_first_final = 58;
static const int gdb_error = 0;

static const int gdb_en_main = 58;


#line 116 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 181 "src/gdb_parser.c"
	{
	cs = gdb_start;
	}

#line 120 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len + 1;

	
#line 197 "src/gdb_parser.c"
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
#line 11 "src/gdb_parser.rl"
	{
		cmd = 0;
		rsp = NULL;
		data = stack;
		memset(stack, 0, sizeof(stack));
		crc = 0;
	}
	break;
	case 1:
#line 19 "src/gdb_parser.rl"
	{
		crc += *p;
	}
	break;
	case 2:
#line 23 "src/gdb_parser.rl"
	{
		data++;
		assert(data < &stack[10]);
	}
	break;
	case 3:
#line 28 "src/gdb_parser.rl"
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
#line 39 "src/gdb_parser.rl"
	{
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
	break;
	case 5:
#line 58 "src/gdb_parser.rl"
	{cmd = GET_MEM;}
	break;
	case 6:
#line 63 "src/gdb_parser.rl"
	{cmd = PUT_MEM;}
	break;
	case 7:
#line 70 "src/gdb_parser.rl"
	{cmd = GET_GPRS;}
	break;
	case 8:
#line 72 "src/gdb_parser.rl"
	{cmd = GET_SPR;}
	break;
	case 9:
#line 75 "src/gdb_parser.rl"
	{cmd = STOP_REASON;}
	break;
	case 10:
#line 77 "src/gdb_parser.rl"
	{cmd = SET_THREAD;}
	break;
	case 11:
#line 80 "src/gdb_parser.rl"
	{rsp = "1";}
	break;
	case 12:
#line 81 "src/gdb_parser.rl"
	{rsp = "QC1";}
	break;
	case 13:
#line 82 "src/gdb_parser.rl"
	{rsp = "multiprocess+;vContSupported+";}
	break;
	case 14:
#line 83 "src/gdb_parser.rl"
	{rsp = "m1l";}
	break;
	case 15:
#line 86 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 16:
#line 87 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 17:
#line 88 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 18:
#line 90 "src/gdb_parser.rl"
	{command_callbacks[INTERRUPT](stack, priv);}
	break;
#line 384 "src/gdb_parser.c"
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

#line 130 "src/gdb_parser.rl"

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


#line 1 "src/gdb_parser.rl"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "src/pdbgproxy.h"
#include "debug.h"


#line 113 "src/gdb_parser.rl"


static enum gdb_command cmd = NONE;
static uint64_t stack[10], *data = stack;
static char *rsp;
static uint8_t crc;
static int cs;

static command_cb *command_callbacks;


#line 26 "src/gdb_parser_precompile.c"
static const char _gdb_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 14, 1, 19, 1, 20, 1, 
	21, 1, 22, 2, 0, 1, 2, 2, 
	1, 2, 3, 1, 2, 3, 4, 2, 
	10, 1, 2, 12, 1, 2, 13, 1, 
	2, 14, 1, 2, 15, 1, 2, 16, 
	1, 2, 17, 1, 2, 18, 1, 3, 
	0, 5, 1, 3, 0, 6, 1, 3, 
	0, 7, 1, 3, 0, 8, 1, 3, 
	0, 9, 1, 3, 0, 11, 1
};

static const unsigned char _gdb_key_offsets[] = {
	0, 0, 10, 11, 17, 23, 30, 37, 
	38, 45, 53, 60, 68, 75, 83, 88, 
	90, 92, 94, 96, 98, 100, 102, 104, 
	111, 113, 115, 117, 119, 121, 123, 125, 
	127, 129, 130, 132, 134, 136, 138, 140, 
	142, 144, 146, 148, 150, 152, 154, 156, 
	158, 161, 164, 165, 166
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
	70, 97, 102, 35, 44, 48, 57, 65, 
	70, 97, 102, 35, 65, 67, 83, 102, 
	35, 116, 35, 116, 35, 97, 35, 99, 
	35, 104, 35, 101, 35, 100, 35, 58, 
	35, 48, 57, 65, 70, 97, 102, 35, 
	117, 35, 112, 35, 112, 35, 111, 35, 
	114, 35, 116, 35, 101, 35, 100, 35, 
	58, 35, 35, 84, 35, 104, 35, 114, 
	35, 101, 35, 97, 35, 100, 35, 73, 
	35, 110, 35, 102, 35, 111, 35, 67, 
	35, 111, 35, 110, 35, 116, 35, 59, 
	63, 35, 99, 115, 35, 35, 3, 36, 
	43, 45, 0
};

static const char _gdb_single_lengths[] = {
	0, 10, 1, 0, 0, 1, 1, 1, 
	1, 2, 1, 2, 1, 2, 5, 2, 
	2, 2, 2, 2, 2, 2, 2, 1, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	2, 1, 2, 2, 2, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	3, 3, 1, 1, 4
};

static const char _gdb_range_lengths[] = {
	0, 0, 0, 3, 3, 3, 3, 0, 
	3, 3, 3, 3, 3, 3, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0
};

static const unsigned char _gdb_index_offsets[] = {
	0, 0, 11, 13, 17, 21, 26, 31, 
	33, 38, 44, 49, 55, 60, 66, 72, 
	75, 78, 81, 84, 87, 90, 93, 96, 
	101, 104, 107, 110, 113, 116, 119, 122, 
	125, 128, 130, 133, 136, 139, 142, 145, 
	148, 151, 154, 157, 160, 163, 166, 169, 
	172, 176, 180, 182, 184
};

static const char _gdb_indicies[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 
	9, 10, 0, 12, 11, 13, 13, 13, 
	14, 15, 15, 15, 14, 12, 16, 16, 
	16, 11, 17, 16, 16, 16, 11, 12, 
	18, 12, 19, 19, 19, 11, 12, 20, 
	19, 19, 19, 11, 12, 21, 21, 21, 
	11, 12, 22, 21, 21, 21, 11, 12, 
	23, 23, 23, 11, 12, 22, 23, 23, 
	23, 11, 12, 24, 25, 26, 27, 11, 
	12, 28, 11, 12, 29, 11, 12, 30, 
	11, 12, 31, 11, 12, 32, 11, 12, 
	33, 11, 12, 34, 11, 12, 35, 11, 
	12, 36, 36, 36, 11, 12, 37, 11, 
	12, 38, 11, 12, 39, 11, 12, 40, 
	11, 12, 41, 11, 12, 42, 11, 12, 
	43, 11, 12, 44, 11, 12, 45, 11, 
	47, 46, 12, 48, 11, 12, 49, 11, 
	12, 50, 11, 12, 51, 11, 12, 52, 
	11, 12, 53, 11, 12, 54, 11, 12, 
	55, 11, 12, 56, 11, 12, 57, 11, 
	12, 58, 11, 12, 59, 11, 12, 60, 
	11, 12, 61, 11, 12, 62, 63, 11, 
	12, 64, 65, 11, 12, 66, 12, 67, 
	68, 69, 70, 71, 14, 0
};

static const char _gdb_trans_targs[] = {
	2, 3, 2, 5, 7, 8, 2, 12, 
	5, 14, 44, 2, 3, 4, 0, 52, 
	6, 3, 7, 9, 10, 11, 5, 13, 
	15, 2, 24, 34, 16, 17, 18, 19, 
	20, 21, 22, 23, 23, 25, 26, 27, 
	28, 29, 30, 31, 32, 33, 2, 3, 
	35, 36, 37, 38, 39, 40, 41, 42, 
	43, 2, 45, 46, 47, 48, 49, 2, 
	50, 51, 50, 51, 52, 1, 52, 52
};

static const char _gdb_trans_actions[] = {
	19, 1, 71, 75, 19, 59, 63, 55, 
	67, 19, 19, 3, 0, 7, 0, 28, 
	25, 5, 31, 25, 22, 25, 22, 25, 
	3, 37, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 34, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 40, 9, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	3, 43, 3, 3, 3, 3, 3, 46, 
	3, 3, 49, 52, 11, 13, 15, 17
};

static const int gdb_start = 52;
static const int gdb_first_final = 52;
static const int gdb_error = 0;

static const int gdb_en_main = 52;


#line 124 "src/gdb_parser.rl"

void parser_init(command_cb *callbacks)
{
	
#line 168 "src/gdb_parser_precompile.c"
	{
	cs = gdb_start;
	}

#line 128 "src/gdb_parser.rl"

	command_callbacks = callbacks;
}

int parse_buffer(char *buf, size_t len, void *priv)
{
	char *p = buf;
	char *pe = p + len;

	
#line 184 "src/gdb_parser_precompile.c"
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
#line 13 "src/gdb_parser.rl"
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
#line 22 "src/gdb_parser.rl"
	{
		crc += *p;
	}
	break;
	case 2:
#line 26 "src/gdb_parser.rl"
	{
		data++;
		assert(data < &stack[10]);
	}
	break;
	case 3:
#line 31 "src/gdb_parser.rl"
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
#line 42 "src/gdb_parser.rl"
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
	case 5:
#line 62 "src/gdb_parser.rl"
	{cmd = GET_MEM;}
	break;
	case 6:
#line 67 "src/gdb_parser.rl"
	{cmd = PUT_MEM;}
	break;
	case 7:
#line 74 "src/gdb_parser.rl"
	{cmd = GET_GPRS;}
	break;
	case 8:
#line 76 "src/gdb_parser.rl"
	{cmd = GET_SPR;}
	break;
	case 9:
#line 79 "src/gdb_parser.rl"
	{cmd = STOP_REASON;}
	break;
	case 10:
#line 81 "src/gdb_parser.rl"
	{cmd = SET_THREAD;}
	break;
	case 11:
#line 83 "src/gdb_parser.rl"
	{cmd = DETACH;}
	break;
	case 12:
#line 87 "src/gdb_parser.rl"
	{rsp = "1";}
	break;
	case 13:
#line 88 "src/gdb_parser.rl"
	{rsp = "QC1";}
	break;
	case 14:
#line 89 "src/gdb_parser.rl"
	{rsp = "multiprocess+;vContSupported+";}
	break;
	case 15:
#line 90 "src/gdb_parser.rl"
	{rsp = "m1l";}
	break;
	case 16:
#line 93 "src/gdb_parser.rl"
	{rsp = "vCont;c;C;s;S";}
	break;
	case 17:
#line 94 "src/gdb_parser.rl"
	{cmd = V_CONTC;}
	break;
	case 18:
#line 95 "src/gdb_parser.rl"
	{cmd = V_CONTS;}
	break;
	case 19:
#line 98 "src/gdb_parser.rl"
	{ if (command_callbacks) command_callbacks[INTERRUPT](stack, priv); PR_INFO("RAGEL:interrupt\n");}
	break;
	case 20:
#line 105 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:cmd\n");}
	break;
	case 21:
#line 108 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:ack\n");}
	break;
	case 22:
#line 109 "src/gdb_parser.rl"
	{PR_INFO("RAGEL:nack\n");}
	break;
#line 389 "src/gdb_parser_precompile.c"
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

#line 138 "src/gdb_parser.rl"

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

#!/bin/sh

if [ $# -ne 1 ] ; then
	echo "Usage: $0 <file.dtb>"
	exit 1
fi

SYMBOL=$(echo "$1" | tr '.-' '_')
SYM_START="_binary_${SYMBOL}_o_start"
SYM_END="_biary_${SYMBOL}_o_end"
SYM_SIZE="_biary_${SYMBOL}_o_size"
HEADER="$f.h"

cat - <<EOF
/* This file is auto-generated from generate_dt_header.sh */
#ifndef __${SYMBOL}_H__
#define __${SYMBOL}_H__

extern unsigned char ${SYM_START};
extern unsigned char ${SYM_END};
extern long ${SYM_SIZE};

#endif /* __${SYMBOL}_H__ */
EOF

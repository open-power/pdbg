#!/bin/sh

if [ $# -ne 1 ] ; then
	echo "Usage: $0 <file.dts>"
	exit 1
fi

SYMBOL=$(basename "$1" | sed 's/dts/dtb/' | tr '.-' '_')
SYM_START="_binary_${SYMBOL}_o_start"
SYM_END="_binary_${SYMBOL}_o_end"
SYM_SIZE="_binary_${SYMBOL}_o_size"

cat - <<EOF
/* This file is auto-generated from generate_dt_header.sh */
#ifndef __${SYMBOL}_H__
#define __${SYMBOL}_H__

extern unsigned char ${SYM_START};
extern unsigned char ${SYM_END};
extern long ${SYM_SIZE};

#endif /* __${SYMBOL}_H__ */
EOF

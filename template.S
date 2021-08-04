// On platforms that define this macro, a symbol called "foo" in C must be
// named "_foo" in assembler. GCC Should always define the macro, but
// we add a fallback just in case
#ifndef __USER_LABEL_PREFIX__
#define __USER_LABEL_PREFIX__ ""
#endif

#define CONCAT1(a, b, c, d) CONCAT2(a, b, c, d)
#define CONCAT2(a, b, c, d) a ## b ## c ## d

#define SYM_START(x)    CONCAT1(__USER_LABEL_PREFIX__, _binary_, x, _start)

.section .data
.align 8
SYM_START(SYMBOL_PREFIX):
.incbin FILENAME
.globl SYM_START(SYMBOL_PREFIX)

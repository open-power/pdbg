#ifdef __USER_LABEL_PREFIX__
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)
#else
#define SYM(x) x
#endif

#define SYM_START(x)	CONCAT1(CONCAT1(_binary_, SYM(x)), _start)
#define SYM_END(x)	CONCAT1(CONCAT1(_binary_, SYM(x)), _end)
#define SYM_SIZE(x)	CONCAT1(CONCAT1(_binary_, SYM(x)), _size)

.section .data
SYM_START(SYMBOL_PREFIX):
.incbin FILENAME
.align 4
SYM_END(SYMBOL_PREFIX):
SYM_SIZE(SYMBOL_PREFIX):
	.long	SYM_END(SYMBOL_PREFIX) - SYM_START(SYMBOL_PREFIX)
.globl SYM_START(SYMBOL_PREFIX)
.globl SYM_END(SYMBOL_PREFIX)
.globl SYM_SIZE(SYMBOL_PREFIX)

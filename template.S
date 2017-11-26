#ifdef __USER_LABEL_PREFIX__
#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (__USER_LABEL_PREFIX__, x)
#else
#define SYM(x) x
#endif

.section .data
SYM(_binary_SYMBOL_PREFIX_start):
.incbin "FILENAME"
.align 4
SYM(_binary_SYMBOL_PREFIX_end):
SYM(_binary_SYMBOL_PREFIX_size):
	.long	SYM(_binary_SYMBOL_PREFIX_end) - SYM(_binary_SYMBOL_PREFIX_start)
.globl SYM(_binary_SYMBOL_PREFIX_start)
.globl SYM(_binary_SYMBOL_PREFIX_end)
.globl SYM(_binary_SYMBOL_PREFIX_size)

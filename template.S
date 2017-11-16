.section .data
_binary_SYMBOL_PREFIX_start:
.incbin "FILENAME"
.align 4
_binary_SYMBOL_PREFIX_end:
_binary_SYMBOL_PREFIX_size:
	.long	_binary_SYMBOL_PREFIX_end - _binary_SYMBOL_PREFIX_start
.globl _binary_SYMBOL_PREFIX_start
.globl _binary_SYMBOL_PREFIX_end
.globl _binary_SYMBOL_PREFIX_size

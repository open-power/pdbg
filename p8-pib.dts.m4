define(`CONCAT', `$1$2')dnl
define(`HEX', `CONCAT(0x, $1)')dnl
define(`CORE_BASE', `eval(0x10000000 + $1 * 0x1000000, 16)')dnl
define(`CORE', `core@CORE_BASE($1) {
	#address-cells = <0x2>;
	#size-cells = <0x1>;
	compatible = "ibm,power8-core";
	reg = <0x0 HEX(CORE_BASE($1)) 0xfffff>;
	index = <0x$2>;

	THREAD(0);
	THREAD(1);
	THREAD(2);
	THREAD(3);
	THREAD(4);
	THREAD(5);
	THREAD(6);
	THREAD(7);
}')dnl
define(`THREAD_BASE', `eval(0x13000 + $1 * 0x10, 16)')dnl
define(`THREAD',`thread@THREAD_BASE($1) {
	reg = <0x0 HEX(THREAD_BASE($1)) 0x10>;
	compatible = "ibm,power8-thread";
	index = <HEX(eval($1, 16))>;
	}')dnl
dnl
define(`PROC_CORES', `CORE(1, 0);
CORE(2, 1);
CORE(3, 2);
CORE(4, 3);
CORE(5, 4);
CORE(6, 5);
CORE(9, 6);
CORE(10, 7);
CORE(11, 8);
CORE(12, 9);
CORE(13, 10);
CORE(14, 11)')dnl

adu@2020000 {
	compatible = "ibm,power8-adu";
	reg = <0x0 0x2020000 0x4>;
};

PROC_CORES;

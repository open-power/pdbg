define(`CONCAT', `$1$2')dnl
define(`HEX', `CONCAT(0x, $1)')dnl
define(`CORE_BASE', `eval(0x10000000 + $1 * 0x1000000, 16)')dnl
define(`CORE', `core@CORE_BASE($1) {
	#address-cells = <0x2>;
	#size-cells = <0x1>;
	compatible = "ibm,power8-core";
	reg = <0x0 HEX(CORE_BASE($1)) 0xfffff>;
	index = <HEX(eval($2, 16))>;
	chtm@11000 {
		compatible = "ibm,power8-chtm";
		reg = <0x0 0x11000 0xB>;
		index = <0x0>;
	};

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
define(`PROC_CORES', `CORE(1, 1);
CORE(2, 2);
CORE(3, 3);
CORE(4, 4);
CORE(5, 5);
CORE(6, 6);
CORE(9, 9);
CORE(10, 10);
CORE(11, 11);
CORE(12, 12);
CORE(13, 13);
CORE(14, 14)')dnl

adu@2020000 {
	compatible = "ibm,power8-adu";
	reg = <0x0 0x2020000 0x4>;
};

nhtm@2010880 {
	compatible = "ibm,power8-nhtm";
	reg = <0x0 0x2010880 0x8>;
	index = <0x0>;
};

PROC_CORES;

define(`CONCAT', `$1$2')dnl
define(`HEX', `CONCAT(0x, $1)')dnl
define(`CORE_BASE', `eval(0x20000000 + $1 * 0x1000000, 16)')dnl
define(`CORE', `chiplet@CORE_BASE($1) {
#address-cells = <0x2>;
#size-cells = <0x1>;
compatible = "ibm,power9-chiplet";
index = <HEX(eval($2, 16))>;
reg = <0x0 HEX(CORE_BASE($1)) 0xfffff>;

core@0 {
       #address-cells = <0x1>;
       #size-cells = <0x0>;
       compatible = "ibm,power9-core";
       index = <HEX(eval($2, 16))>;
       reg = <0x0 0x0 0xfffff>;

       THREAD(0);
       THREAD(1);
       THREAD(2);
       THREAD(3);
};
}')dnl
define(`THREAD_BASE', `eval($1, 16)')dnl
define(`THREAD',`thread@THREAD_BASE($1) {
compatible = "ibm,power9-thread";
reg = <0x0>;
tid = <HEX(eval($1, 16))>;
index = <HEX(eval($1, 16))>;
}')dnl
define(`CHIPLET_BASE', `eval(0x1000000 * $1, 16)')dnl
define(`CHIPLET', `chiplet@CHIPLET_BASE($1) {
compatible = "ibm,power9-chiplet";
index = <HEX(eval($1, 16))>;
reg = <0x0 HEX(CHIPLET_BASE($1)) 0xfffff>;
}')dnl


adu@90000 {
	  compatible = "ibm,power9-adu";
	  reg = <0x0 0x90000 0x5>;
};

htm@5012880 {
	compatible = "ibm,power9-nhtm";
	reg = <0x0 0x5012880 0x40>;
	index = <0x0>;
};

htm@50128C0 {
	compatible = "ibm,power9-nhtm";
	reg = <0x0 0x50128C0 0x40>;
	index = <0x1>;
};

CORE(0, 0);
CORE(1, 1);
CORE(2, 2);
CORE(3, 3);
CORE(4, 4);
CORE(5, 5);
CORE(6, 6);
CORE(7, 7);
CORE(8, 8);
CORE(9, 9);
CORE(10, 10);
CORE(11, 11);
CORE(12, 12);
CORE(13, 13);
CORE(14, 14);
CORE(15, 15);
CORE(16, 16);
CORE(17, 17);
CORE(18, 18);
CORE(19, 19);
CORE(20, 20);
CORE(21, 21);
CORE(22, 22);
CORE(23, 23);

CHIPLET(1);
CHIPLET(2);
CHIPLET(3);
CHIPLET(4);
CHIPLET(5);
CHIPLET(6);
CHIPLET(7);
CHIPLET(8);
CHIPLET(9);
CHIPLET(12);
CHIPLET(13);
CHIPLET(14);
CHIPLET(15);
CHIPLET(16);
CHIPLET(17);
CHIPLET(18);
CHIPLET(19);
CHIPLET(20);
CHIPLET(21);

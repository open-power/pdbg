dnl
dnl CHIP([index])
dnl
define(`CHIP',
`
       fsi$1 {
		compatible = "ibm,cronus-fsi";
		index = <0x$1>;
		system-path = "/proc$1/fsi";
	};

	pib$1 {
		compatible = "ibm,cronus-pib";
		index = <0x$1>;
		system-path = "/proc$1/pib";
	};

	sbefifo$1 {
		compatible = "ibm,cronus-sbefifo";
		index = <0x$1>;

		sbefifo-chipop {
			compatible = "ibm,sbefifo-chipop";
			index = <0x$1>;
		};

		sbefifo-mem {
			compatible = "ibm,sbefifo-mem";
			index = <0x$1>;
			system-path = "/mem$1";
		};

		sbefifo-pba {
			compatible = "ibm,sbefifo-mem-pba";
			index = <0x$1>;
			system-path = "/mempba$1";
		};
	};
')dnl

/dts-v1/;

/ {
	CHIP(0)
	CHIP(1)
	CHIP(2)
	CHIP(3)
	CHIP(4)
	CHIP(5)
	CHIP(6)
	CHIP(7)
};

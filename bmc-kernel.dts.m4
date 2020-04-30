dnl
dnl PIB([addr], [index], [path-index])
dnl
define(`PIB',
`
	pib@$1 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		reg = <0x0 0x$1 0x7>;
		compatible = "ibm,kernel-pib";
		index = <0x$2>;
		device-path = "/dev/scom$3";
		system-path = "/proc$2/pib";
	};
')dnl

dnl
dnl SBEFIFO([index], [path-index])
dnl
define(`SBEFIFO',
`
	sbefifo@2400 { /* Bogus address */
		reg = <0x0 0x2400 0x7>;
		compatible = "ibm,kernel-sbefifo";
		index = <0x$1>;
		device-path = "/dev/sbefifo$2";

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

		sbefifo-chipop {
			compatible = "ibm,sbefifo-chipop";
			index = <0x$1>;
		};
	};
')dnl

dnl
dnl FSI_PRE([addr], [index], [path-index])
dnl
define(`FSI_PRE',
`
	fsi@$1 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		reg = <0x0 0x$1 0x8000>;
		compatible = "ibm,kernel-fsi";
		index = <0x$2>;
		system-path = "/proc$2/fsi";
		status = "mustexist";

		PIB(1000, $2, $3)
		SBEFIFO($2, $3)
')dnl

dnl
dnl FSI_POST()
dnl
define(`FSI_POST',
`
	};
')dnl

dnl
dnl HMFSI([addr], [port], [index], [path-index])
dnl
define(`HMFSI',
`
	hmfsi@$1 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		reg = <0x0 0x$1 0x8000>;
		compatible = "ibm,fsi-hmfsi";
		port = <0x$2>;
		index = <0x$3>;
		system-path = "/proc$3/fsi";

		PIB(1000, $3, $4)
		SBEFIFO($3, $4)
	};
')dnl


/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	FSI_PRE(0, 0, 1)

	HMFSI(100000, 1, 1, 2)
	HMFSI(180000, 2, 2, 3)
	HMFSI(200000, 3, 3, 4)
	HMFSI(280000, 4, 4, 5)
	HMFSI(300000, 5, 5, 6)
	HMFSI(380000, 6, 6, 7)
	HMFSI(400000, 7, 7, 8)

	FSI_POST()
};

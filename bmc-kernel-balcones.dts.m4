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
dnl PIB_ODY([index], [proc], [path-index], port)
dnl
define(`PIB_ODY',
`
	pib_ody@$3$4 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		reg = <0x0 0x$1 0x8000>; /*dummy to fix dts warning*/
		compatible = "ibm,kernel-pib-ody";
		index = <0x$1>;
		proc = <0x$2>;
		port = <$4>;
		device-path = "/dev/scom$3$4";
		system-path = "/proc$2/ocmb$1";
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
		device-path = "/fsi0/slave@00:00/raw";
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
dnl BMC_I2CBUS([index])
dnl
define(`BMC_I2CBUS',
`
	bmc-i2c-bus$1 {
		#address-cells = <0x1>;
		#size-cells = <0x0>;
		index = <$1>;
		compatible = "ibm,kernel-i2c-bus";
		device-path = "/dev/i2c-$1";
		system-path = "/bmc0/i2c-$1";
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

//ody ocmb chips are defined in system device tree. The pdbg targets
//that captures the device path to communicate with system ody ocmb
//chips will be defined in backend device tree.

//ody ocmb system device tree targets need to be mapped to backend
//ody pib device tree targets for communication with the ody ocmb targets.
//Mapping is done based on proc, ocmb chip index of the ody ocmb system target
//with the proc, ocmb index and port number defined in the backend kernel device
//tree

//for get or put scom in kernel mode device path defined in PIBODY will be used
//for cfam device-path specified in HMFSIODY will be used
dnl
dnl HMFSI_ODY([index], [proc], [path-index], [port])
dnl
define(`HMFSI_ODY',
`
	hmfsi-ody@$3$4 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,kernel-fsi-ody";
		device-path = "/i2cr$3$4/slave@00:00/raw";
		reg = <0x0 0x$1 0x8000>; /*dummy to fix dts warning*/
		index = <0x$1>;
		proc = <0x$2>;
		port = <$4>;
		system-path = "/proc$2/ocmb$1/fsi";

		PIB_ODY($1, $2, $3, $4)
		/*SBE_FIFO not required in kernel mode */
	};
')dnl

/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	BMC_I2CBUS(0)
	BMC_I2CBUS(1)
	BMC_I2CBUS(2)
	BMC_I2CBUS(3)
	BMC_I2CBUS(4)
	BMC_I2CBUS(5)
	BMC_I2CBUS(6)
	BMC_I2CBUS(7)
	BMC_I2CBUS(8)
	BMC_I2CBUS(9)
	BMC_I2CBUS(10)
	BMC_I2CBUS(11)
	BMC_I2CBUS(12)
	BMC_I2CBUS(13)
	BMC_I2CBUS(14)
	BMC_I2CBUS(15)

	FSI_PRE(0, 0, 1)

	HMFSI(100000, 1, 1, 2)
	HMFSI(180000, 2, 2, 3)
	HMFSI(200000, 3, 3, 4)
	HMFSI(280000, 4, 4, 5)
	HMFSI(300000, 5, 5, 6)
	HMFSI(380000, 6, 6, 7)
	HMFSI(400000, 7, 7, 8)

	FSI_POST()

	HMFSI_ODY(0, 0, 1, 11)
	HMFSI_ODY(1, 0, 1, 10)
	HMFSI_ODY(2, 0, 1, 12)
	HMFSI_ODY(3, 0, 1, 13)
	HMFSI_ODY(4, 0, 1, 15)
	HMFSI_ODY(5, 0, 1, 00)
	HMFSI_ODY(6, 0, 1, 14)
	HMFSI_ODY(7, 0, 1, 01)

};

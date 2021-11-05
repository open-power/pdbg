define(`PROC',`
define(`PROC_ID',`$1')dnl
	proc$1 {
		compatible = "ibm,power-proc", "ibm,power8-proc";
		index = <$1>;

		fsi {
			index = <$1>;
		};

		pib {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			index = <$1>;
			include(p8-pib.dts.m4)dnl
		};
	};
	mem$1 {
		index = <$1>;
	}'
)dnl

dnl
dnl I2CBUS([index])
dnl
define(`I2CBUS', `i2c-$1 {
}')dnl

/dts-v1/;

/ {
	bmc0 {
		I2CBUS(0);
		I2CBUS(1);
		I2CBUS(2);
		I2CBUS(3);
		I2CBUS(4);
		I2CBUS(5);
		I2CBUS(6);
		I2CBUS(7);
	};

	PROC(0);
	PROC(1);
	PROC(2);
	PROC(3);
	PROC(4);
	PROC(5);
	PROC(6);
	PROC(7);
	PROC(8);
	PROC(9);
	PROC(10);
	PROC(11);
	PROC(12);
	PROC(13);
	PROC(14);
	PROC(15);
	PROC(16);
	PROC(17);
	PROC(18);
	PROC(19);
	PROC(20);
	PROC(21);
	PROC(22);
	PROC(23);
	PROC(24);
	PROC(25);
	PROC(26);
	PROC(27);
	PROC(28);
	PROC(29);
	PROC(30);
	PROC(31);
};

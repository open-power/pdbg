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

	fsi0: kernelfsi@0 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,kernel-fsi";
		device-path = "/fsi0/slave@00:00/raw";
		reg = <0x0 0x0 0x0>;
		index = <0x0>;
		status = "mustexist";
		system-path = "/proc0/fsi";

		pib@1000 {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			reg = <0x0 0x1000 0x7>;
			index = <0x0>;
			compatible = "ibm,fsi-pib";
			system-path = "/proc0/pib";
		};

		hmfsi@100000 {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			compatible = "ibm,fsi-hmfsi";
			reg = <0x0 0x100000 0x8000>;
			port = <0x1>;
			index = <0x1>;
			system-path = "/proc1/fsi";

			pib@1000 {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				reg = <0x0 0x1000 0x7>;
				compatible = "ibm,fsi-pib";
				index = <0x1>;
				system-path = "/proc1/pib";
			};

		};
       };
};


/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	/* I2C attached pib */
	pib@50 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,power8-i2c-slave";
		bus = "/dev/i2c4";
		reg = <0x50>;
		index = <0x0>;
		status = "mustexist";
		system-path = "/proc0/pib";

		opb@20010 {
			#address-cells = <0x1>;
			#size-cells = <0x1>;
			reg = <0x0 0x20010 0xa>;
			compatible = "ibm,power8-opb";

			hmfsi@100000 {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				compatible = "ibm,power8-opb-hmfsi";
				reg = <0x100000 0x80000>;
				port = <0x1>;
				index = <0x1>;
				system-path = "/proc1/fsi";

				pib@1000 {
					#address-cells = <0x2>;
					#size-cells = <0x1>;
					reg = <0x0 0x1000 0x7>;
					compatible = "ibm,fsi-pib", "ibm,power8-fsi-pib";
					index = <0x1>;
					system-path = "/proc1/pib";
				};
			};

			hmfsi@180000 {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				compatible = "ibm,power8-opb-hmfsi";
				reg = <0x180000 0x80000>;
				port = <0x2>;
				index = <0x2>;
				system-path = "/proc2/fsi";

				pib@1000 {
					#address-cells = <0x2>;
					#size-cells = <0x1>;
					reg = <0x0 0x1000 0x7>;
					compatible = "ibm,fsi-pib", "ibm,power8-fsi-pib";
					index = <0x2>;
					system-path = "/proc2/pib";
				};
			};
		};
	};
};


/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	fsi0: fsi@0 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,bmcfsi";
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
				index = <0x1>;
				compatible = "ibm,fsi-pib";
				system-path = "/proc1/pib";
			};
		};

	};
};

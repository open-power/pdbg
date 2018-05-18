/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	fsi@0 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,fake-fsi";
		reg = <0x0 0x0 0x0>;

		index = <0x0>;
		status = "mustexist";

		pib@0 {
			compatible = "ibm,fake-pib";
			reg = <0x0 0x0 0x0>;
			index = <0x0>;
		};
	};
};

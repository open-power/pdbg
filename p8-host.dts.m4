/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	/* Host based debugfs access */
	pib@0 {
	      #address-cells = <0x2>;
	      #size-cells = <0x1>;
	      compatible = "ibm,host-pib";
	      chip-id = <0x0>;
	      index = <0x0>;
	      include(p8-pib.dts.m4)dnl
	};

	pib@8 {
	      #address-cells = <0x2>;
	      #size-cells = <0x1>;
	      compatible = "ibm,host-pib";
	      chip-id = <0x8>;
	      index = <0x1>;
	      include(p8-pib.dts.m4)dnl
	};
};

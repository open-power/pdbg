/dts-v1/;

/ {
       #address-cells = <0x1>;
       #size-cells = <0x0>;

       fsi0: kernelfsi@0 {
	       #address-cells = <0x2>;
	       #size-cells = <0x1>;
	       compatible = "ibm,cronus-fsi";
	       reg = <0x0 0x0 0x0>;

	       index = <0x0>;
	       status = "mustexist";

	       pib@1000 {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			reg = <0x0 0x1000 0x7>;
			index = <0x0>;
			compatible = "ibm,cronus-pib";
			include(p8-pib.dts.m4)dnl
	       };

	       hmfsi@100000 {
		       #address-cells = <0x2>;
		       #size-cells = <0x1>;
		       compatible = "ibm,cronus-hmfsi";
		       reg = <0x0 0x100000 0x8000>;
		       port = <0x1>;
		       index = <0x1>;

		       pib@1000 {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				reg = <0x0 0x1000 0x7>;
				compatible = "ibm,cronus-pib";
				index = <0x1>;
				include(p8-pib.dts.m4)dnl
		       };

	       };
       };
};


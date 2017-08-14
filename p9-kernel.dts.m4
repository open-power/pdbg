/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	fsi0: kernelfsi@0 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,kernel-fsi";
		reg = <0x0 0x0 0x0>;

		index = <0x0>;
		status = "hidden";

		pib@1000 {
			reg = <0x0 0x1000 0x7>;
			index = <0x0>;
			compatible = "ibm,fsi-pib", "ibm,power9-fsi-pib";
			include(p9-pib.dts.m4)dnl
		};

		hmfsi@100000 {
			compatible = "ibm,fsi-hmfsi";
			reg = <0x0 0x100000 0x8000>;
			port = <0x1>;
			index = <0x1>;

			pib@1000 {
				 reg = <0x0 0x1000 0x7>;
				 index = <0x1>;
				 compatible = "ibm,fsi-pib", "ibm,power9-fsi-pib";
				 include(p9-pib.dts.m4)dnl
			};
		};
	};
};

define(`CONCAT', `$1$2')dnl
define(`HEX', `CONCAT(0x, $1)')dnl
define(`CORE_BASE', `eval(0x20000000 + $1 * 0x1000000, 16)')dnl
define(`CORE', `core@CORE_BASE($1) {
#address-cells = <0x1>;
#size-cells = <0x0>;
compatible = "ibm,power9-core";
reg = <0x0 HEX(CORE_BASE($1)) 0xfffff>;
index = <HEX(eval($2, 16))>;

THREAD(0);
THREAD(1);
THREAD(2);
THREAD(3);
}')dnl
define(`THREAD_BASE', `eval($1, 16)')dnl
define(`THREAD',`thread@THREAD_BASE($1) {
compatible = "ibm,power9-thread";
reg = <0x0>;
tid = <HEX(eval($1, 16))>;
index = <HEX(eval($1, 16))>;
}')dnl

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	fsi0: fsi@0 {
		#address-cells = <0x2>;
		#size-cells = <0x1>;
		compatible = "ibm,bmcfsi";
		reg = <0x0 0x0 0x0>;

		index = <0x0>;
		status = "hidden";

		pib@1000 {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			reg = <0x0 0x1000 0x7>;
			compatible = "ibm,fsi-pib", "ibm,power9-fsi-pib";
			index = <0x0>;

			adu@90000 {
				compatible = "ibm,power9-adu";
				reg = <0x0 0x90000 0x5>;
			};

			CORE(0, 0);
			CORE(1, 1);
			CORE(2, 2);
			CORE(3, 3);
			CORE(4, 4);
			CORE(5, 5);
			CORE(6, 6);
			CORE(7, 7);
			CORE(8, 8);
			CORE(9, 9);
			CORE(10, 10);
			CORE(11, 11);
			CORE(12, 12);
			CORE(13, 13);
			CORE(14, 14);
			CORE(15, 15);
			CORE(16, 16);
			CORE(17, 17);
			CORE(18, 18);
			CORE(19, 19);
			CORE(20, 20);
			CORE(21, 21);
			CORE(22, 22);
			CORE(23, 23);
		};

		hmfsi@100000 {
			compatible = "ibm,fsi-hmfsi";
			reg = <0x0 0x100000 0x8000>;
			index = <0x1>;

			pib@1000 {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				reg = <0x0 0x1000 0x7>;
				compatible = "ibm,fsi-pib", "ibm,power9-fsi-pib";
				index = <0x1>;

				adu@90000 {
					compatible = "ibm,power9-adu";
					reg = <0x0 0x90000 0x5>;
				};

				CORE(0, 0);
				CORE(1, 1);
				CORE(2, 2);
				CORE(3, 3);
				CORE(4, 4);
				CORE(5, 5);
				CORE(6, 6);
				CORE(7, 7);
				CORE(8, 8);
				CORE(9, 9);
				CORE(10, 10);
				CORE(11, 11);
				CORE(12, 12);
				CORE(13, 13);
				CORE(14, 14);
				CORE(15, 15);
				CORE(16, 16);
				CORE(17, 17);
				CORE(18, 18);
				CORE(19, 19);
				CORE(20, 20);
				CORE(21, 21);
				CORE(22, 22);
				CORE(23, 23);
			 };
		};

	};
};

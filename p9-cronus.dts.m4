/dts-v1/;

/ {
       fsi0 {
		compatible = "ibm,cronus-fsi";
		index = <0x0>;
		system-path = "/proc0/fsi";
	};

	pib0 {
		compatible = "ibm,cronus-pib";
		index = <0x0>;
		system-path = "/proc0/pib";
	};

	fsi1 {
		compatible = "ibm,cronus-fsi";
		index = <0x1>;
		system-path = "/proc1/fsi";
	};

	pib1 {
		compatible = "ibm,cronus-pib";
		index = <0x1>;
		system-path = "/proc1/pib";
	};
};

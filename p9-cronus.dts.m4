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

	sbefifo0 {
		index = <0x0>;
		compatible = "ibm,cronus-sbefifo";

		sbefifo-chipop {
			compatible = "ibm,sbefifo-chipop";
			index = <0x0>;
		};

		sbefifo-mem {
			compatible = "ibm,sbefifo-mem";
			system-path = "/mem0";
		};

		sbefifo-pba {
			compatible = "ibm,sbefifo-mem-pba";
			system-path = "/mempba0";
		};
	};

	sbefifo1 {
		index = <0x1>;
		compatible = "ibm,cronus-sbefifo";

		sbefifo-chipop {
			compatible = "ibm,sbefifo-chipop";
			index = <0x1>;
		};

		sbefifo-mem {
			compatible = "ibm,sbefifo-mem";
			system-path = "/mem1";
		};

		sbefifo-pba {
			compatible = "ibm,sbefifo-mem-pba";
			system-path = "/mempba1";
		};
	};
};

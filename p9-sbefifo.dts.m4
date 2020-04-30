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
		status = "mustexist";
		system-path = "/proc0/fsi";

		sbefifo@2400 { /* Bogus address */
			reg = <0x0 0x2400 0x7>;
			index = <0x0>;
			compatible = "ibm,kernel-sbefifo";
			device-path = "/dev/sbefifo1";

			sbefifo-pib {
				#address-cells = <0x2>;
				#size-cells = <0x1>;
				index = <0x0>;
				compatible = "ibm,sbefifo-pib";
				system-path = "/proc0/pib";
			};

			sbefifo-mem {
				compatible = "ibm,sbefifo-mem";
				system-path = "/mem0";
			};

			sbefifo-pba {
				compatible = "ibm,sbefifo-mem-pba";
				system-path = "/mempba0";
			};

			sbefifo-chipop {
				compatible = "ibm,sbefifo-chipop";
				index = <0x0>;
			};
		};

		hmfsi@100000 {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			compatible = "ibm,fsi-hmfsi";
			reg = <0x0 0x100000 0x8000>;
			port = <0x1>;
			index = <0x1>;
			system-path = "/proc1/fsi";

			sbefifo@2400 { /* Bogus address */
				reg = <0x0 0x2400 0x7>;
				index = <0x1>;
				compatible = "ibm,kernel-sbefifo";
				device-path = "/dev/sbefifo2";

				sbefifo-pib {
					#address-cells = <0x2>;
					#size-cells = <0x1>;
					index = <0x1>;
					compatible = "ibm,sbefifo-pib";
					system-path = "/proc1/pib";
				};

				sbefifo-mem {
					compatible = "ibm,sbefifo-mem";
					system-path = "/mem1";
				};

				sbefifo-pba {
					compatible = "ibm,sbefifo-mem-pba";
					system-path = "/mempba1";
				};

				sbefifo-chipop {
					compatible = "ibm,sbefifo-chipop";
					index = <0x1>;
				};
			};
		};
	};
};

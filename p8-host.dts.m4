define(`CHIP',`pib@$1 {
	      #address-cells = <0x2>;
	      #size-cells = <0x1>;
	      compatible = "ibm,host-pib";
	      reg = <$1>;
	      chip-id = <$1>;
	      index = <$1>;
	      include(p8-pib.dts.m4)dnl
	}')dnl

/dts-v1/;

/ {
	#address-cells = <0x1>;
	#size-cells = <0x0>;

	CHIP(0);
	CHIP(1);
	CHIP(2);
	CHIP(3);
	CHIP(4);
	CHIP(5);
	CHIP(6);
	CHIP(7);
	CHIP(8);
	CHIP(9);
	CHIP(10);
	CHIP(11);
	CHIP(12);
	CHIP(13);
	CHIP(14);
	CHIP(15);
	CHIP(16);
	CHIP(17);
	CHIP(18);
	CHIP(19);
	CHIP(20);
	CHIP(21);
	CHIP(22);
	CHIP(23);
	CHIP(24);
	CHIP(25);
	CHIP(26);
	CHIP(27);
	CHIP(28);
	CHIP(29);
	CHIP(30);
	CHIP(31);
};

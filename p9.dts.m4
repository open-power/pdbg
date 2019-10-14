define(`PROC',`
define(`PROC_ID',`$1')dnl
	proc$1 {
		index = <$1>;

		fsi {
			index = <$1>;
		};

		pib {
			#address-cells = <0x2>;
			#size-cells = <0x1>;
			index = <$1>;
			include(p9-pib.dts.m4)dnl
		};
	};
	mem$1 {
		index = <$1>;
	}'
)dnl

/dts-v1/;

/ {
	PROC(0);
	PROC(1);
};

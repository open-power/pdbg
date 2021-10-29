define(`CONCAT', `$1$2')dnl

dnl
dnl CORE([index])
dnl
define(`THREAD',
`
	thread@$1 {
		reg = <0x00>;
		compatible = "ibm,power-thread", "ibm,power9-thread";
		index = <0x$1>;
	};
')dnl

dnl
dnl CORE([index])
dnl
define(`CORE',
`
	core@0 {
		#address-cells = <0x01>;
		#size-cells = <0x00>;
		reg = <0x00 0x00 0xfffff>;
		compatible = "ibm,power-core", "ibm,power9-core";
		index = <0x$1>;

		THREAD(0)
		THREAD(1)
		THREAD(2)
		THREAD(3)
	};
')dnl

dnl
dnl CHIPLET__([index])
dnl
define(`CHIPLET__',
`define(`addr', CONCAT($1, 000000))dnl

	CONCAT(chiplet@, addr) {
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power9-chiplet";
		index = <0x$1>;

')dnl

dnl
dnl CHIPLET_([index])
dnl
define(`CHIPLET_',
`define(`addr', CONCAT($1, 000000))dnl

	CONCAT(chiplet@, addr) {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power9-chiplet";
		index = <0x$1>;

')dnl

dnl
dnl EQ_([index])
dnl
define(`EQ_',
`define(`chiplet_id', CONCAT(1, $1))dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	eq@$1 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power9-eq";
		index = <$1>;

')dnl

dnl
dnl EX_([eq_index, ex_index])
dnl
define(`EX_',
`define(`chiplet_id', CONCAT(1, $1))dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	ex@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power9-ex";
		index = <$2>;

')dnl

dnl
dnl CHIP([index])
dnl
define(`CHIP',
`
	mem$1 {
		index = < 0x$1 >;
	};

	proc$1 {
		compatible = "ibm,power-proc", "ibm,power9-proc";
		index = < 0x$1 >;

		fsi {
			index = < 0x$1 >;
		};

		pib {
			#address-cells = < 0x02 >;
			#size-cells = < 0x01 >;
			index = < 0x$1 >;

			adu@90000 {
				compatible = "ibm,power9-adu";
				reg = < 0x00 0x90000 0x50 >;
				system-path = "/mem$1";
			};

			htm@5012880 {
				compatible = "ibm,power9-nhtm";
				reg = < 0x00 0x5012880 0x40 >;
				index = < 0x$1 >;
			};

			htm@50128C0 {
				compatible = "ibm,power9-nhtm";
				reg = < 0x00 0x50128c0 0x40 >;
				index = < 0x$1 >;
			};

			CHIPLET_(1)
				tp@0 {
					reg = < 0x00 0x1000000 0xfffff >;
					compatible = "ibm,power9-tp";
					index = < 0x00 >;
				};
			};

			CHIPLET__(2)
				n0 {
					compatible = "ibm,power9-nest";
					index = < 0x00 >;

					capp0 {
						compatible = "ibm,power9-capp";
						index = < 0x00 >;
					};
				};
			};

			CHIPLET__(3)
				n1 {
					compatible = "ibm,power9-nest";
					index = < 0x01 >;

					mcs2 {
						compatible = "ibm,power9-mcs";
						index = < 0x02 >;
					};

					mcs3 {
						compatible = "ibm,power9-mcs";
						index = < 0x03 >;
					};
				};
			};

			CHIPLET__(4)
				n2 {
					compatible = "ibm,power9-nest";
					index = < 0x02 >;

					capp1 {
						compatible = "ibm,power9-capp";
						index = < 0x01 >;
					};
				};
			};

			CHIPLET__(5)
				n3 {
					compatible = "ibm,power9-nest";
					index = < 0x03 >;

					mcs0 {
						compatible = "ibm,power9-mcs";
						index = < 0x00 >;
					};

					mcs1 {
						compatible = "ibm,power9-mcs";
						index = < 0x01 >;
					};
				};
			};

			CHIPLET_(6)
				xbus$1_0: xbus@0 {
					compatible = "ibm,power9-xbus";
					index = < 0x01 >;
					reg = < 0x00 0x6000000 0xfffff >;
				};
			};

			CHIPLET_(7)
				mc@0 {
					reg = < 0x00 0x7000000 0xfffff >;
					compatible = "ibm,power9-mc";
					index = < 0x00 >;

					mca0 {
						compatible = "ibm,power9-mca";
						index = < 0x00 >;
					};

					mca1 {
						compatible = "ibm,power9-mca";
						index = < 0x01 >;
					};

					mca2 {
						compatible = "ibm,power9-mca";
						index = < 0x02 >;
					};

					mca3 {
						compatible = "ibm,power9-mca";
						index = < 0x03 >;
					};

					mcbist {
						compatible = "ibm,power9-mcbist";
						index = < 0x00 >;
					};
				};
			};

			CHIPLET_(8)
				mc@1 {
					reg = < 0x00 0x8000000 0xfffff >;
					compatible = "ibm,power9-mc";
					index = < 0x01 >;

					mca0 {
						compatible = "ibm,power9-mca";
						index = < 0x04 >;
					};

					mca1 {
						compatible = "ibm,power9-mca";
						index = < 0x05 >;
					};

					mca2 {
						compatible = "ibm,power9-mca";
						index = < 0x06 >;
					};

					mca3 {
						compatible = "ibm,power9-mca";
						index = < 0x07 >;
					};

					mcbist {
						compatible = "ibm,power9-mcbist";
						index = < 0x01 >;
					};
				};
			};

			CHIPLET_(9)
				obus@0 {
					reg = < 0x00 0x9000000 0xfffff >;
					compatible = "ibm,power9-obus";
					index = < 0x00 >;
				};

				obrick0 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x00 >;
				};

				obrick1 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x01 >;
				};

				obrick2 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x02 >;
				};
			};

			CHIPLET_(c)
				obus@3 {
					reg = < 0x00 0xc000000 0xfffff >;
					compatible = "ibm,power9-obus";
					index = < 0x03 >;
				};

				obrick0 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x09 >;
				};

				obrick1 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x0a >;
				};

				obrick2 {
					compatible = "ibm,power9-obus_brick";
					index = < 0x0b >;
				};
			};

			CHIPLET_(d)
				pec@d000000 {
					reg = < 0x00 0xd000000 0xfffff >;
					compatible = "ibm,power9-pec";
					index = < 0x00 >;
				};

				phb0 {
					compatible = "ibm,power9-phb";
					index = < 0x00 >;
				};

				phb1 {
					compatible = "ibm,power9-phb";
					index = < 0x01 >;
				};
			};

			CHIPLET_(e)
				pec@e000000 {
					reg = < 0x00 0xe000000 0xfffff >;
					compatible = "ibm,power9-pec";
					index = < 0x01 >;
				};

				phb0 {
					compatible = "ibm,power9-phb";
					index = < 0x02 >;
				};

				phb1 {
					compatible = "ibm,power9-phb";
					index = < 0x03 >;
				};
			};

			CHIPLET_(f)
				pec@f000000 {
					reg = < 0x00 0xf000000 0xfffff >;
					compatible = "ibm,power9-pec";
					index = < 0x02 >;
				};

				phb0 {
					compatible = "ibm,power9-phb";
					index = < 0x04 >;
				};

				phb1 {
					compatible = "ibm,power9-phb";
					index = < 0x05 >;
				};
			};

			CHIPLET_(10)
				EQ_(0)
					EX_(0,0)
						CHIPLET_(20)
							CORE(00)
						};

						CHIPLET_(21)
							CORE(01)
						};
					};

					EX_(0,1)
						CHIPLET_(22)
							CORE(02)
						};

						CHIPLET_(23)
							CORE(03)
						};
					};
				};
			};

			CHIPLET_(11)
				EQ_(1)
					EX_(1,0)
						CHIPLET_(24)
							CORE(04)
						};

						CHIPLET_(25)
							CORE(05)
						};
					};

					EX_(1,1)
						CHIPLET_(26)
							CORE(06)
						};

						CHIPLET_(27)
							CORE(07)
						};
					};
				};
			};

			CHIPLET_(12)
				EQ_(2)
					EX_(2,0)
						CHIPLET_(28)
							CORE(08)
						};

						CHIPLET_(29)
							CORE(09)
						};
					};

					EX_(2,1)
						CHIPLET_(2a)
							CORE(0a)
						};

						CHIPLET_(2b)
							CORE(0b)
						};
					};
				};
			};

			CHIPLET_(13)
				EQ_(3)
					EX_(3,0)
						CHIPLET_(2c)
							CORE(0c)
						};

						CHIPLET_(2d)
							CORE(0d)
						};
					};

					EX_(3,1)
						CHIPLET_(2e)
							CORE(0e)
						};

						CHIPLET_(2f)
							CORE(0f)
						};
					};
				};
			};

			CHIPLET_(14)
				EQ_(4)
					EX_(4,0)
						CHIPLET_(30)
							CORE(10)
						};

						CHIPLET_(31)
							CORE(11)
						};
					};

					EX_(4,1)
						CHIPLET_(32)
							CORE(12)
						};

						CHIPLET_(33)
							CORE(13)
						};
					};
				};
			};

			CHIPLET_(15)
				EQ_(5)
					EX_(5,0)
						CHIPLET_(34)
							CORE(14)
						};

						CHIPLET_(35)
							CORE(15)
						};
					};

					EX_(5,1)
						CHIPLET_(36)
							CORE(16)
						};

						CHIPLET_(37)
							CORE(17)
						};
					};
				};
			};

			nv0 {
				compatible = "ibm,power9-nv";
				index = < 0x00 >;
			};

			nv1 {
				compatible = "ibm,power9-nv";
				index = < 0x01 >;
			};

			nv2 {
				compatible = "ibm,power9-nv";
				index = < 0x02 >;
			};

			nv3 {
				compatible = "ibm,power9-nv";
				index = < 0x03 >;
			};

			nv4 {
				compatible = "ibm,power9-nv";
				index = < 0x04 >;
			};

			nv5 {
				compatible = "ibm,power9-nv";
				index = < 0x05 >;
			};

			occ0 {
				compatible = "ibm,power9-occ";
				index = < 0x00 >;
			};

			sbe0 {
				compatible = "ibm,power9-sbe";
				index = < 0x00 >;
			};

			ppe0 {
				compatible = "ibm,power9-ppe";
				index = < 0x00 >;
			};

			ppe1 {
				compatible = "ibm,power9-ppe";
				index = < 0x0a >;
			};

			ppe2 {
				compatible = "ibm,power9-ppe";
				index = < 0x0d >;
			};

			ppe3 {
				compatible = "ibm,power9-ppe";
				index = < 0x14 >;
			};

			ppe4 {
				compatible = "ibm,power9-ppe";
				index = < 0x19 >;
			};

			ppe5 {
				compatible = "ibm,power9-ppe";
				index = < 0x1e >;
			};

			ppe6 {
				compatible = "ibm,power9-ppe";
				index = < 0x28 >;
			};

			ppe7 {
				compatible = "ibm,power9-ppe";
				index = < 0x29 >;
			};

			ppe8 {
				compatible = "ibm,power9-ppe";
				index = < 0x2a >;
			};

			ppe9 {
				compatible = "ibm,power9-ppe";
				index = < 0x2b >;
			};

			ppe10 {
				compatible = "ibm,power9-ppe";
				index = < 0x2c >;
			};

			ppe11 {
				compatible = "ibm,power9-ppe";
				index = < 0x2d >;
			};

			ppe12 {
				compatible = "ibm,power9-ppe";
				index = < 0x2e >;
			};

			ppe13 {
				compatible = "ibm,power9-ppe";
				index = < 0x32 >;
			};

			ppe14 {
				compatible = "ibm,power9-ppe";
				index = < 0x34 >;
			};

			ppe15 {
				compatible = "ibm,power9-ppe";
				index = < 0x38 >;
			};
		};
	};
')dnl

dnl
dnl I2CBUS([index])
dnl
define(`I2CBUS', `i2c-$1 {
}')dnl

/dts-v1/;

/ {
	bmc0 {
		I2CBUS(0);
		I2CBUS(1);
		I2CBUS(2);
		I2CBUS(3);
		I2CBUS(4);
		I2CBUS(5);
		I2CBUS(6);
		I2CBUS(7);
		I2CBUS(8);
		I2CBUS(9);
		I2CBUS(10);
		I2CBUS(11);
		I2CBUS(12);
		I2CBUS(13);
		I2CBUS(14);
		I2CBUS(15);
	};

	CHIP(0)
	CHIP(1)
	CHIP(2)
	CHIP(3)
	CHIP(4)
	CHIP(5)
	CHIP(6)
	CHIP(7)
};

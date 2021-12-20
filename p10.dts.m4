define(`CONCAT', `$1$2')dnl

dnl
dnl CORE([index])
dnl
define(`THREAD',
`
	thread@$1 {
		reg = <0x00>;
		compatible = "ibm,power-thread", "ibm,power10-thread";
		index = <0x$1>;
	};
')dnl

dnl
dnl CORE([index])
dnl
define(`CORE',
` define(`id', eval(`0x$1 % 2'))dnl

	CONCAT(core@, id) {
		#address-cells = <0x01>;
		#size-cells = <0x00>;
		reg = <0x00 0x00 0xfffff>;
		compatible = "ibm,power-core", "ibm,power10-core";
		index = <0x$1>;

		THREAD(0)
		THREAD(1)
		THREAD(2)
		THREAD(3)

		htm@20010680 {
			compatible = "ibm,power10-chtm";
			reg = < 0x20010680 >;
			index = < 0x$1 >;
		};

	};
')dnl

dnl
dnl NX([index])
dnl
define(`NX',
`
	nx {
		compatible = "ibm,power10-nx";
		index = <$1>;
	};
')

dnl
dnl CHIPLET__([index])
dnl
define(`CHIPLET__',
`define(`addr', CONCAT($1, 000000))dnl

	CONCAT(chiplet@, addr) {
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-chiplet";
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
		compatible = "ibm,power10-chiplet";
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
		compatible = "ibm,power10-eq";
		index = <$1>;

')dnl

dnl
dnl FC_([index])
dnl
define(`FC_',
`define(`chiplet_id', CONCAT(1, $1))dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	fc@$1 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-fc";
		index = <$1>;

')dnl

dnl
dnl PAUC_([chiplet], [index])
dnl
define(`PAUC_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	pauc@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-pauc";
		index = <$2>;
')dnl

dnl
dnl PAU([chiplet], [index])
dnl
define(`PAU',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	pau@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-pau";
		index = <$2>;
	};
')

dnl
dnl IOHS_([chiplet], [index])
dnl
define(`IOHS_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	iohs@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-iohs";
		index = <$2>;
	};
')

dnl
dnl SMPGROUP([chiplet], [index])
dnl
define(`SMPGROUP',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	smpgroup@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-smpgroup";
		index = <0x$2>;
	};
')

dnl
dnl MI_([chiplet], [index])
dnl
define(`MI_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	mi@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-mi";
		index = <$2>;
')

dnl
dnl MC_([chiplet], [index])
dnl
define(`MC_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	mc@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-mc";
		index = <0x$2>;
')

dnl
dnl MCC_([chiplet], [index])
dnl
define(`MCC_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl
define(`id', eval(`$2 % 2'))dnl

	CONCAT(mcc@, id) {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-mcc";
		index = <$2>;
')

dnl
dnl OMIC([chiplet], [index])
dnl
define(`OMIC',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	omic@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-omic";
		index = <$2>;
	};
')

dnl
dnl OMI_([chiplet], [index])
dnl
define(`OMI_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl
define(`id', eval(`$2 % 2'))dnl

	CONCAT(omi@, id) {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-omi";
		index = <$2>;
')

dnl
dnl OCMB_([index])
dnl
define(`OCMB_',
`
	CONCAT(ocmb@, $1) {
		reg = <0x00 0x08010000 0x3c00>;
		compatible = "ibm,power-ocmb", "ibm,power10-ocmb";
		index = <$1>;
')

dnl
dnl MEM_PORT_([index])
dnl
define(`MEM_PORT_',
`define(`id', eval(`$1 % 2'))dnl

	CONCAT(mem_port, id) {
		compatible = "ibm,power10-memport";
		index = <$1>;
')

dnl
dnl DIMM([index])
dnl
define(`DIMM',
`
	CONCAT(dimm, $1) {
		compatible = "ibm,power10-dimm";
		index = <$1>;
	};
')

dnl
dnl ADC([index])
dnl
define(`ADC',
`define(`id', eval(`$1 % 2'))dnl

	CONCAT(adc, id) {
		compatible = "ibm,power10-adc";
		index = <$1>;
	};
')

dnl
dnl PEC_([chiplet], [index])
dnl
define(`PEC_',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	pec@$2 {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-pec";
		index = <$2>;
')

dnl
dnl PHB([chiplet], [index])
dnl
define(`PHB',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl
define(`id', eval(`$2 % 3'))dnl

	CONCAT(phb@,id) {
		#address-cells = <0x02>;
		#size-cells = <0x01>;
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-phb";
		index = <$2>;
	};
')

dnl
dnl NMMU([chiplet], [index])
dnl
define(`NMMU',
`define(`chiplet_id', $1)dnl
define(`addr', CONCAT(chiplet_id, 000000))dnl

	nmmu@$2 {
		reg = <0x00 CONCAT(0x,addr) 0xfffff>;
		compatible = "ibm,power10-nmmu";
		index = <$2>;
	};
')

dnl
dnl
dnl CHIP([index])
dnl
define(`CHIP',
`
	mem$1 {
		index = < 0x$1 >;
	};

	proc$1 {
		compatible = "ibm,power-proc", "ibm,power10-proc";
		index = < 0x$1 >;

		fsi {
			index = < 0x$1 >;
		};

		pib {
			#address-cells = < 0x02 >;
			#size-cells = < 0x01 >;
			index = < 0x$1 >;

			adu@3001C00 {
				compatible = "ibm,power10-adu";
				reg = < 0x00 0x3001C00 0x50 >;
				system-path = "/mem$1";
			};

			htm@3011C80 {
				compatible = "ibm,power10-nhtm";
				reg = < 0x00 0x3011C80 0x40 >;
				index = < 0x$1 >;
			};

			htm@30120C0 {
				compatible = "ibm,power10-nhtm";
				reg = < 0x00 0x30120C0 0x40 >;
				index = < 0x$1 >;
			};

			NX(0)

			CHIPLET_(1)
				tp@0 {
					reg = < 0x00 0x1000000 0xfffff >;
					compatible = "ibm,power10-tp";
					index = < 0x00 >;
				};
			};

			CHIPLET_(2)
				NMMU(2,0)
			};

			CHIPLET_(3)
				NMMU(3,1)
			};

			CHIPLET_(8)
				PEC_(8,0)
					PHB(8,0)
					PHB(8,1)
					PHB(8,2)
				};
			};

			CHIPLET_(9)
				PEC_(9,1)
					PHB(9,3)
					PHB(9,4)
					PHB(9,5)
				};
			};

			CHIPLET_(c)
				MC_(c,0)
					MI_(c,0)
						MCC_(c,0)
							OMI_(c,0)
								OCMB_(0)
									MEM_PORT_(0)
										DIMM(0)
										ADC(0)
										ADC(1)
									};
								};
							};
							OMI_(c,1)
								OCMB_(1)
									MEM_PORT_(1)
										DIMM(1)
										ADC(2)
										ADC(3)
									};
								};
							};
						};
						MCC_(c,1)
							OMI_(c,2)
								OCMB_(2)
									MEM_PORT_(2)
										DIMM(2)
										ADC(4)
										ADC(5)
									};
								};
							};
							OMI_(c,3)
								OCMB_(3)
									MEM_PORT_(3)
										DIMM(3)
										ADC(6)
										ADC(7)
									};
								};
							};
						};
					};
					OMIC(c,0)
					OMIC(c,1)
				};
			};

			CHIPLET_(d)
				MC_(d,1)
					MI_(d,1)
						MCC_(d,2)
							OMI_(d,4)
								OCMB_(4)
									MEM_PORT_(4)
										DIMM(4)
										ADC(8)
										ADC(9)
									};
								};
							};
							OMI_(d,5)
								OCMB_(5)
									MEM_PORT_(5)
										DIMM(5)
										ADC(10)
										ADC(11)
									};
								};
							};
						};
						MCC_(d,3)
							OMI_(d,6)
								OCMB_(6)
									MEM_PORT_(6)
										DIMM(6)
										ADC(12)
										ADC(13)
									};
								};
							};
							OMI_(d,7)
								OCMB_(7)
									MEM_PORT_(7)
										DIMM(7)
										ADC(14)
										ADC(15)
									};
								};
							};
						};
					};
					OMIC(d,2)
					OMIC(d,3)
				};
			};

			CHIPLET_(e)
				MC_(e,2)
					MI_(e,2)
						MCC_(e,4)
							OMI_(e,8)
							};
							OMI_(e,9)
							};
						};
						MCC_(e,5)
							OMI_(e,10)
							};
							OMI_(e,11)
							};
						};
					};
					OMIC(e,4)
					OMIC(e,5)
				};
			};

			CHIPLET_(f)
				MC_(f,3)
					MI_(f,3)
						MCC_(f,6)
							OMI_(f,12)
							};
							OMI_(f,13)
							};
						};
						MCC_(f,7)
							OMI_(f,14)
							};
							OMI_(f,15)
							};
						};
					};
					OMIC(f,6)
					OMIC(f,7)
				};
			};

			CHIPLET_(10)
				PAUC_(10,0)
					PAU(10,0)
				};
			};

			CHIPLET_(11)
				PAUC_(11,1)
					PAU(11,3)
				};
			};

			CHIPLET_(12)
				PAUC_(12,2)
					PAU(12,4)
					PAU(12,5)
				};
			};

			CHIPLET_(13)
				PAUC_(13,3)
					PAU(13,6)
					PAU(13,7)
				};
			};

			CHIPLET_(18)
				IOHS_(18,0)
					SMPGROUP(18,0)
					SMPGROUP(18,1)
			};

			CHIPLET_(19)
				IOHS_(19,1)
					SMPGROUP(19,2)
					SMPGROUP(19,3)
			};

			CHIPLET_(1a)
				IOHS_(1a,2)
					SMPGROUP(1a,4)
					SMPGROUP(1a,5)
			};

			CHIPLET_(1b)
				IOHS_(1b,3)
					SMPGROUP(1b,6)
					SMPGROUP(1b,7)
			};

			CHIPLET_(1c)
				IOHS_(1c,4)
					SMPGROUP(1c,8)
					SMPGROUP(1c,9)
			};

			CHIPLET_(1d)
				IOHS_(1d,5)
					SMPGROUP(1d,a)
					SMPGROUP(1d,b)
			};

			CHIPLET_(1e)
				IOHS_(1e,6)
					SMPGROUP(1e,c)
					SMPGROUP(1e,d)
			};

			CHIPLET_(1f)
				IOHS_(1f,7)
					SMPGROUP(1f,e)
					SMPGROUP(1f,f)
			};

			CHIPLET_(20)
				EQ_(0)
					FC_(0)
						CORE(0)
						CORE(1)
					};
					FC_(1)
						CORE(2)
						CORE(3)
					};
				};
			};

			CHIPLET_(21)
				EQ_(1)
					FC_(0)
						CORE(4)
						CORE(5)
					};
					FC_(1)
						CORE(6)
						CORE(7)
					};
				};
			};

			CHIPLET_(22)
				EQ_(2)
					FC_(0)
						CORE(8)
						CORE(9)
					};
					FC_(1)
						CORE(a)
						CORE(b)
					};
				};
			};

			CHIPLET_(23)
				EQ_(3)
					FC_(0)
						CORE(c)
						CORE(d)
					};
					FC_(1)
						CORE(e)
						CORE(f)
					};
				};
			};

			CHIPLET_(24)
				EQ_(4)
					FC_(0)
						CORE(10)
						CORE(11)
					};
					FC_(1)
						CORE(12)
						CORE(13)
					};
				};
			};

			CHIPLET_(25)
				EQ_(5)
					FC_(0)
						CORE(14)
						CORE(15)
					};
					FC_(1)
						CORE(16)
						CORE(17)
					};
				};
			};

			CHIPLET_(26)
				EQ_(6)
					FC_(0)
						CORE(18)
						CORE(19)
					};
					FC_(1)
						CORE(1a)
						CORE(1b)
					};
				};
			};

			CHIPLET_(27)
				EQ_(7)
					FC_(0)
						CORE(1c)
						CORE(1d)
					};
					FC_(1)
						CORE(1e)
						CORE(1f)
					};
				};
			};

		};
	};
')dnl

dnl
dnl I2CBUS([index])
dnl
define(`I2CBUS', `i2c-$1 {
}')dnl

dnl
dnl TPM([index])
dnl
define(`TPM',
`
   CONCAT(tpm, $1) {
       compatible = "ibm,power10-tpm";
       index = <$1>;
   };
')

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

	TPM(0)
};

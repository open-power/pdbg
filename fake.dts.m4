define(`CONCAT', `$1$2')dnl

dnl
dnl forloop([var], [start], [end], [iterator])
dnl
divert(`-1')
define(`forloop', `pushdef(`$1', `$2')_forloop($@)popdef(`$1')')
define(`_forloop',
       `$4`'ifelse($1, `$3', `', `define(`$1', incr($1))$0($@)')')

dnl
dnl dump_thread([index])
dnl
define(`dump_thread',
`
          thread@$1 {
            #address-cells = <0x0>;
            #size-cells = <0x0>;
            compatible = "ibm,fake-thread";
            reg = <0x$1 0x0>;
            index = <0x$1>;
          };
')dnl

dnl
dnl dump_core_pre([index], [addr])
dnl
define(`dump_core_pre',
`
        core@$2 {
          #address-cells = <0x1>;
          #size-cells = <0x1>;
          compatible = "ibm,fake-core";
          reg = <0x$2 0x0>;
          index = <0x$1>;')

dnl
dnl dump_core_post()
dnl
define(`dump_core_post', `        };
')dnl

dnl
dnl dump_core([index], [addr], [num_threads])
dnl
define(`dump_core',
`dump_core_pre(`$1', `$2')
forloop(`i', `0', eval(`$3-1'), `dump_thread(i)')
dump_core_post()')

dnl
dnl dump_processor_pre([index])
dnl
define(`dump_processor_pre',`dnl
     pib {
        #address-cells = <0x1>;
        #size-cells = <0x1>;
	ATTR2 = "processor$1";')

dnl
dnl dump_processor_post()
dnl
define(`dump_processor_post', `    };
')dnl

dnl
dnl dump_processor([index], [num_cores], [num_threads])
dnl
define(`dump_processor',dnl
`dump_processor_pre(`$1')
forloop(`i', `0', eval(`$2-1'), `dump_core(i, eval(10000+(i+1)*10), $3)')
dump_processor_post()')

dnl
dnl dump_system([num_processors], [num_cores], [num_threads])
dnl
define(`dump_system',
`forloop(`i', `0', eval(`$1-1'),dnl
`
    CONCAT(proc,i) {
      index = < CONCAT(0x,i) >;
      compatible = "ibm,fake-proc";

dump_processor(i, $2, $3)
    };
')')
divert`'dnl

/dts-v1/;

/ {
    #address-cells = <0x1>;
    #size-cells = <0x1>;
    ATTR1 = [ 00 ];
    ATTR2 = [ 00 00 ];
    ATTR4 = <0x0>;
    ATTR8 = <0x0 0x0>;
    ATTR16 = <0x0 0x0 0x0 0x0>;

dump_system(8, 4, 2)
};

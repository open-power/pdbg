define(`CONCAT', `$1$2')dnl

dnl
dnl forloop([var], [start], [end], [iterator])
dnl
divert(`-1')
define(`forloop', `pushdef(`$1', `$2')_forloop($@)popdef(`$1')')
define(`_forloop',
       `$4`'ifelse($1, `$3', `', `define(`$1', incr($1))$0($@)')')

dnl
dnl dump_system([num_processors], [num_cores], [num_threads])
dnl
define(`dump_system',
`forloop(`i', `0', eval(`$1-1'),dnl
`
    CONCAT(proc,i) {
      index = < CONCAT(0x,i) >;
    };
')
')
divert`'dnl

/dts-v1/;

/ {
    #address-cells = <0x1>;
    #size-cells = <0x1>;
dump_system(8, 4, 2)
};

import os, sys

bench = ['adder.aig', 'ctrl.aig', 'i2c.aig','mem_ctrl.aig','sin.aig','arbiter.aig','dec.aig','int2float.aig','multiplier.aig','sqrt.aig','bar.aig','div.aig','log2.aig','priority.aig','square.aig','cavlc.aig','max.aig','router.aig','voter.aig']

bench_mcnc = ['5xp1.aig',
              'Z9sym.aig',
              'amd.aig',
              'c1355.aig',
              'c1908.aig',
              'c2670.aig',
              'c3540.aig',
              'c5315.aig',
              'c6288.aig',
              'c7552.aig',
              'dalu.aig',
              'dk48.aig',
              'apex1.aig',
              'apex2.aig',
              'apex3.aig',
              'apex4.aig',
              'apex5.aig',
              'apex6.aig',
              'apex7.aig',
              'b2.aig',
              'b3.aig',
              'b4.aig',
              'cps.aig',
              'des.aig',
              'intdiv-dxs-rec16.aig',
              'intdiv-dxs-rec32.aig',
              'intdiv-dxs-rec64.aig'
              ]

also_exe_path    = '/Users/chu/also/build/bin/also'
bench_epfl_path = '/Users/chu/benchmarks/aiger/epfl/'
bench_mcnc_path = '/Users/chu/benchmarks/aiger/'
verilog_path = '/Users/chu/also/utils/'

abc_exe_path = '/Users/chu/abc/abc'
also_cmds = 'lut_mapping; lut_resyn -nx; xmgrw; xmginv'

def parse_file_name( fname ):
    return fname.split('.')[0]

def run_epfl():
    for i in range(len(bench)):
      name = parse_file_name( bench[i] )
      print bench[i]
      cmd_mig = ''+ also_exe_path + ' -c \" read_aiger ' + bench_epfl_path + bench[i] + '; '+ also_cmds + '; write_verilog -x '+ name +'.v \"'
      cmd_abc = ''+ abc_exe_path + ' -c \" cec -n '+ bench_epfl_path + bench[i] + ' '+ verilog_path + name +'.v \" '
      os.system( cmd_mig )
      os.system( cmd_abc)
      print '\n\n'

def run_mcnc():
    for i in range(len(bench_mcnc)):
      name = parse_file_name( bench_mcnc[i] )
      print bench_mcnc[i]
      cmd_mig = ''+ also_exe_path + ' -c \" read_aiger ' + bench_mcnc_path + bench_mcnc[i] + '; '+ also_cmds + '; write_verilog -x '+ name +'.v \"'
      cmd_abc = ''+ abc_exe_path + ' -c \" cec -n '+ bench_mcnc_path + bench_mcnc[i] + ' '+ verilog_path + name +'.v \" '
      os.system( cmd_mig )
      os.system( cmd_abc)
      print '\n\n'

if __name__ == "__main__":
    argc = len( sys.argv )
    if argc == 1 or argc > 2:
        print( "usage: ./run_koala.py epfl/mcnc" )
        print( "example: ./run_koala.py epfl" )
        exit(1)

    strategy = sys.argv[1]

    if strategy == "mcnc":
        run_mcnc()
    elif strategy == "epfl":
        run_epfl()
    else: 
       print( "UNKOWN Strategy\n" )


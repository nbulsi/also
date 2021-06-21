#! /bin/bash
# Name:test.sh
# Desc:test command of also for some circuits
# Path:also/test/
# Usage:you must be in the directory (also/test),then run ./test.sh
# Update: 2021.6.21
# Author: Chuanghe Shang, Zhufei Chu

path_also='also -c'
path_circuit=./aiger_benchmarks

function judge_running()
{
    if [ $? -ne 0 ];then
	echo "Error $1 with $2!!"
    else
	echo "Pass!\n"
    fi
}

atm=aig_to_m*ig
atx=aig_to_xmg
atg=aig_to_xag
mtxgm=mig_to_xmg/xag/m*ig


for i in $path_circuit/*.aig
do
    echo "(1) Test for $atm with command: \n
	read_aiger $i; lut_mapping; lut_resyn -n --test_m3ig; lut_resyn -n --test_m5ig; lut_resyn -n; convert --aig_to_mig; convert --mig_to_xmg; mighty; quit"
    $path_also "
	read_aiger $i; ps -a;
	lut_mapping;
	lut_resyn -n --test_m3ig;
	lut_resyn -n --test_m5ig;
	lut_resyn -n;
	convert --aig_to_mig; ps -m;
	convert --mig_to_xmg; ps -x;
	mighty;
	quit" &>/dev/null
    judge_running $atm $i

    echo "(2) Test for $atx with command: \n
	read_aiger $i; lut_mapping; lut_resyn -nm; lut_resyn -nx; convert --aig_to_xmg; xmgban; xmgrw; xmginv; xmgcost2; quit"
    $path_also "
	read_aiger $i;
	lut_mapping; ps -l;
	lut_resyn -nm; ps -m;
	lut_resyn -nx; ps -x;
	convert --aig_to_xmg;
	xmgban;
	xmgrw;
	xmginv;
	xmgcost2;
	quit" &>/dev/null
    if [ -e test.bench ];then
	rm test.bench
    fi
    judge_running $atx $i

    echo "(3) Test for $atg  with command: \n
	read_aiger $i; lut_mapping; lut_resyn -ng; xagban; xagopt; xagrs; xagrw; quit"
    $path_also "
	read_aiger $i;
	lut_mapping;
	lut_resyn -ng;
	xagban;
	xagopt;
	xagrs;
	xagrw;
	quit" &>/dev/null
    judge_running $atg $i

    echo "(4) Test for $mtxgm with command: \n
	read_aiger $i; convert --aig_to_mig; lut_mapping -m; lut_resyn -n; lut_resyn -nm; lut_resyn -nx; lut_resyn -ng; quit"
    $path_also "
	read_aiger $i;
	convert --aig_to_mig;
	lut_mapping -m;
	lut_resyn -n;
	lut_resyn -nm;
	lut_resyn -nx;
	lut_resyn -ng;
	quit" &>/dev/null
    judge_running $mtxgm $i
done

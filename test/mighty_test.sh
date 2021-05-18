#! /bin/bash

# Name:test.sh
# Desc:test command of also for some circuits
# Path:also/test/
# Usage:you must be in the directory (also/test),then run ./test.sh
# Update: 2021.5.10

path_also='../build/bin/also -c'
path_circuit=./benchmarks

function judge_running()
{
    if [ $? -ne 0 ];then
	echo -e "\e[1;31mError $1 with $2!!\e[0m"
    else
	echo -e "\e[1;32mPass!\e[0m\n"
    fi
}

atm=aig_to_m*ig
atx=aig_to_xmg
atg=aig_to_xag
#ati=aig_to_img
mtxgm=mig_to_xmg/xag/m*ig
xtmg=xmg_to_m*ig/xag
gtmx=xag_to_m*ig/xmg


for i in $path_circuit/*.aig
do
    echo -e "-------test for \e[1;31m$atm\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	lut_mapping\n
	lut_resyn -n --test_m3ig\n
	lut_resyn -n --test_m5ig\n
	lut_resyn -n\n
	convert --aig_to_mig\n
	convert --mig_to_xmg;\n
	mighty-------"
    $path_also "
	read_aiger $i;
	lut_mapping;
	lut_resyn -n --test_m3ig;
	lut_resyn -n --test_m5ig;
	lut_resyn -n;
	convert --aig_to_mig;
	convert --mig_to_xmg;
	mighty;
	quit" &>/dev/null
    judge_running $atm $i

    echo -e "-------test for \e[1;31m$atx\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	lut_mapping\n
	lut_resyn -nm\n
	lut_resyn -nx\n
	convert --aig_to_xmg\n
	xmgban\n
	xmgrw\n
	xmginv\n
	xmgcost2-------"
    $path_also "
	read_aiger $i;
	lut_mapping;
	lut_resyn -nm;
	lut_resyn -nx;
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
    
    echo -e "-------test for \e[1;31m$atg\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	lut_mapping\n
	lut_resyn -ng\n
	xagban\n
	xagopt\n
	xagrs\n
	xagrw\n
	quit-------"
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

:<<'COMMENT'
    echo -e "-------test for \e[1;31m$ati\e[0m with command: read_aiger \e[1;31m$i\e[0m;lut_mapping;lut_resyn -ni;imgff;imgrw;quit-------"
    $path_also "read_aiger $i;lut_mapping;lut_resyn -ni;imgff;imgrw;quit" &>/dev/null
    judge_running $ati $i
COMMENT
    
    echo -e "-------test for \e[1;31m$mtxgm\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	convert --aig_to_mig\n
	lut_mapping -m\n
	lut_resyn -n\n
	lut_resyn -nm\n
	lut_resyn -nx\n
	lut_resyn -ng\n
	lut_resyn -n --test_m3ig\n
	lut_resyn -n --test_m5ig\n
	quit-------"
    $path_also "
	read_aiger $i;
	convert --aig_to_mig;
	lut_mapping -m;
	lut_resyn -n;
	lut_resyn -nm;
	lut_resyn -nx;
	lut_resyn -ng;
	lut_resyn -n --test_m3ig;
	lut_resyn -n --test_m5ig;
	quit" &>/dev/null
    judge_running $mtxgm $i

    echo -e "-------test for \e[1;31m$xtmg\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	convert --aig_to_xmg\n
	lut_mapping -x\n
	lut_resyn -n\n
	lut_resyn -nm\n
	lut_resyn -nx\n
	lut_resyn -ng\n
	lut_resyn -n --test_m3ig\n
	lut_resyn -n --test_m5ig\n
	quit-------"
    $path_also "
	read_aiger $i;
	convert --aig_to_xmg;
	lut_mapping -x;
	lut_resyn -n;
	lut_resyn -nm;
	lut_resyn -nx;
	lut_resyn -ng;
	lut_resyn -n --test_m3ig;
	lut_resyn -n --test_m5ig;
	quit" &>/dev/null
    judge_running $xtmg $i

    echo -e "-------test for \e[1;31m$gtmx\e[0m with command: \n
	read_aiger \e[1;31m$i\e[0m\n
	convert --aig_to_mig\n
	lut_mapping -m\n
	lut_resyn -ng\n
	lut_mapping -g\n
	lut_resyn -n\n
	lut_resyn -nx\n
	lut_resyn -nm\n
	lut_resyn -ng\n
	lut_resyn -n --test_m3ig\n
	lut_resyn -n --test_m5ig\n
	quit-------"
    $path_also "
	read_aiger $i;
	convert --aig_to_mig;
	lut_mapping -m;
	lut_resyn -ng;
	lut_mapping -g;
	lut_resyn -n;
	lut_resyn -nx;
	lut_resyn -nm;
	lut_resyn -ng;
	lut_resyn -n --test_m3ig;
	lut_resyn -n --test_m5ig;
	quit" &>/dev/null
    judge_running $gtmx $i



done




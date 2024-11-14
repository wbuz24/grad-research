#!/bin/bash

 

# $2 is l3 size

# $3 is mc size

# $4 meta switch

# $5 l3 switch

 

gdb -x gdb_bp.gdb --args "./build/ARM/gem5.debug" \

--debug-flags=MCX2,MemCtrl \

configs/example/mcx_fs.py \

--disk-image $M5_PATH/disks/ubuntu_q4.img \

--kernel $M5_PATH/binaries/unmodified_vmlinux.arm64 \

--cpu-type DerivO3CPU \

--mem-size 8GB \

--num-cpus 4 \

--checkpoint-restore 1 \

--checkpoint-dir /home/connorbremner/gem5resources/cpts/HW/602.gcc_s \

--maxinsts 500000000 \

\

--caches \

--cacheline_size 64 \

--l1d_size 32kB \

--l1i_size 16kB \

--l2cache \

--l2_size 512kB \

--l3_size 2MiB \

--meta-size 64KiB \

--l1d_assoc 2 \

--l1i_assoc 2 \

--l2_assoc 8 \

--l3_assoc 64 \

--meta-filter never \

--l3-filter never \

--mem-filter DEFAULT \

#--no-meta-cache \

#--num-l2caches NUM_L2CACHES

#--num-l3caches NUM_L3CACHES

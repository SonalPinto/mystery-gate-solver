#!/bin/bash

# make build1
# make build2
# make build1p
make build2p

# c432 c880 c1355 c2670 c3540 c5315 c6288 c7552 c3540 

for block in c880 c1355 c2670 c3540 c5315
do
	# export NUMTHREADS=16
	# ./mgs_pbf_perf ckts/$block | tee ${block}_pbf_16.log
	# export NUMTHREADS=64
	# ./mgs_pbf_perf ckts/$block | tee ${block}_pbf_64.log
	# export NUMTHREADS=256
	# ./mgs_pbf_perf ckts/$block | tee ${block}_pbf_256.log

	export NUMTHREADS=4
	./mgs_psat_perf ckts/$block | tee ${block}_psat_4.log
	export NUMTHREADS=8
	./mgs_psat_perf ckts/$block | tee ${block}_psat_8.log
	export NUMTHREADS=16
	./mgs_psat_perf ckts/$block | tee ${block}_psat_16.log
	export NUMTHREADS=32
	./mgs_psat_perf ckts/$block | tee ${block}_psat_32.log
	export NUMTHREADS=64
	./mgs_psat_perf ckts/$block | tee ${block}_psat_64.log
	export NUMTHREADS=128
	./mgs_psat_perf ckts/$block | tee ${block}_psat_128.log
done
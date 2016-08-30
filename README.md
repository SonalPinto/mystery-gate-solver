Mystery Gate Solver
=================== 

Author: Sonal Pinto
Source Code submitted towards CS5234 Term Project


BUILD
==========

```
make build
```

builds all targets

| Target | compiled objects |
| --- | --- |
| buildsim | runsim |
| build1 | mgs_bf, mgs_bf_perf |
| build2 | mgs_sat, mgs_sat_perf |
| build1p | mgs_pbf, mgs_pbf_perf |
| build2p | mgs_psat, mgs_psat_perf |

where,
bf = brute-force
sat = Functional Satisfiability
psat = Parallel fSAT
\_perf = automated benchmarking


RUN
==========
```
SIMPLE RUN
	Input: levelized gate netlist with mystery gates
	
 	Sample input files: 
		ckts/c432_mystery2
		ckts/s298_mystery2

BENCHMARKING RUN
	Input: levelized gate netlist without mystery gates
	The code will incrementally obfuscate and attempt to solve
		for the mystery gates, and display the results at the end. 
	Timeout: 10mins
		
	Sample input files:
		ckts/c432
		ckts/cc880
		ckts/c1355
		ckts/c2670
		ckts/c3540
		ckts/c5315
		ckts/c6288
		ckts/c7552
```


Serial : Simple
---------------
```
	./mgs_sat ckts/c432_mystery2
```

Serial : Benchmarking
---------------------
```
	./mgs_sat_perf ckts/c432
```


Parallel: Simple
----------------
```
	export NUMTHREADS=32
	./mgs_psat ckts/c432_mystery2
```

Parallel: Benchmarking
----------------------
```
	export NUMTHREADS=32
	./mgs_psat_perf ckts/c432
```

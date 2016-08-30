
CC = g++
CFLAGS = -std=c++0x -g
OTHER = gateLevelCkt.cpp eventSimulator.cpp


buildsim: runsim.cpp
	${CC} ${CFLAGS} runsim.cpp ${OTHER} -o runsim

build1: mgs_bf.cpp mgs_bf_perf.cpp
	${CC} ${CFLAGS} mgs_bf.cpp ${OTHER} -o mgs_bf
	${CC} ${CFLAGS} mgs_bf_perf.cpp ${OTHER} -o mgs_bf_perf

build2: mgs_sat.cpp mgs_sat_perf.cpp
	${CC} ${CFLAGS} mgs_sat.cpp ${OTHER} -o mgs_sat
	${CC} ${CFLAGS} mgs_sat_perf.cpp ${OTHER} -o mgs_sat_perf

build1p: mgs_pbf.cpp mgs_pbf_perf.cpp
	${CC} ${CFLAGS} -fopenmp mgs_pbf.cpp ${OTHER} -o mgs_pbf
	${CC} ${CFLAGS} -fopenmp mgs_pbf_perf.cpp ${OTHER} -o mgs_pbf_perf

build2p: mgs_psat.cpp mgs_psat_perf.cpp
	${CC} ${CFLAGS} -fopenmp mgs_psat.cpp ${OTHER} -o mgs_psat
	${CC} ${CFLAGS} -fopenmp mgs_psat_perf.cpp ${OTHER} -o mgs_psat_perf

build: clean buildsim build1 build2 build1p build2p


.PHONY: clean
clean:
	rm -f runsim mgs_bf mgs_bf_perf mgs_sat mgs_sat_perf mgs_pbf mgs_pbf_perf mgs_psat mgs_psat_perf


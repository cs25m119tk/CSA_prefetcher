# ChampSim Custom Prefetcher

## Description

This project implements a custom prefetcher in ChampSim.  
It combines stride delta based for indirect memory access typically in sparse graphs, along with dynamic tuning of prefetch degree and distance. 

---

## Trace Files

SPEC CPU traces used in this project are available at:

https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/

---

## Build Instructions

After modifying `champsim_config.json`, rebuild ChampSim:

```bash
./config.sh champsim_config.json
make clean
make -j
```

---

## Running Simulation

### General Command (Skeleton)

```bash
script -q -c "./bin/champsim \
--warmup-instructions <WARMUP_INSTRUCTIONS> \
--simulation-instructions <SIMULATION_INSTRUCTIONS> \
<TRACE_FILE_PATH>" <OUTPUT_FILE>
```

### Example

```bash
script -q -c "./bin/champsim \
--warmup-instructions 50000000 \
--simulation-instructions 100000000 \
/home/kumaru/project/473.astar-359B.champsimtrace.xz" result_nextline_custom_astar.txt
```

---

## Notes

- Multiple traces such as astar, soplex, and milc can be used for evaluation.
- The prefetcher can be changed in the configuration file to compare performance.
- Output files contain logs and results 

---
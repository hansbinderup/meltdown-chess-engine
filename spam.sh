#!/bin/bash


# Optional: number of times to run it
NUM_RUNS=1000

for ((i = 1; i <= NUM_RUNS; i++)); do
  ./.build/meltdown-chess-engine bench
done

#!/bin/bash
clang -O3 -shared -fPIC -std=c11 -march=native -ffast-math \
  openteis_stigmergy.c -o libopenteis_stigmergy.so -lm
echo "[✔] libopenteis_stigmergy.so kompilert."

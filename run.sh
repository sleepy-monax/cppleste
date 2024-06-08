#!/bin/bash
set -e
mkdir -p bin
clang++ `sdl2-config --cflags --libs` -lSDL2 -lSDL2_mixer -o bin/Celeste src/celeste.cpp src/main.cpp src/p8.cpp
./bin/Celeste

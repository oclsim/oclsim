#!/bin/bash

if [[ $1 == "mandel" ]]; then
	make -E "PGR=$1" && "./build/$1" > ./build/mandel.dat
	octave ./mandel_plot.m
else
	make -E "PGR=$1" && "./build/$1"
fi

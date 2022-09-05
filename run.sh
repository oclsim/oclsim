#!/bin/bash

make -E "PGR=$1" && "./build/$1"

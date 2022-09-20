/*
Copyright (C) 2022 Franco Sauvisky
ising_defs.h is part of oclsim

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef ISING_DEFS_HEADER
#define ISING_DEFS_HEADER

// Definitions & constants:
#define SIZEX 64
#define SIZEY 64
#define BUFFLEN (1024)
#define VECLEN (SIZEX*SIZEY)
#define NEIGH_N 4
#define PROB_L (NEIGH_N+1)
#define PROB_Z (NEIGH_N/2)
#define PROB_MAX 1.0
#define MEASDIV 512
#define REPEAT_SIM 256

#define GLOBAL_1D_LENGTH (VECLEN)
#define GLOBAL_1D_RANGE {VECLEN,0,0}
#define GLOBAL_2D_RANGE {SIZEX,SIZEY,0}

#define LOCAL_2D_WIDTH 16
#define LOCAL_1D_LENGTH (LOCAL_2D_WIDTH*LOCAL_2D_WIDTH)
#define LOCAL_1D_RANGE {LOCAL_1D_LENGTH,0,0}
#define LOCAL_2D_RANGE {LOCAL_2D_WIDTH,LOCAL_2D_WIDTH,0}

#define ISING_DIMS_1D ((dims_i){.dim=1,.global=GLOBAL_1D_RANGE,.local=LOCAL_1D_RANGE})
#define ISING_DIMS_2D ((dims_i){.dim=2,.global=GLOBAL_2D_RANGE,.local=LOCAL_2D_RANGE})

#define OVERSAMPLE 1

// Macros:
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)>(y)?(y):(x))
#define IND(x,y) ( (x)*SIZEX + (y) ) // 2D xy -> 1D vector
#define RIND(x,y) ( ((x)%SIZEX)*SIZEX + ((y)%SIZEY) ) // rectangular
#define TIND(x,y) ( ((x)*SIZEX + (y))%VECLEN ) // torus
#define GETI(c,x,y) ( *(y) = ((c)-(*(x)=(c)/SIZEX)) ); // 1D vector -> 2D xy

// Typedefs:
#ifdef __OPENCL_VERSION__
typedef int state_t;
typedef int out_t;
typedef int int_t;
typedef uint uint_t;
typedef float float_t;
typedef uint rand_st;
#else
typedef cl_int state_t;
typedef cl_int out_t;
typedef cl_int int_t;
typedef cl_uint uint_t;
typedef cl_float float_t;
typedef cl_uint rand_st;
#endif

typedef struct state_s* state_p;
typedef struct output_s* output_p;
typedef struct init_arg_s* init_arg_p;
typedef struct main_arg_s* main_arg_p;
typedef struct meas_arg_s* meas_arg_p;

// Structs:
struct output_s
{
  state_t states[BUFFLEN/MEASDIV][VECLEN];
  out_t mag[BUFFLEN/MEASDIV];
} __attribute__((__packed__));

struct output2_s
{
  state_t states[BUFFLEN][VECLEN];
  out_t mag[BUFFLEN];
} __attribute__((__packed__));

struct state_s
{
  state_t state[VECLEN];
  rand_st rseeds[VECLEN];
  int_t counter;
} __attribute__((__packed__));

struct init_arg_s
{
  rand_st rseed;
} __attribute__((__packed__));

struct main_arg_s
{
  uint_t probs[PROB_L];
} __attribute__((__packed__));

struct meas_arg_s
{
  uint_t idiv;
  int_t ioffset;
} __attribute__((__packed__));

#endif

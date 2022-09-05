/*
Copyright (C) 2022 Franco Sauvisky
oclsim.h is part of oclsim

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef OCLSIM_HEADER_BLOCK
#define OCLSIM_HEADER_BLOCK

#define CL_TARGET_OPENCL_VERSION 200
#include <CL/cl.h>

#define INIT_K_NAME "init_k"
#define MAIN_K_NAME "update_k"
#define MEASURE_K_NAME "measure_k"

typedef struct oclsim_sys* oclSys;

typedef struct _dims_i
{
  size_t dim; // run dimensions
  size_t global[3]; // global range
  size_t local[3]; // local range
} dims_i;

// void ocls_print_devices(void);
oclSys cls_new_sys(int plat_i, int dev_i);

void cls_load_sys_from_file(oclSys sys, char* src_filename, size_t states_s);
void cls_load_sys_from_str(oclSys sys, char* src_str, size_t states_s);

void cls_set_init_arg(oclSys sys, void* arg, size_t arg_s, dims_i dims);
void cls_set_main_arg(oclSys sys, void* arg, size_t arg_s, size_t local_s, dims_i dims);
void cls_set_meas_arg(oclSys sys, void* arg, size_t arg_s, size_t local_s, size_t meas_s, dims_i dims);

void cls_run_init(oclSys sys);
void cls_run_update(oclSys sys);
void cls_run_meas(oclSys sys);

size_t cls_get_meas(oclSys sys, void *out);

void cls_release_sys(oclSys sys);

#endif

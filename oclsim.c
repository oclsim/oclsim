/*
Copyright (C) 2022 Franco Sauvisky
oclsim.c is part of oclsim

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oclsim.h"

#define PINFORM(x, ...) {fprintf(stderr, (x), ##__VA_ARGS__);}
#define PERROR(x,val) {fprintf(stderr,\
"Error (%d) on line %d, file %s (function %s):\n%s",\
val,__LINE__, __FILE__, __func__, (x));}
#define CHKERROR(flag,str) {if(flag){PERROR(str,flag);exit(1);};}

struct oclsim_sys
{
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  char state;

  cl_mem states_b[2];
  size_t states_s;
  cl_mem output_b;
  size_t output_s;

  cl_kernel init_k;
  dims_i init_d;
  cl_mem init_arg_b;
  size_t init_arg_s;

  cl_kernel main_k[2];
  dims_i main_d;
  cl_mem main_arg_b;
  size_t main_arg_s;
  size_t main_local_s;

  cl_kernel meas_k[2];
  dims_i meas_d;
  cl_mem meas_arg_b;
  size_t meas_arg_s;
  size_t meas_local_s;
};

oclSys
cls_new_sys(int plat_i, int dev_i)
{
  oclSys newsys = (oclSys)calloc(1,sizeof(struct oclsim_sys));
  cl_int err=0;
  cl_uint plats_n;

  err = clGetPlatformIDs(0,NULL,&plats_n);
  cl_platform_id platforms[plats_n];
  CHKERROR((plat_i<0)||(plat_i>=plats_n), "Selected platform is out of range");

	err = clGetPlatformIDs(plats_n, platforms, NULL);
  CHKERROR(err<0,"Couldn't idenfity platforms");

  newsys->platform = platforms[plat_i];

  char platname[100];
  err = clGetPlatformInfo(newsys->platform,CL_PLATFORM_NAME,100,platname,NULL);
  CHKERROR(err<0,"Couldn't get platform name");
  PINFORM("Selected platform: %s\n", platname);

  cl_uint devs_n;
  err=clGetDeviceIDs(newsys->platform,CL_DEVICE_TYPE_ALL,0,NULL,&devs_n);

  cl_device_id devices[devs_n];
  err=clGetDeviceIDs(newsys->platform,CL_DEVICE_TYPE_ALL,devs_n,devices,NULL);
  CHKERROR(err<0,"Couldn't identify device");
  CHKERROR((dev_i<0)||(dev_i>=devs_n),"Selected device is out of range");
  newsys->device = devices[dev_i];

  char devname[100];
  err = clGetDeviceInfo(newsys->device,CL_DEVICE_NAME,100,devname,NULL);
  CHKERROR(err<0,"Couldn't get platform name");
  PINFORM("Selected device: %s\n", devname);

  newsys->context = clCreateContext(NULL, 1, &newsys->device, NULL, NULL, &err);
  CHKERROR(err<0,"Couldn't create context");

  newsys->queue = clCreateCommandQueueWithProperties(newsys->context,
    newsys->device, (cl_queue_properties[])
    {CL_QUEUE_PROPERTIES,CL_QUEUE_PROFILING_ENABLE,0}, &err);

  newsys->state=0;

  CHKERROR(err, "Couldn't create queue");
  return newsys;
}

void
cls_load_sys_from_str(oclSys sys, char *src_str, size_t states_size)
{
  cl_int err=0;
  size_t src_size = strlen(src_str);

  sys->program = clCreateProgramWithSource(sys->context, 1,(const char**)
                                               &src_str, &src_size, &err);
  CHKERROR(err<0, "Couldn't create program");

  err = clBuildProgram(sys->program, 0, NULL, "-I.", NULL, NULL);
  if(err < 0) // Print compilation log if fails for debugging code
  {
    size_t log_size;
    clGetProgramBuildInfo(sys->program, sys->device,
                          CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    char *log_buff = (char*)malloc(log_size + 1);
    log_buff[log_size] = '\0';
    clGetProgramBuildInfo(sys->program, sys->device,
                          CL_PROGRAM_BUILD_LOG, log_size, log_buff, NULL);
    printf("%s\n", log_buff);
    free(log_buff);
    exit(1);
  }

  sys->init_k = clCreateKernel(sys->program, INIT_K_NAME, &err);
  sys->main_k[0] = clCreateKernel(sys->program, MAIN_K_NAME, &err);
  sys->main_k[1] = clCreateKernel(sys->program, MAIN_K_NAME, &err);
  sys->meas_k[0] = clCreateKernel(sys->program, MEASURE_K_NAME, &err);
  sys->meas_k[1] = clCreateKernel(sys->program, MEASURE_K_NAME, &err);

  sys->states_s = states_size;
  sys->states_b[0] = clCreateBuffer(sys->context, CL_MEM_READ_WRITE, states_size, NULL, &err);
  sys->states_b[1] = clCreateBuffer(sys->context, CL_MEM_READ_WRITE, states_size, NULL, &err);
  CHKERROR(err, "Couldn't load system kernels/state buffers");
}

void
cls_load_sys_from_file(oclSys sys, char *src_filename, size_t states_size)
{
  FILE *src_fh = fopen(src_filename, "r");
  char *src_buff;
  size_t src_size;

  CHKERROR(src_fh==NULL, "Couldn't open .cl src file");
  fseek(src_fh, 0, SEEK_END);
  src_size = ftell(src_fh);
  rewind(src_fh);
  src_buff = (char*)malloc(src_size + 1);
  src_buff[src_size] = '\0';
  fread(src_buff, sizeof(char), src_size, src_fh); // Copy src to buffer
  fclose(src_fh);
  cls_load_sys_from_str(sys, src_buff, states_size);
  free(src_buff);
}

void
cls_set_init_arg(oclSys sys, void* arg, size_t arg_s, dims_i dims)
{
  cl_int err=0;

  // if((sys->init_arg_s!=arg_s)||(sys->init_arg_b==NULL))
  // {
  //   if(sys->init_arg_b!=NULL)
  //   {
  //     clReleaseMemObject(sys->init_arg_b);
  //     sys->init_arg_b=NULL;
  //   }
    sys->init_arg_s = arg_s;
    sys->init_arg_b = clCreateBuffer(sys->context, CL_MEM_READ_ONLY, arg_s, NULL, &err);
  // }

  sys->init_d = dims;
  err |= clSetKernelArg(sys->init_k, 0, sizeof(cl_mem), &sys->states_b[0]);
  err |= clSetKernelArg(sys->init_k, 1, sizeof(cl_mem), &sys->init_arg_b);
  err |= clEnqueueWriteBuffer(sys->queue, sys->init_arg_b, CL_FALSE, 0, arg_s, arg, 0, NULL, NULL);

  CHKERROR(err<0,"Coudn't configure init kernel");
}

void
cls_set_main_arg(oclSys sys, void* arg, size_t arg_s, size_t local_s, dims_i dims)
{
  cl_int err=0;

  // if((sys->main_arg_s!=arg_s)||(sys->main_arg_b==NULL))
  // {
  //   if(sys->main_arg_b!=NULL)
  //   {
  //     clReleaseMemObject(sys->main_arg_b);
  //     sys->main_arg_b=NULL;
  //   }
    sys->main_arg_s = arg_s;
    sys->main_arg_b = clCreateBuffer(sys->context, CL_MEM_READ_ONLY, arg_s, NULL, &err);
  // }

  sys->main_d = dims;
  sys->main_local_s = local_s;

  err |= clEnqueueWriteBuffer(sys->queue, sys->main_arg_b, CL_FALSE, 0, arg_s, arg, 0, NULL, NULL);
  err |= clSetKernelArg(sys->main_k[0], 0, sizeof(cl_mem), &sys->states_b[1]);
  err |= clSetKernelArg(sys->main_k[0], 1, sizeof(cl_mem), &sys->states_b[0]);
  err |= clSetKernelArg(sys->main_k[0], 2, local_s, NULL);
  err |= clSetKernelArg(sys->main_k[0], 3, sizeof(cl_mem), &sys->main_arg_b);

  err |= clSetKernelArg(sys->main_k[1], 0, sizeof(cl_mem), &sys->states_b[0]);
  err |= clSetKernelArg(sys->main_k[1], 1, sizeof(cl_mem), &sys->states_b[1]);
  err |= clSetKernelArg(sys->main_k[1], 2, local_s, NULL);
  err |= clSetKernelArg(sys->main_k[1], 3, sizeof(cl_mem), &sys->main_arg_b);

  CHKERROR(err<0,"Coudn't create/configure update kernel");
}

void
cls_set_meas_arg(oclSys sys, void* arg, size_t arg_s, size_t local_s, size_t meas_s, dims_i dims)
{
  cl_int err=0;

  // if((sys->meas_arg_s!=arg_s)||(sys->meas_arg_b==NULL))
  // {
  //   if(sys->meas_arg_b!=NULL)
  //   {
  //     clReleaseMemObject(sys->meas_arg_b);
  //     sys->meas_arg_b=NULL;
  //   }
    sys->meas_arg_s = arg_s;
    sys->meas_arg_b = clCreateBuffer(sys->context, CL_MEM_READ_ONLY, arg_s, NULL, &err);
  // }

  // if((sys->output_s!=meas_s)||(sys->output_b==NULL))
  // {
  //   if(sys->output_b!=NULL)
  //   {
  //     clReleaseMemObject(sys->output_b);
  //     sys->output_b=NULL;
  //   }
    sys->output_s = meas_s;
    sys->output_b = clCreateBuffer(sys->context, CL_MEM_READ_WRITE, meas_s, NULL, &err);
  // }

  sys->meas_d = dims;
  sys->meas_local_s = local_s;

  err |= clEnqueueWriteBuffer(sys->queue, sys->meas_arg_b, CL_FALSE, 0, arg_s, arg, 0, NULL, NULL);

  err |= clSetKernelArg(sys->meas_k[0], 0, sizeof(cl_mem), &sys->output_b);
  err |= clSetKernelArg(sys->meas_k[0], 1, sizeof(cl_mem), &sys->states_b[0]);
  err |= clSetKernelArg(sys->meas_k[0], 2, local_s, NULL);
  err |= clSetKernelArg(sys->meas_k[0], 3, sizeof(cl_mem), &sys->meas_arg_b);

  err |= clSetKernelArg(sys->meas_k[1], 0, sizeof(cl_mem), &sys->output_b);
  err |= clSetKernelArg(sys->meas_k[1], 1, sizeof(cl_mem), &sys->states_b[1]);
  err |= clSetKernelArg(sys->meas_k[1], 2, local_s, NULL);
  err |= clSetKernelArg(sys->meas_k[1], 3, sizeof(cl_mem), &sys->meas_arg_b);

  CHKERROR(err<0,"Coudn't create/configure measure kernel");
}

void
cls_run_init(oclSys sys)
{
  cl_int err=0;
  clEnqueueNDRangeKernel(sys->queue, sys->init_k, sys->init_d.dim, NULL,
    sys->init_d.global, sys->init_d.local, 0, NULL, NULL);
  CHKERROR(err<0,"Coudn't enqueue init kernel");
}

void
cls_run_update(oclSys sys)
{
  cl_int err=0;
  if(~sys->state&0x01)
  {
    err|=clEnqueueNDRangeKernel(sys->queue, sys->main_k[0], sys->main_d.dim, NULL,
      sys->main_d.global, sys->main_d.local, 0, NULL, NULL);
  }
  else
  {
    err|=clEnqueueNDRangeKernel(sys->queue, sys->main_k[1], sys->main_d.dim, NULL,
      sys->main_d.global, sys->main_d.local, 0, NULL, NULL);
  }
  sys->state^=1;
  CHKERROR(err<0,"Coudn't enqueue main kernel");
}

void
cls_run_meas(oclSys sys)
{
  cl_int err=0;
  if(~sys->state&0x01)
  {
    err|=clEnqueueNDRangeKernel(sys->queue, sys->meas_k[0], sys->meas_d.dim, NULL,
      sys->meas_d.global, sys->meas_d.local, 0, NULL, NULL);
  }
  else
  {
    err|=clEnqueueNDRangeKernel(sys->queue, sys->meas_k[1], sys->meas_d.dim, NULL,
      sys->meas_d.global, sys->meas_d.local, 0, NULL, NULL);
  }
  CHKERROR(err<0,"Coudn't enqueue measure kernel");
}

size_t
cls_get_meas(oclSys sys, void *out)
{
  cl_int err=0;

  // if(out!=NULL)
  // {
    err|=clFlush(sys->queue);
    err|=clFinish(sys->queue);
    err|= clEnqueueReadBuffer(sys->queue, sys->output_b, CL_TRUE, 0,
      sys->output_s, out, 0, NULL, NULL);
  // }

  CHKERROR(err<0,"Coudn't read output data");
  return sys->output_s;
}

void
cls_release_sys(oclSys sys)
{
  if(sys->program) {clReleaseProgram(sys->program); sys->program=NULL;}
  if(sys->queue) {clReleaseCommandQueue(sys->queue); sys->queue=NULL;}
  if(sys->context) {clReleaseContext(sys->context); sys->context=NULL;}
  if(sys->states_b[0]) {clReleaseMemObject(sys->states_b[0]); sys->states_b[0]=NULL;}
  if(sys->states_b[1]) {clReleaseMemObject(sys->states_b[1]); sys->states_b[1]=NULL;}
  if(sys->init_k) {clReleaseKernel(sys->init_k); sys->init_k=NULL;}
  if(sys->init_arg_b) {clReleaseMemObject(sys->init_arg_b); sys->init_arg_b=NULL;}
  if(sys->main_k[0]) {clReleaseKernel(sys->main_k[0]); sys->main_k[0]=NULL;}
  if(sys->main_k[1]) {clReleaseKernel(sys->main_k[1]); sys->main_k[1]=NULL;}
  if(sys->main_arg_b) {clReleaseMemObject(sys->main_arg_b); sys->main_arg_b=NULL;}
  if(sys->meas_k[0]) {clReleaseKernel(sys->meas_k[0]); sys->meas_k[0]=NULL;}
  if(sys->meas_k[1]) {clReleaseKernel(sys->meas_k[1]); sys->meas_k[1]=NULL;}
  if(sys->meas_arg_b) {clReleaseMemObject(sys->meas_arg_b); sys->meas_arg_b=NULL;}
  if(sys->output_b) {clReleaseMemObject(sys->output_b); sys->output_b=NULL;}
  free(sys);
}

// void
// cls_print_devices(void)
// {
//
// }

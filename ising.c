/*
Copyright (C) 2022 Franco Sauvisky
ising.c is part of oclsim

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "oclsim.h"
#include "ising.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

int64_t millis()
{
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  return ((int64_t) now.tv_sec) * 1000 + ((int64_t) now.tv_nsec) / 1000000;
}

void
main(void)
{
  oclSys ising = cls_new_sys(2,0);
  cls_load_sys_from_file(ising, "./ising.cl", sizeof(struct state_s));

  struct init_arg_s init_arg;
  struct main_arg_s main_arg;
  struct meas_arg_s meas_arg = {.idiv = MEASDIV, .ioffset = -BUFFLEN/4-MEASDIV};

  uint rseed = (uint)time(NULL);
  srand(rseed);

  struct output_s *out = malloc(sizeof(struct output_s));

  for(float temp = 2.0; temp < 3.0; temp+=0.05)
  {
    double mag = 0.0, mag2 = 0.0;

    for(int i = 0; i < PROB_L; i++)
    {
      main_arg.probs[i] = (cl_ulong)CL_UINT_MAX * PROB_MAX * MIN(1.0, exp(-4.0*(i-PROB_Z)/temp));
    }

    for(int k = 0; k < REPEAT_SIM; k++)
    {
      cl_uint new_seed = rand();
      init_arg.rseed = new_seed;

      cls_set_init_arg(ising, &init_arg, sizeof(init_arg), ISING_DIMS_2D);
      cls_set_main_arg(ising, &main_arg, sizeof(main_arg), 1, ISING_DIMS_2D);
      cls_set_meas_arg(ising, &meas_arg, sizeof(meas_arg), sizeof(state_t)*LOCAL_1D_LENGTH, sizeof(struct output_s), ISING_DIMS_1D);

      cls_run_init(ising);

      for(int i = 0; i < BUFFLEN/4; i++)
      {
        cls_run_update(ising);
      }

      for(int i = 0; i < BUFFLEN/MEASDIV; i++)
      {
        for(int k = 0; k < MEASDIV; k++)
        {
          cls_run_update(ising);
        }
        cls_run_meas(ising);
      }

      cls_get_meas(ising, out);

      for(int i = 0; i < BUFFLEN/MEASDIV; i++)
      {
        mag += (double)out->mag[i];
        mag2 += pow(out->mag[i],2);
      }
    }
    printf("%f %f %f\n", temp, mag/(BUFFLEN/MEASDIV*REPEAT_SIM), sqrt(mag2/(BUFFLEN/MEASDIV*REPEAT_SIM)));
  }

  cls_release_sys(ising);
}

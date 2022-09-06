/*
Copyright (C) 2022 Franco Sauvisky
test.c is part of oclsim

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
#include "mandel.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

void
main(void)
{
  oclSys testsim = cls_new_sys(1,0);
  cls_load_sys_from_file(testsim, "./test.cl", sizeof(struct state_s));

  struct init_arg_s init_arg = {.z0={.x=X0, .y=Y0}, .dz={.x=DX, .y=DY}};
  struct main_arg_s main_arg;
  struct meas_arg_s meas_arg;

  cls_set_init_arg(testsim, &init_arg, sizeof(init_arg), ISING_DIMS_2D);
  cls_set_main_arg(testsim, &main_arg, sizeof(main_arg), 1, ISING_DIMS_2D);
  cls_set_meas_arg(testsim, &meas_arg, sizeof(meas_arg), 1, sizeof(struct output_s), ISING_DIMS_1D);

  cls_run_init(testsim);
  for(int i = 0; i < ITER; i++)
  {
    cls_run_update(testsim);
  }

  cls_run_meas(testsim);
  struct output_s *out = malloc(sizeof(struct output_s));
  cls_get_meas(testsim, out);

  for(int i = 0; i < VECLEN*VECLEN; i++) printf("%d,%f\n",out->lastc[i],out->abs[i]);
  // printf("\n");
  cls_release_sys(testsim);
}

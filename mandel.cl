#include "mandel.h"

kernel void
init_k(global struct state_s *output,
       constant struct init_arg_s *arg)
{
  size_t i = get_global_id(0), j = get_global_id(1), ij = IND(i,j);
  state_t z0;

  z0.x = arg->z0.x+arg->dz.x*((int_t)i-VECLEN/2);
  z0.y = arg->z0.y+arg->dz.y*((int_t)j-VECLEN/2);

  output->states[ij] = z0;
  output->z0[ij] = z0;
  output->lastc[ij]=1;
}

kernel void
update_k(global struct state_s *output,
         global struct state_s *input,
         local void *lc_skpd,
         constant struct main_arg_s *arg)
{
  size_t i = get_global_id(0), j = get_global_id(1), ij = IND(i,j);
  state_t z = input->states[ij], z0 = input->z0[ij], new;
  int_t lcount = input->lastc[ij];

  new.x = z.x*z.x-z.y*z.y+z0.x; // z^2+z0
  new.y = 2*z.x*z.y+z0.y;

  float_t abs = new.x*new.x + new.y*new.y;
  int_t mask = abs<=4.0;

  output->states[ij] = new*mask+in*(1-mask); // update or hold
  output->lastc[ij] = lcount+mask;
  output->z0[ij] = z0;
}

kernel void
measure_k(global struct output_s *output,
          global struct state_s *input,
          local void* lc_skpd,
          constant struct meas_arg_s *arg)
{
  size_t i = get_global_id(0);
  state_t ins = input->states[i];
  output->lastc[i] = input->lastc[i];
  output->abs[i] = ins.x*ins.x+ins.y*ins.y;
}

#include "ising.h"

//  MWC64X Random Number Generator
// uint
// MWC64X(uint2 *state)
// {
//   enum {A=4294883355U};
//   uint x=(*state).x, c=(*state).y;  // Unpack the state
//   uint res=x^c;                     // Calculate the result
//   uint hi=mul_hi(x,A);              // Step the RNG
//   x=x*A+c;
//   c=hi+(x<c);
//   *state=(uint2)(x,c);              // Pack the state back up
//   return res;                       // Return the next result
// }

// Based on "4-byte Integer Hashing" by Thomas Wang
// http://www.burtleburtle.net/bob/hash/integer.html
inline uint
randomize_seed(uint a)
{
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return a;
}

kernel void
init_k(global struct state_s *output,
     constant struct init_arg_s *arg)
{
  size_t i = get_global_id(0),
         j = get_global_id(1);

  size_t ij = IND(i,j);
  int rand_sample = randomize_seed(arg->rseed + 42013*ij);

  output->state[ij] = (rand_sample>0)?-1:1;
  output->rseeds[ij] = randomize_seed(arg->rseed + 98473*ij);

  if(ij==0)
  {
    output->counter = 0;
  }
}

kernel void
update_k(global struct state_s *output,
       global struct state_s *input,
       local void *lc_skpd,
       constant struct main_arg_s *arg)
{
  size_t i = get_global_id(0),
         j = get_global_id(1);
  size_t ij = IND(i,j);
  uint rand_sample = input->rseeds[ij];
  uint iter =  input->counter;

  state_t self_s = input->state[IND(i,j)];
  state_t neig1_s = input->state[RIND(i-1,j)];
  state_t neig2_s = input->state[RIND(i,j-1)];
  state_t neig3_s = input->state[RIND(i+1,j)];
  state_t neig4_s = input->state[RIND(i,j+1)];
  state_t s_sum = self_s*(neig1_s+neig2_s+neig3_s+neig4_s);

  uint par = ((i+j+(iter%2))%2); // checkboard pattern (0 or 1)
  char flip = par&&(rand_sample < arg->probs[(size_t)(PROB_Z + s_sum/2)]);

  output->state[ij] = (flip)?-self_s:self_s;
  output->rseeds[ij] = randomize_seed(rand_sample + 42013*ij);
  if(ij==0)
  {
    output->counter = iter+1;
  }
}

kernel void
measure_k(global struct output_s *output,
        global struct state_s *input,
        local void* lc_skpd,
        constant struct meas_arg_s *arg)
{
  size_t i = get_global_id(0),
         i_l = get_local_id(0),
         i_T = get_local_size(0);
  int iter = input->counter;
  size_t out_i = (iter+arg->ioffset)/arg->idiv;
  state_t self_s = input->state[i];
  local state_t *local_buff = lc_skpd;

  // Parallel sum in local buffer
  local_buff[i_l] = self_s;
  for(int delta = i_T/2; delta != 0; delta >>= 1)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    if(i_l<delta) local_buff[i_l] += local_buff[i_l + delta];
  }

  output->states[out_i][i] = self_s; // copy state
  if(i_l == 0)
  {
    atomic_add(&output->mag[out_i], local_buff[0]);
  }
}

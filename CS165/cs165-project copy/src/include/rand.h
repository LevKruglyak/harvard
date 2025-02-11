#pragma once

// Credit: https://github.com/ESultanik/mtwister/blob/master/mtwister.h
//
#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

typedef struct {
  unsigned long mt[STATE_VECTOR_LENGTH];
  int index;
} mt_rand;

mt_rand m_srand(unsigned long seed);
unsigned long m_gen_rand(mt_rand* rand);
double m_genf_rand(mt_rand* rand);

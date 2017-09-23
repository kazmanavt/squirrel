#ifndef OUT_H_
#define OUT_H_

#include <algos.h>

int sqo_init ( void );

extern SQAlgoObject_t* sqo_algo;
static inline
void sqo_set_curr_algo ( SQAlgoObject_t* alg )
{
  sqo_algo = alg;
}


int sqo_oorsend ( void* data, size_t len );

#endif // OUT_H_
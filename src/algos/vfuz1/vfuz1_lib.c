#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "algos.h"
#include "tsignals.h"
#include "proto.h"
#include "rail.h"
#include "vfuzzy1.h"



/* **************************************************************************************
 */
int sqr_fake_read ( char *fileName, SQPsignal_t *buff, int step ) {
  int i = step, k;
  FILE *IN = fopen ( fileName, "r" );
  if ( IN == NULL ) {
    perror ( "ERR> open incomming signals file failure: " );
    return 0;
  }
  size_t bs = 1024;
  double ( *a ) [17], dval;
  size_t n = 0, N = bs;
  a = malloc ( N * 17 * sizeof ( double ) );

  while ( fscanf ( IN,
                   "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
                   &a[n][0],  &a[n][1],  &a[n][2],  &a[n][3],
                   &a[n][4],  &a[n][5],  &a[n][6],  &a[n][7],
                   &a[n][8],  &a[n][9],  &a[n][10], &a[n][11],
                   &a[n][12], &a[n][13], &a[n][14], &a[n][15], &a[n][16] ) == 17 ) {

    ++n;
    if ( n == N ) {
      a = realloc ( a, ( N + bs ) * 17 * sizeof ( double ) );
      if ( a == NULL ) {
        perror ( "ERR> memory alloc0 for inp task failed: " );
        /// TODO change to exit
        _exit ( -1 );
      }
      N += bs;
    }
  }

  i = step % n;
  a = realloc ( a, n * 17 * sizeof ( double ) );

  for ( k = 0; k < 16; k++ ) {
    buff[k].trust = 0.0;
    buff[k].val = a[i][k + 1];
    buff[k].ts.sec = ( uint_fast32_t ) ( a[i][0] );
    dval = ( ( ( double ) a[i][0] - ( double ) buff[k].ts.sec ) * 1000. );
    buff[k].ts.usec = ( uint_fast32_t ) ( dval + 0.5 );
    printf ( "----fake read  K=%d Time=%d s(%d ms) Cod=%d %f Trust=%f \n", k,
             buff[k].ts.sec, buff[k].ts.usec,
             buff[k].code, buff[k].val, buff[k].trust );

  }
  fclose ( IN );
  return 16;

}
#define ALL_MUST_VALID 1
/* *********************************************************
 * */
/* check if data is valid */
int sqR_dataIsValid ( SQPsignal_t *buff, int buf_len ) {
  int i;

  for ( i = 0; i < buf_len; i++ ) {
#ifndef  ALL_MUST_VALID
    if ( buff[i].trust != 1.0 && ( buff[i].ts.sec != 0 ||  buff[i].ts.usec != 0 ) ) {
      return 1;
    } /* AT LEASE ONE trusted data found
                       * the trust is 0 !!!! if data is valid */
#else
    if ( buff[i].trust == 1.0 || ( buff[i].ts.sec == 0 &&  buff[i].ts.usec == 0 ) ) {
      return 0;
    } /* AT LEASE ONE UN trusted data found
                       * the trust is 0 !!!! if data is valid */
#endif
  }
#ifndef  ALL_MUST_VALID
  return 0;
#else
  return 1;
#endif
}

/* **************************************************************************
 */
double sq_fuz_norm_signal ( SQTSigdef_t sig_info, SQPsignal_t sig1,
                            SQPsignal_t sig2 )
/* compute the normalized deviation for one signal */
{
  double ds;
  double dt, dv;
  double signorm = 0, delta = sig_info.delta;

  if ( delta < 0 ) { /* if delta negative compute it from parts of the value itelf */
    delta = fabs ( sig1.val ) * fabs ( delta );
  }

  ds = ( double ) ( sig2.val - sig1.val );
  dt = ( double ) ( sig2.ts.sec - sig1.ts.sec ) + ( double ) ( sig2.ts.usec -
                                                               sig1.ts.usec ) / 1000000.;
  // fprintf(stderr,"DT=%f\n",dt);

  if ( ds == 0 ) {
    return signorm = 0.0;
  }

  if ( dt != 0.0 ) {
    /*   delta=sig_info.delta/dt; necessary only if delta in lags not in sec */
    dv = ds / dt;
  } else {
    return signorm = ds / fabs ( ds );
  }

  if ( delta != 0.0 ) {
    dv = dv / delta;
  } else {
    return signorm = ds / abs ( ds );
  }

  if ( dv > 1.0 ) {
    dv = 1.0;
  }

  if ( dv < -1 ) {
    dv = -1;
  }

  signorm = dv;

  return signorm;


}

/* ***********************************************************************
 */
void   computeSignalNormsSQR ( SQTSigdef_t *signals_info, SQPsignal_t *buff1,
                               SQPsignal_t *buff2, double *inpArray, int buf_len )
/* compute signals difference and put result into inpArray to pass to the fuzzy engine */
{
  int n;

  for ( n = 0; n < buf_len; n++ ) {
    inpArray[n] = sq_fuz_norm_signal ( signals_info[n], buff1[n], buff2[n] );
  }
}


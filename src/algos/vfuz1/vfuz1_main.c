/*!
  @defgroup agr_vfyz Контроллер нечеткой логики
  @ingroup defalgos
  Описание...
  @{
*/
/*!
  @file vfuz1_main.c

  @date 12.11.2012
  @author v1925@mail.ru
*/

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <rail.h>
#include <time.h>
#include <math.h>
#include <sys/timeb.h>
#include "algos.h"
#include "simple_conf.h"
#include "out.h"
#include "vfuzzy1.h"

char *name = "VFUZ1";
uint32_t version = 101;
uint32_t protocol = 0;

#define SWAP_PING(val) (val==1?0:1)
// #define DEBUG_VPFUZ1 1 /* add some  print with debug  data */
//
//
typedef struct fuzAlgoDescriptor {
  char *name;

  char **sigs;
  char **osigs;
  int PROBLEM_SIZE;
  SQPsignal_t *vfuzbuf[2];
  int ping_pong;
  SQPsignal_t *vfuz1bufwrite; /* some usefull write buffer */
  SQTSigdef_t *siginfo;
  EngineDesc eng;  /* fuzzy engine descriptor */
  int firstRun;
  unsigned int cntIter;
  int alarmCnt;
  int PROBLEM_COUNTER;   /* how many times we should get alarm when deceison about fault will be made? */

} FuzAlgoDescriptor;



void *run ( FuzAlgoDescriptor *pfuz ) {
  void *retPtr = NULL;
  int realread;
  double valArray[pfuz->PROBLEM_SIZE];
  int numOfVals = pfuz->PROBLEM_SIZE;
  double outval;


  if ( ( realread = sqr_read ( pfuz->vfuzbuf[pfuz->ping_pong], NULL ) ) == 0 ) {
    return retPtr;  /* nothing to do no data */
  }

  if ( sqR_dataIsValid ( pfuz->vfuzbuf[pfuz->ping_pong], pfuz->PROBLEM_SIZE ) == 0 ) {
    fprintf ( stderr, "No valid  data %f\n", outval );
    return retPtr;  /* no valid data */
  }

  if ( pfuz->firstRun == 0 ) {
    pfuz->firstRun = 1;
  } else { /* skipp the first update as we need to check delta between signals */

    computeSignalNormsSQR ( pfuz->siginfo,
                            pfuz->vfuzbuf[SWAP_PING ( pfuz->ping_pong )],
                            pfuz->vfuzbuf[pfuz->ping_pong], valArray, pfuz->PROBLEM_SIZE );
    setAllFuzzyInput ( valArray, numOfVals, &pfuz->eng );
    outval = processFuzzy ( &pfuz->eng );
    if ( isnan ( outval ) ) {
      pfuz->alarmCnt = 0; /* clear sequental alarm counter */
#ifdef DEBUG_VPFUZ1
      if ( ( pfuz->cntIter % 100 ) == 0 ) { // simple DEBUG
        fprintf ( stderr, "Normal situation \n" );
      }
#endif
    } else {
      if ( pfuz->alarmCnt >= pfuz->PROBLEM_COUNTER ) {
#if 0
        fprintf ( stderr, "Fault detected at time %d %06d with weight %f\n",
                  vfuzbuf[ping_pong][0].ts.sec,
                  vfuzbuf[ping_pong][0].ts.usec, outval );
#endif
        char test[1024];
        snprintf ( test, sizeof ( test ),
                   "Fault detected at time %d %06d with weight %f\n",
                   pfuz->vfuzbuf[pfuz->ping_pong][0].ts.sec,
                   pfuz->vfuzbuf[pfuz->ping_pong][0].ts.usec, outval );
        sqo_oorsend ( test, strlen ( test ) + 1 );
        // exit(0);
      }
#if 0
      fprintf ( stderr, "Alarm at time %d %06d with weight %f\n",
                vfuzbuf[ping_pong][0].ts.sec,
                vfuzbuf[ping_pong][0].ts.usec, outval );
#endif
      pfuz->alarmCnt++; /* increment sequenal alarm counter */
    }
  }
  pfuz->ping_pong = SWAP_PING ( pfuz->ping_pong );
  // usleep(SLEEPLAG/4);
  pfuz->cntIter++;
  return 0;
}

char **qs ( FuzAlgoDescriptor *pfuz ) {
  return pfuz->sigs;
}

char **qo ( FuzAlgoDescriptor *pfuz ) {
  return pfuz->osigs;
}


/*!
  @~russian
  @short создает экземпляр алгоритма.

  Создает экземпляр алгоритма в соответствии со строкой инициализации задаваемой
  аргументом @c init_str.
  @param[in] init_str строка инициализации
  @returns Указатель на идентифицирующий созданный экземпляр алгоритма, в случае
           успешного завершения. При возникновении ошибок возвращается нулевой
           указатель @c NULL.

  @~english
  @short creates instance of algorythm.

  Creates instance of algorythm inspired by initialization string given by @c init_str
  argument.
  @param[in] init_str initialization string
  @returns On success, pointer identificating created instance of algorythm is returned.
           In case of error @c NULL pointer is returned.
*/
void *create ( char *init_str ) {
  FuzAlgoDescriptor *pfuz;
  int n;
  char *fconf = init_str;
  char modName[128] = "mod.fis";
  char param[128];
  char outsig_name[] = "ARBITRARY_VFUZ1";


  /* initilize new fuzyy algo descriptor */
  pfuz = calloc ( 1, sizeof ( FuzAlgoDescriptor ) );
  pfuz->name = name;
  /* initilize the name of output signal */
  pfuz->osigs = calloc ( 2, sizeof ( char * ) );
  pfuz->osigs[0] = calloc ( 1, strlen ( outsig_name ) + 1 );

  /* set problem counter , default to one */
  if ( sc_read_parms ( fconf, "FUZ_PROBLEM_COUNTER = %d", &pfuz->PROBLEM_COUNTER,
                       NULL ) == 0 ) {
    pfuz->PROBLEM_COUNTER = 1;
  }

  snprintf ( param, sizeof ( param ), "FIS_MODEL = %%%dc", sizeof ( modName ) );




  sc_read_parms ( fconf, param , modName, NULL );

  fprintf ( stderr, "Use FIS model=%s\n", modName );

  if ( ( pfuz->PROBLEM_SIZE = initSquirrelFuzzy ( modName, &pfuz->sigs,
                                                  &pfuz->eng ) ) <=  0 ) {
    fprintf ( stderr,
              "Non consistent list of input signals in model, scan ends on %d\n",
              pfuz->PROBLEM_SIZE );
    exit ( 2 );
  }

  // TODO correct memory allocation order and requested shared segment size
  // sizeof vmin vmax is unknown till allocation !!!!!!!!!!
  // There is no params passing to vfuz1_qs vfuz1_qo need corrections !!!!!
  for ( n = 0; n < 2; n++ )
    pfuz->vfuzbuf[n] = ( SQPsignal_t * ) malloc ( sizeof ( SQPsignal_t ) *
                                                  pfuz->PROBLEM_SIZE );


  return pfuz;
}
/* Функция будут вызвана на втром этапе инициализации */
void init ( FuzAlgoDescriptor *pfuz ) {
  int n;

  pfuz->siginfo = calloc ( pfuz->PROBLEM_SIZE, sizeof ( SQTSigdef_t ) );
  /* parse signal info */
  for ( n = 0; n < pfuz->PROBLEM_SIZE; n++ )
    if ( sqr_get_sigdef ( pfuz->sigs[n], &pfuz->siginfo[n] )  == -1 ) {
      fprintf ( stderr, "No signal info for signal %s\n", pfuz->sigs[n] );
      exit ( 2 );
    }
}

//! @}
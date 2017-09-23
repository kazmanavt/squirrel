/*!
  @weakgroup worker
  @ingroup intern
  @{
  Контейнер алгоритмов предназначается для циклического исполнения ассоциированных с ним
  алгоритмов. Список ассоциированных алгоритмов и длительность цикла являются для
  контейнера входными параметрами. Контейнер в течении одного цикла выпполняет однократный
  вызов каждого алгоритма. Так же выполняется контроль времени исполнения.
*/

/*!
  @file worker.c
  @short реализация контейнера алгоритмов.

  @see @ref worker @n
  @date 27.11.2012
  @author masolkin@gmail.com
*/

#define _ISOC99_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <unistd.h>

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
#error C99 compliant compilator required
#endif

#if !defined(_POSIX_VERSION) || (_POSIX_VERSION < 200809L)
#error POSIX.1-2008 expected to be supported
#endif

#if !defined(_XOPEN_VERSION) || (_XOPEN_VERSION < 700)
#error Open Group Single UNIX Specification, Version 4 (SUSv4) expected to be supported
#endif

#if !defined(__gnu_linux__)
#error GNU Linux is definitely supported by now
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>

// #include <math.h>

#include <kz_erch.h>
#include <fastlog.h>
#include <algos.h>
#include <rail.h>
// #include <worker.h>
#include <out.h>

static bool run = true;
static void     term_handler ( int signum, siginfo_t *info, void *data ) {
  run = false;
}

/*!
  @details
  При запуске контейнера алгоритмов выполняется калибровка вызовов таймера, и переход к
  циклическому исполнению алгоритмов переданных в вызове контейнера.
  @param[in] pool_id идентификатор контейнера.
  @param[in] algos   массив указателей на экземпляры алгоритмов.
  @param[in] tic     длительность цикла обработки технологических сигналов для
                 контейнера алгоритмов вцелом.
  @retval  0 при корректном завершении.
  @retval -1 при завершении в случае ошибок.
*/
int sqw_start_worker ( int pool_id, SQAlgoObject_t **algos, long tic ) {
  struct sigaction siact;
  memset ( &siact, 0, sizeof ( siact ) );
  siact.sa_sigaction = term_handler;
  EC_NEG1 ( sigfillset ( &siact.sa_mask ) );
  siact.sa_flags = SA_SIGINFO;
  EC_NEG1 ( sigaction ( SIGTERM, &siact, NULL ) );

  sigset_t mask;
  EC_NEG1 ( sigemptyset ( &mask ) );
  EC_NEG1 ( sigaddset ( &mask, SIGTERM ) );
  EC_NEG1 ( sigprocmask ( SIG_UNBLOCK, &mask, NULL ) );

  // clock calibration
  // long  resolution, clk_tm, wait_tm, corr_tm = 10, run_tm, towait;

  int32_t correction = 0;
  {
    char who[] = "worker #xxxxxx";
    snprintf ( &who[8], 6, "%d", pool_id );
    EC_NEG1 ( correction = sq_calibrate ( who, 1000, 100000 ) );
  }



  for ( SQAlgoObject_t **alg = algos; *alg != NULL; alg++ ) {
    sqr_set_curr_algo ( *alg );
    sqo_set_curr_algo ( *alg );
    alg[0]->super->init ( alg[0]->subj );
  }

  struct timespec ts_cycle, ts;

  EC_NEG1 ( clock_gettime ( CONFIGURED_CLOCK, &ts_cycle ) );

  while ( run ) {
    for ( SQAlgoObject_t **alg = algos; *alg != NULL; alg++ ) {
      sqr_set_curr_algo ( *alg );
      sqo_set_curr_algo ( *alg );
      alg[0]->super->run ( alg[0]->subj );
    }

    EC_NEG1 ( clock_gettime ( CONFIGURED_CLOCK, &ts ) );

    int64_t delta = __INT64_C ( 1000000000 ) * ( ts.tv_sec - ts_cycle.tv_sec ) + ( ts.tv_nsec - ts_cycle.tv_nsec ) + correction;
    int64_t shift = delta - tic;
    if ( shift > 0 ) {
      /* code */
      fl_log ( "WRN> worker #%d: possible cycle drift detected [%g msec], moving origin!\n", pool_id, ( double ) ( ( delta - tic ) / 1000000.0 ) );
      int32_t sec = shift / __INT64_C ( 1000000000 );
      ts_cycle.tv_sec  += sec;
      ts_cycle.tv_nsec += ( shift - __INT64_C ( 1000000000 ) * sec );
    }
    ts_cycle.tv_nsec += tic;
    if ( ts_cycle.tv_nsec > 1000000000L ) {
      ts_cycle.tv_nsec -= 1000000000L;
      ++ts_cycle.tv_sec;
    }
    EC_RC ( clock_nanosleep ( CONFIGURED_CLOCK, TIMER_ABSTIME, &ts_cycle, NULL ) );
  }

  sqr_mb_release ();

  return 0;

  EC_CLEAN_SECTION (
    sqr_mb_release ();
    fl_log_ec ();
    return 1;
  );
}


//! @}

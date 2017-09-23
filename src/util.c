/*!
  @addtogroup util
  @{
*/
/*!
  @file util.c
  @short Общие функции, типы и пр.

  @em Реализация функций и прочих конструкций применяемых в различных местах @b squirrel.

  Функции реализованные здесь не имеют побочных эффектов.

  @see @ref util @n
  @date 05.02.2013
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
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#define __USE_GNU
#include <sched.h>
#include <math.h>


#include <util.h>

#include <kz_erch.h>
#include <fastlog.h>

/*!
  @param[in] p1 указатель на переменную типа <tt>char *</tt> представляющую null-терминированную
                строку.
  @param[in] p2 указатель на переменную типа <tt>char *</tt> представляющую null-терминированную
                строку.
  @retval @c 0 строки равны
  @retval @c <0 *p1 < *p2
  @retval @c >0 *p1 > *p2
*/
int compare_cstr ( const void *p1, const void *p2 ) {
  return strcmp ( * ( ( char ** ) p1 ), * ( ( char ** ) p2 ) );
}




/*!
  @returns текущее время в формате @ref SQtime_t
*/
SQtime_t sq_time ( void ) {
  struct timespec tp;

  //TODO change needed!!!
  clock_gettime ( CONFIGURED_CLOCK, &tp );

  SQtime_t sqt = { .sec = tp.tv_sec, .usec = tp.tv_nsec / 1000 };
  return sqt;
}


/*!
  @param[in] cpu id процессора к которому должен быть привязан вызывающий процесс.
  @param[in] prio приоритет устанавливаемый для процесса.
  @retval  0 в отсутствии ошибок
  @retval -1 в случае возникновения ошибок
*/
int sq_set_cpu ( int cpu, int prio ) {
  cpu_set_t affinity_mask;
  long num_proc;

  EC_NEG1 ( num_proc = sysconf ( _SC_NPROCESSORS_CONF ) );

  CPU_ZERO ( &affinity_mask );
  CPU_SET ( cpu, &affinity_mask );
  EC_NEG1 ( sched_setaffinity ( 0, num_proc, &affinity_mask ) );

  struct sched_param sp;
  sp.sched_priority = prio;
  EC_NEG1 ( sched_setscheduler ( 0, SCHED_FIFO, &sp ) ) ;

  return 0;

  EC_CLEAN_SECTION (
    return -1;
  );
}




/**************************************************/
/***********                           ************/
/********   clock calibration &analysis   *********/
/***********                           ************/
/**************************************************/

// calculate statistical parameters of given data set, actualy min, max and mean values
// (optionaly dispersion can be calculated)
// returns max value
static int32_t print_statistics ( int32_t *data, int32_t len, const char *who, const char *theme ) {
  int32_t vmin = INT32_MAX, vmax = 0;
  double av = 0.0, av_corr = 0.0/*, av2 = 0.0, av2_corr = 0.0*/;
  for ( int32_t i = 0; i < len; i++ ) {
    if ( data[i] > vmax ) {
      vmax = data[i];
    } else if ( data[i] < vmin ) {
      vmin = data[i];
    }
    double d_corr = data[i] - av_corr;
    double new = av + d_corr;
    av_corr = ( new - av ) - d_corr;
    av = new;

    // quadratic mean for dispersion calculation
    // d_corr = (double)data[i] * (double)data[i] - av2_corr;
    // new = av2 + d_corr;
    // av2_corr = (new - av2) - d_corr;
    // av2 = new;
  }
  av /= len;
  // double sigma = sqrt (av2 / len - av * av);

  // fl_log ( "INF> %s %s stats:\n   min / max / av / sigma: %ld / %ld / %g / %g\n", who, theme, vmin, vmax, av, sigma );
  fl_log ( "INF> %s %s stats:\n   min / max / av: %ld / %ld / %g\n", who, theme, vmin, vmax, av );

  return vmax;
}

// should be easy
#define my_drand48 drand48
// static int my_drand48 ()
// {
//   return 1;
// }
/*!
  @details выполняется тайминг функций clock_gettime(), clock_nanosleep() и nanosleep(), в контексте текущего процесса.
  Так же учитывается разрешение текущего источника времени(Выбираемого макросом @c CONFIGURED_CLOCK. По умолчанию в
  @b squirrel используется @c CLOCK_REALTIME).
  @param[in] who          символьная строка описывающая вызывающий процесс, для отображения в информационных сообщениях.
  @param[in] num_tests    количество тестов проводимых для набора статистики.
  @param[in] max_duration максимальное время случайной задержки в тестах для функций clock_nanosleep() и nanosleep().
  @returns возвращается максимальная ожидаемая куммулятивная задержка на цикл. Или @c -1 в случае возникновения ошибок.
*/
int32_t sq_calibrate ( const char *who, int32_t num_tests, int32_t max_duration ) {
  int32_t *deltas = NULL;
  EC_NULL ( deltas = calloc ( num_tests, sizeof ( *deltas ) ) );

  // checking time resolution
  struct timespec tm;
  EC_NEG1 ( clock_getres ( CONFIGURED_CLOCK, &tm ) );
  if ( tm.tv_sec != 0 || ( tm.tv_sec == 0 && tm.tv_nsec > 10000L ) ) {
    EC_UMSG ( "ERR> clock source has insufficient resolution: %ld.%09ld\n", tm.tv_sec, tm.tv_nsec );
  }
  fl_log ( "INF> %s: time resolution -> %ld ns\n", who, tm.tv_nsec );
  int32_t res = tm.tv_nsec;



  // calibration of clock_gettime() call
  int32_t dsec;
  struct timespec ts0, ts;

  for ( int32_t i = 0; i < num_tests; i++ ) {
    clock_gettime ( CONFIGURED_CLOCK, &ts0 );
    clock_gettime ( CONFIGURED_CLOCK, &ts );

    dsec = ts.tv_sec - ts0.tv_sec;
    if ( dsec >= 2 ) {
      EC_UMSG ( "ERR> clock_gettime() latency > 1 sec !!" );
    }
    deltas[i] = 1000000000L * dsec + ts.tv_nsec - ts0.tv_nsec;
  }
  int32_t tm_clk = print_statistics ( deltas, num_tests, who, "timing, clock_gettime()" );


  // calibration of empty section for clock_nanosleep
  for ( int32_t i = 0; i < num_tests; i++ ) {
    int32_t duration = my_drand48 () * max_duration;
    clock_gettime ( CONFIGURED_CLOCK, &ts0 );
    ts.tv_sec = ts0.tv_sec;
    ts.tv_nsec = ts0.tv_nsec + duration;
    if ( ts.tv_nsec > 1000000000L ) {
      ++ts.tv_sec;
      ts.tv_nsec -= 1000000000L;
    }
    clock_gettime ( CONFIGURED_CLOCK, &ts );

    dsec = ts.tv_sec - ts0.tv_sec;
    if ( dsec >= 2 ) {
      EC_UMSG ( "ERR> empty section latency >= 1 sec !!" );
    }
    deltas[i] = 1000000000L * dsec + ts.tv_nsec - ts0.tv_nsec;
  }
  print_statistics ( deltas, num_tests, who, "timing, empty section" );


  // calibration of clock_nanosleep() call
  for ( int32_t i = 0; i < num_tests; i++ ) {
    int32_t duration = my_drand48 () * max_duration;
    clock_gettime ( CONFIGURED_CLOCK, &ts0 );
    ts.tv_sec = ts0.tv_sec;
    ts.tv_nsec = ts0.tv_nsec + duration;
    if ( ts.tv_nsec > 1000000000L ) {
      ++ts.tv_sec;
      ts.tv_nsec -= 1000000000L;
    }
    EC_RC ( clock_nanosleep ( CONFIGURED_CLOCK, TIMER_ABSTIME, &ts, NULL ) );
    clock_gettime ( CONFIGURED_CLOCK, &ts );

    dsec = ts.tv_sec - ts0.tv_sec;
    if ( dsec >= 2 ) {
      EC_UMSG ( "ERR> clock_nanosleep() latency >= 1 sec !!" );
    }
    deltas[i] = 1000000000L * dsec + ts.tv_nsec - ts0.tv_nsec - duration;
  }
  int32_t tm_durf = print_statistics ( deltas, num_tests, who, "timing, clock_nanosleep()" );


  // calibration of nanosleep() call
  for ( int32_t i = 0; i < num_tests; i++ ) {
    int32_t duration = my_drand48 () * max_duration;
    struct timespec to = { 0, duration };
    clock_gettime ( CONFIGURED_CLOCK, &ts0 );
    EC_NEG1 ( nanosleep ( &to, NULL ) );
    clock_gettime ( CONFIGURED_CLOCK, &ts );

    dsec = ts.tv_sec - ts0.tv_sec;
    if ( dsec >= 2 ) {
      EC_UMSG ( "ERR> nanosleep() latency >= 1 sec !!" );
    }
    deltas[i] = 1000000000L * dsec + ts.tv_nsec - ts0.tv_nsec - duration;
  }
  print_statistics ( deltas, num_tests, who, "timing, nanosleep()" );

  free ( deltas );
  return res + tm_clk + tm_durf;

  EC_CLEAN_SECTION (
  if ( deltas != NULL ) {
  free ( deltas );
  }
  return -1;
  );
}

//! @}

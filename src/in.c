/*!
  @weakgroup datain
  @ingroup intern
  @{
  Задача приема технологических сигналов запускается в выделенном процессе,
  на выделенном ЦПУ, с доступом к [разделяемой шине сигналов](@ref rail).
  Здесь реализованы все операции необходимые для инициализации каналов приема
  технологических сигналов, получения сигналов из внешних источников, помещения
  принятой информации на внутреннюю шину.
*/
/*!
  @file in.c
  @short Реализация [задачи приема технологических сигналов](@ref datain).

  @see @ref datain @n
  @date 15.02.2013
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
#error Open Group Single UNIX Specification, Version 4 (SUSv4) expected to be supported (_XOPEN_VERSION)
#endif

#if !defined(__gnu_linux__)
#error GNU Linux is definitely supported by now
#endif



#include <stdlib.h>
#include <stdio.h> // FIXME: faked net exchange
#include <time.h>
#include <string.h>
// #include <math.h>
#include <signal.h>


#include <kz_erch.h>
#include <fastlog.h>
#include <jconf.h>
#include <simple_net.h>
// #include <worker.h>
#include <rail.h>
#include <tsignals.h>
#include <proto.h>


static bool run = true;
static void     term_handler( int signum, siginfo_t *info, void *data )
{
  run = false;
}

// FIXME: faked net exchange
double ( *RAW_DATA ) [17];
size_t NUM;
size_t TM_SHIFT;
int load_fake_signals();
int read_fake_signals ( SQPsignal_t*, int * );
static void     wakeup( int signum, siginfo_t *info, void *data ) {}
/* ++ */

/*!
  Вызов предназначен для инициализации [подсистемы приема технологических сигналов](@ref datain).
  В ходе вызова выполняются следующие операции.
*/
int sqi_run( const char **signals, int num_signals, long tic, bool fake_input )
{
  struct sigaction siact;
  int s = -1; //  Сокет
  uint32_t *map = NULL; // карта связи кодов сигналов на шине с кодами в сети
  memset( &siact, 0, sizeof( siact ) );
  siact.sa_sigaction = term_handler;
  EC_NEG1( sigfillset( &siact.sa_mask ) );
  siact.sa_flags = SA_SIGINFO;
  EC_NEG1( sigaction( SIGTERM, &siact, NULL ) );
// FIXME: faked net exchange
  siact.sa_sigaction = wakeup;
  EC_NEG1( sigaction( SIGUSR1, &siact, NULL ) );

  sigset_t mask;
  EC_NEG1( sigemptyset( &mask ) );
  EC_NEG1( sigaddset( &mask, SIGTERM ) );
  EC_NEG1( sigaddset( &mask, SIGUSR1 ) );  // FIXME: faked net exchange
  EC_NEG1( sigprocmask( SIG_UNBLOCK, &mask, NULL ) );

  //! Калибровка часиков.
  int32_t correction = 0;
  EC_NEG1( correction = sq_calibrate( "input task", 1000, 100000 ) );


  //! Инициализация соединения.
  int *proto_codes = NULL;
  if (!fake_input) {
    const char *host = NULL, *port = NULL;
    EC_NULL( host = jcf_s( NULL, ".iis_host" ) );
    EC_NULL( port = jcf_s( NULL, ".iis_port" ) );
  
    EC_NEG1( s = sn_stream_socket( NULL, NULL ) );
    EC_NEG1( sn_connect( s, host, port ) );

    EC_NULL( proto_codes = sqp_handshake( s, signals, num_signals ) );
  } else {
    // FIXME: faked net exchange
    EC_NEG1( load_fake_signals() );  
    EC_NULL( proto_codes  = malloc( 16* sizeof(int) ) );// FIXME: faked net exchange
    for ( int i = 0; i < 16; i++ ) proto_codes[i] = i;// FIXME: faked net exchange
  }

  int max_proto_code = -1;
  // find max code value for technological signal
  for ( int i = 0; i < num_signals; i++ ) {
    if ( proto_codes[i] > max_proto_code ) {
      max_proto_code = proto_codes[i];
    }
    // const SQTSigdef_t* sigdef;
    // EC_NULL ( sigdef = sqts_get_sigdef ( signals[i] ) );
    // if ( sigdef->code > max_code ) max_code = sigdef->code;
  }

  //! Связывание кодов сигналов с входными очередями.
  EC_NULL( map = calloc( max_proto_code + 1, sizeof( uint32_t ) ) );
  for ( int i = 0, qid = 0; i < num_signals; i++ ) {
    EC_CHCK( proto_codes[i], -1 );
    map[proto_codes[i]] = qid;
    const SQTSigdef_t *sigdef;
    EC_NULL( sigdef = sqts_get_sigdef( signals[i] ) );
    EC_NEG1( sqr_bind_queue( qid++, sigdef->code, sigdef->kks ) );
  }

  sqr_run( 1 );
  
  printf("---------Input await for others. Kill me with 'sudo kill -USR1 %d' ---------------\n", getpid());
  pause();
  printf("Yo me running!\n");

  SQPsignal_t *data;
  int32_t N = num_signals * 2;
  EC_NULL( data = malloc( num_signals * sizeof( SQPsignal_t ) ) );

  struct timespec ts_cycle, ts;

  EC_NEG1( clock_gettime( CONFIGURED_CLOCK, &ts_cycle ) );

  while ( run ) {
    int rc = 1;
    while ( rc ) {
      if ( ! fake_input ) {
        EC_NEG1( rc = sqp_recv( s, data, &N ) );
      } else {
        // FIXME: faked net exchange
        read_fake_signals(data, &N);
        rc = 0;
      }

      for ( int i = 0; i < N; i++ ) {
        EC_NEG1( sqr_write1( &data[i], map[data[i].code] ) );
      }
    }

    EC_NEG1( clock_gettime( CONFIGURED_CLOCK, &ts ) );

    int64_t delta = __INT64_C( 1000000000 ) * ( ts.tv_sec - ts_cycle.tv_sec ) + ( ts.tv_nsec - ts_cycle.tv_nsec ) + correction;
    int64_t shift = delta - tic;
    if ( shift > 0 ) {
      /* code */
      fl_log( "WRN> input task: possible cycle drift detected [%g msec], moving origin!\n", ( ( double ) shift ) / 1000000.0 );
      int32_t sec = shift / __INT64_C( 1000000000 );
      ts_cycle.tv_sec  += sec;
      ts_cycle.tv_nsec += ( shift - __INT64_C( 1000000000 ) * sec );
    }
    ts_cycle.tv_nsec += tic;
    if ( ts_cycle.tv_nsec > 1000000000L ) {
      ts_cycle.tv_nsec -= 1000000000L;
      ++ts_cycle.tv_sec;
    }
    EC_RC( clock_nanosleep( CONFIGURED_CLOCK, TIMER_ABSTIME, &ts_cycle, NULL ) );
  }

  sqr_mb_release();
  return 0;
  EC_CLEAN_SECTION(
    if ( map != NULL ) free( map );
    sqr_run( 1 );
    sqr_mb_release();
    if ( s != -1 ) close( s );

      fl_log( "INF> input task: abnormal termination\n" );
      ec_print("tmp");
      fl_log_ec();
      return 1;
    );
}


/* TODO: remove it naher */

int load_fake_signals() {
  FILE* IN;
  EC_NULL( IN = fopen ( "test.dlm", "r" ) );

  size_t bs = 1024;
  double ( *a ) [17], dval;
  size_t n = 0, N = bs;
  EC_NULL( a = malloc ( N * 17 * sizeof ( double ) ) );

  while ( fscanf ( IN, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
                   &a[n][0],  &a[n][1],  &a[n][2],  &a[n][3],
                   &a[n][4],  &a[n][5],  &a[n][6],  &a[n][7],
                   &a[n][8],  &a[n][9],  &a[n][10], &a[n][11],
                   &a[n][12], &a[n][13], &a[n][14], &a[n][15], &a[n][16] ) == 17 ) {

    ++n;
    if ( n == N ) {
      EC_REALLOC( a, ( N + bs ) * 17 * sizeof ( double ) );
      N += bs;
    }
  }
  fclose(IN);

  EC_REALLOC( a, n * 17 * sizeof ( double ) );

  RAW_DATA = a;
  NUM = n;
  TM_SHIFT = RAW_DATA[NUM-1][0] - RAW_DATA[0][0] + 1;
  return 0;

  EC_CLEAN_SECTION(
    if ( IN != NULL ) fclose( IN );

      fl_log( "INF> input task: fake input load failure\n" );
      // ec_print("tmp");
      fl_log_ec();
      return -1;
    );
}


int read_fake_signals ( SQPsignal_t* buff, int * num ) {
  static size_t tm_offset = 0;
  static size_t step = 0;
  size_t step_c = step % NUM;

  //struct timespec  ts;
  //clock_gettime( CONFIGURED_CLOCK, &ts );
  time_t sec = RAW_DATA[step_c][0];
  long usec = ( RAW_DATA[step_c][0] - sec ) * 1000000L;
  sec += tm_offset;

  for ( int i = 0; i < 16; i++ ) {
    buff[i].code = i;
    buff[i].val = RAW_DATA[step_c][i + 1];
    buff[i].trust = 0.0;
    buff[i].ts.sec = sec;
    buff[i].ts.usec = usec;
//    printf ( "----fake read  K=%d Time=%d s(%d ms) Cod=%d %f Trust=%f \n", i, buff[i].ts.sec, buff[i].ts.usec,
//            buff[i].code, buff[i].val, buff[i].trust );

  }

  ++step;
  if ( step_c == NUM - 1 ) {
    tm_offset += TM_SHIFT;
    printf("Kus'\n");
  }

  *num = 16;
  return 0;
}

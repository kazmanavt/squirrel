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


#include <stdio.h>
#include <string.h>
// #include <aio.h>

#include <kz_erch.h>
#include <fastlog.h>
#include <algos.h>
// #include <proto.h>
// #include <simple_conf.h>
// #include <simple_net.h>


#define SQ_ASYNC 1

// static int tosaz = -1;
SQAlgoObject_t* sqo_algo = NULL;



int sqo_init ( void )
{
  // char host[100], port[6];
  // EC_NEG1 ( sc_read_parms ( "sq.conf", "saz_host = %100c", &host,
  //                           "saz_port = %6c", &port, NULL ) );
  // EC_NEG1 ( tosaz = sn_dg_socket ( NULL, NULL ) );
  // EC_NEG1 ( sn_connect ( tosaz, host, port ) );

  return 0;

  // EC_CLEAN_BEG {
  //   return -1;
  // }  EC_CLEAN_END;
}

int sqo_oorsend ( void* data, size_t len )
{
  char str[len + 1];
  strncpy ( str, data, len );
  str[len] = '\0';
  printf ( "Get alarm  %s\n", str );
  // #if SQ_ASYNC == 0
  // #warning !!!! USING SYNCRONOUS OUTPUT !!!!
  //   EC_NEG1 ( write (tosaz, data, len) );
  // #else
  //   EC_RC ( aio_error (&sqo_algo->aio) );

  //   EC_NEG1 ( memset ( &sqo_algo->aio, 0, sizeof (sqo_algo->aio) ) );
  //   sqo_algo->aio.aio_fildes = tosaz;
  //   sqo_algo->aio.aio_sigevent.sigev_notify = SIGEV_NONE;
  //   sqo_algo->aio.aio_lio_opcode = LIO_WRITE;
  //   sqo_algo->aio.aio_buf    = data;
  //   sqo_algo->aio.aio_nbytes = len;
  //   EC_NEG1 ( aio_write (&sqo_algo->aio) );
  // #endif

  return 0;

  // EC_CLEAN_BEG {
  //   fl_log_ec ();
  //   ec_clean ();
  //   return -1;
  // } EC_CLEAN_END;
}


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
#include <stdlib.h>
#include <string.h>

#include <kz_erch.h>
#include <jconf.h>
#include <algos.h>
#include <rail.h>

#include <ext_rail.h>

static SQAlgoObject_t algo;
static SQAlgoContext_t ctx;


SQAlgoContext_t *sq_rail_init( const char *name )
{
  const char **kks = NULL;
  EC_NEG1( jcf_load( "sq.conf" ) );

  JCFobj cfs = NULL;
  EC_NULL( cfs = jcf_a( NULL, ".standalons" ) );

  const char *inp_nm = NULL;
  for ( int i = 0; i < jcf_al( cfs, NULL ); ++i ) {
    const char *nm;
    EC_NULL( nm = jcf_s( jcf_ai( cfs, NULL, i ), ".name" ) );
    if ( strcmp( name, nm ) == 0 ) {
      EC_NULL( inp_nm = jcf_s( jcf_ai( cfs, NULL, i ), ".inputs" ) );
      break;
    }
  }
  JCFobj inp = NULL;
  if ( inp_nm ) {
    EC_NULL( cfs = jcf_a( NULL, ".algos" ) );
    for ( int i = 0; i < jcf_al( cfs, NULL ); ++i ) {
      const char *id;
      EC_NULL( id = jcf_s( jcf_ai( cfs, NULL, i ), ".id" ) );
      if ( strcmp( id, inp_nm ) == 0 ) {
        EC_NULL( inp = jcf_o( jcf_ai( cfs, NULL, i ), ".inputs" ) );
        break;
      }
    }
  }
  if ( inp == NULL ) {
    EC_UMSG( "ERR> input signals block NOT found." );
  }

  EC_NULL( kks = calloc( jcf_ol( inp, NULL ), sizeof( char * ) ) );
  int idx = 0;
  JCFobj in = NULL;
  EC_ERRNO( in = jcf_o1st( inp, NULL ) );
  while ( in != NULL ) {
    EC_NULL( kks[idx++] = jcf_s( in, NULL ) );
    EC_ERRNO( in = jcf_onext( in ) );
  }

  EC_NEG1( sqr_attach() );

  for ( int i = 0; i < idx; ++i ) {
    int queue;
    EC_NEG1( queue = sqr_find_queue_by_name( kks[i] ) );
    SQAisigBind_t *bind;
    EC_NULL( bind = malloc( sizeof( SQAisigBind_t ) ) );
    bind->inp = "UNASSIGNED_SIGNAL_INPUT_NAME";
    bind->kks = kks[i];
    bind->queue = queue;
    bind->q_cursor = sqr_new_cursor( queue );
    bind->upd = 1;

    EC_REALLOC( algo.isigs, ( i + 2 ) * sizeof( SQAisigBind_t * ) );
    algo.isigs[i] = bind;
    algo.isigs[i + 1] = NULL;

    printf( "#%02d: %s\n", i, kks[i] );
  }

  algo.filter_chains = ( SQFilterChain_t ** ) calloc( 1 , sizeof( SQFilterChain_t * ) );

  sqr_set_curr_algo( &algo );

  EC_NULL( ctx.tic = jcf_l( NULL, ".tic" ) );
  ctx.num_sig = idx;
  free( kks );
  return &ctx;


  EC_CLEAN_SECTION(
    if ( kks ) free( kks );
    ec_print( "ERR: failed to init sq_rail subsystem\n" );
    return NULL;
  );
}

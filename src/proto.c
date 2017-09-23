//-#- noe -#-

/*!
  @addtogroup proto
  @{
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
#include <string.h>
#include <arpa/inet.h>


#include <proto.h>

#include <kz_erch.h>
#include <fastlog.h>
#include <simple_net.h>

typedef struct Header_s {
  int32_t code;
  int32_t r[2];
  int32_t len;
} Header_t;

typedef struct HeaderL_s {
  int32_t code;
  int32_t r[2];
  int32_t seq;
  int32_t subseq;
  int32_t sec;
  int32_t usec;
  int32_t n;
} HeaderL_t;

typedef struct Signal_s {
  int32_t code;
  int32_t sec;
  int32_t usec;
  int32_t val;
} Signal_t;


static inline
void bswap_nh( void *ptr, size_t size )
{
  for ( size_t i = 0; i < size / sizeof( int ); ++i ) {
    ( ( int * ) ptr ) [i] = ntohl( ( ( int * ) ptr ) [i] );
  }
}

static inline
void bswap_hn( void *ptr, size_t size )
{
  for ( size_t i = 0; i < size / sizeof( int ); ++i ) {
    ( ( int * ) ptr ) [i] = htonl( ( ( int * ) ptr ) [i] );
  }
}

int *sqp_handshake( int s, const char **signals, int num_signals )
{
  Header_t head = { .code = htonl( 100 ), .len = htonl( num_signals ) };
  int data_len = 0;
  for ( int i = 0; i < num_signals; ++i ) {
    data_len += strlen( signals[i] ) + 1;
  }
  head.r[1] = htonl( data_len );

  char *data = NULL;
  EC_NULL( data = malloc( data_len ) );

  int ofs = 0;
  for ( int i = 0; i < num_signals; ++i ) {
    strcpy( data + ofs, signals[i] );
    ofs += strlen( signals[i] ) + 1;
  }
  assert( ofs == data_len );

  EC_NEG1( sn_send( s, &head, sizeof( head ) ) );
  EC_NEG1( sn_send( s, data, data_len ) );

  EC_NEG1( sn_recv( s, &head, sizeof( head ) ) );
  EC_NEG1( sn_recv( s, data, num_signals * sizeof( int32_t ) ) );
  bswap_nh( &head, sizeof( head ) );
  bswap_nh( data, num_signals * sizeof( int ) );

  if ( head.code != 101 ) {
    EC_UMSG( "ERR> input task: bad response in handshake (resp code = %d)", head.code );
  }
  if ( head.len != num_signals ) {
    EC_UMSG( "ERR> input task: bad response (%d of %d codes in response)", head.len, num_signals );
  }

  head.code = htonl( 100 );
  head.len = htonl( 0 );
  EC_NEG1( sn_send( s, &head, sizeof( head ) ) );

  return ( int * ) data;
  EC_CLEAN_SECTION(
    if ( data ) free( data );
    return NULL;
  );
}


int sqp_recv( int s, SQPsignal_t *_data, int32_t *N )
{
  // state of channel flags:
  //   CLEAR - new request should be issued to server, no data pending
  //   WAITMORE - there is active request and server has more data which will arrives by
  //               next packet with ordinar header
  //   MIDDLE - local buffer does not comprise all data from channel, next read should be done
  //           according to current header and already received data
  Signal_t data[*N];
  static enum { CLEAR = 0, WAITMORE = 1, MIDDLE = 2 } state = CLEAR;
  static int serial = -1;      // request serial number
  static int subserial = 0;    // number of responce to current request
  // static int req[] = {htonl ( 1 ), 0, 0, 0};
  static HeaderL_t rhead; // responce header

  if ( !( state & MIDDLE ) ) {  // new packet reception
    if ( state == CLEAR ) { // request data from server
      Header_t head = { htonl( 1 ) , {0, 0}, htonl( serial + 1 ) };
      EC_NEG1( sn_send( s, &head, sizeof( head ) ) );
      ++serial;
      subserial = 0;
    }

    // recieve and convert header
    EC_NEG1( sn_recv( s, &rhead, sizeof( rhead ) ) );
    bswap_nh( &rhead, sizeof( rhead ) );

    // check req/resp sequence
    if ( rhead.seq != serial ) {
      fl_log( "WRN> input task: loose packets between %d-%d", serial, rhead.seq );
      state = CLEAR;
      subserial = 0;
    }
    // check multipart resp sequence
    if ( ( state & WAITMORE ) && ( rhead.subseq != subserial ) ) {
      fl_log( "WRN> input task: loose subpackets of multipart #%d: between %d-%d", serial, subserial, rhead.subseq );
    }

    if ( rhead.code == 4 ) { // last packet for request from server
      state = CLEAR;
    } else if ( rhead.code == 3 ) {  // server has more data
      state = WAITMORE;
    } else { // unknown header
      EC_UMSG( "Incorrect response from server (code = %d)", rhead.code );
    }
  }

  if ( rhead.n > *N ) { // buffer doesn't comprise response
    state |= MIDDLE;
    EC_NEG1( sn_recv( s, data, *N * sizeof( Signal_t ) ) );
    rhead.n -= *N;
  } else {
    *N = rhead.n;
    state &= ~MIDDLE;
    EC_NEG1( sn_recv( s, data, *N * sizeof( Signal_t ) ) );
  }

  for ( int i = 0; i < *N; ++i ) {
    _data[i].code = ntohl( data[i].code );
    _data[i].ts.sec = ntohl( data[i].sec );
    _data[i].ts.usec = ntohl( data[i].usec );

    data[i].val = ntohl( data[i].val );
    _data[i].val = * ( ( float * ) &data[i].val );

    _data[i].trust = 0;
  }

  // return code -1 - error, 0 -all data here, 1 - we should be called more
  return state == CLEAR ? 0 : 1;
  EC_CLEAN_SECTION(
    return -1;
  );
}




// @}

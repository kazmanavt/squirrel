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


#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <kz_erch.h>

static
int create_and_bind_sock ( struct addrinfo *ret_ai ) {
  // try IPv4 first others next
  int count = 2, s;
  while ( count-- ) {
    for ( struct addrinfo *ai = ret_ai; ai != NULL; ai = ai->ai_next ) {
      if ( count == 1 && ai->ai_family != AF_INET ) {
        continue;
      }
      if ( count == 0 && ai->ai_family == AF_INET ) {
        continue;
      }
      EC_NEG1 ( s = socket ( ai->ai_family, ai->ai_socktype, ai->ai_protocol ) );
      EC_NEG1 ( bind ( s, ai->ai_addr, ai->ai_addrlen ) );

      return s;
      EC_CLEAN_SECTION (
        ec_print ( "WRN> Fail trying addr %d", count );
        ec_clean ();
      );
    }
  }
  return -1;
}

int sn_dg_socket ( char *iface, char *port ) {
  int s = -1;
  struct addrinfo *ret_ai = NULL, hint;
  if ( iface != NULL && port == NULL ) {
    // fail if only hostname given (not suitable for bind - no port)
    EC_FAIL;
  } else if ( iface == NULL && port == NULL ) {
    // creat unbound socket for outgoing connection
    EC_NEG1 ( s = socket ( AF_INET, SOCK_DGRAM, 0 ) );
  } else {
    //create server socket bound to specified address
    memset ( &hint, 0, sizeof ( hint ) );

    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_DGRAM;
    // hint.ai_flags = ( iface == NULL ? AI_PASSIVE : 0 );
    hint.ai_flags = AI_ADDRCONFIG | ( iface == NULL ? AI_PASSIVE : 0 );

    EC_GAI ( getaddrinfo ( iface, port, &hint, &ret_ai ) );
    EC_NEG1 ( s = create_and_bind_sock ( ret_ai ) );
  }

  return s;

  EC_CLEAN_SECTION (
    if ( ret_ai != NULL ) freeaddrinfo ( ret_ai );
    if ( s != -1 ) close ( s );
      return -1;
    );
}

int sn_stream_socket ( char *iface, char *port ) {
  int s = -1;
  struct addrinfo *ret_ai = NULL, hint;
  if ( iface != NULL && port == NULL ) {
    // fail if only hostname given (not suitable for bind - no port)
    EC_FAIL;
  } else if ( iface == NULL && port == NULL ) {
    // creat unbound socket for outgoing connection
    EC_NEG1 ( s = socket ( AF_INET, SOCK_STREAM, 0 ) );
  } else {
    //create server socket bound to specified address
    memset ( &hint, 0, sizeof ( hint ) );

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    // hint.ai_flags = ( iface == NULL ? AI_PASSIVE : 0 );
    hint.ai_flags = AI_ADDRCONFIG | ( iface == NULL ? AI_PASSIVE : 0 );

    EC_GAI ( getaddrinfo ( iface, port, &hint, &ret_ai ) );
    EC_NEG1 ( s = create_and_bind_sock ( ret_ai ) );
  }

  return s;

  EC_CLEAN_SECTION (
    if ( ret_ai != NULL ) freeaddrinfo ( ret_ai );
    if ( s != -1 ) close ( s );
      return -1;
    );
}

int sn_connect ( int sock, const char *host, const char *port ) {
  if ( host == NULL || port == NULL ) {
    EC_FAIL;
  }

  struct sockaddr_storage gues;
  socklen_t sz = sizeof ( gues );
  EC_NEG1 ( getsockname ( sock, ( struct sockaddr * ) &gues, &sz ) );
  int type;
  sz = sizeof ( int );
  EC_NEG1 ( getsockopt ( sock, SOL_SOCKET, SO_TYPE, &type, &sz ) );

  struct addrinfo hint, *ret_ai = NULL;
  memset ( &hint, 0, sizeof ( hint ) );
  hint.ai_family = gues.ss_family;
  hint.ai_socktype = type;
  EC_GAI ( getaddrinfo ( host, port, &hint, &ret_ai ) );

  EC_NEG1 ( connect ( sock, ret_ai->ai_addr, ret_ai->ai_addrlen ) );

  freeaddrinfo ( ret_ai );

  return 0;

  EC_CLEAN_SECTION (
    if ( ret_ai != NULL ) freeaddrinfo ( ret_ai );
    return -1;
  );
}


int sn_send ( int sock, void *buff, size_t len ) {
  while ( len > 0 ) {
    int rc = -1;
    EC_NEG1 ( rc = send ( sock, buff, len, MSG_NOSIGNAL ) );
    len -= rc;
  }

  return 0;

  EC_CLEAN_SECTION (
    return -1;
  );
}

int sn_recv ( int sock, void *buff, size_t size ) {
  EC_NEG1 ( recv ( sock, buff, size, MSG_WAITALL ) );
  return 0;

  EC_CLEAN_SECTION (
    return -1;
  );
}

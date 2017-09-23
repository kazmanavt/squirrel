#ifndef SIMPLE_NET_H_
#define SIMPLE_NET_H_

#if defined(__cplusplus)
extern "C"  {
#endif

int sn_dg_socket ( char* iface, char* port );

int sn_stream_socket ( char* iface, char* port );

int sn_connect ( int sock, const char* host, const char* port );

int sn_send ( int sock, void * buff, size_t len );

int sn_recv ( int sock, void* buff, size_t size );


#if defined(__cplusplus)
} // extern "C"
#endif

#endif // SIMPLE_NET_H_

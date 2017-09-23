//-#- noe -#-

/*!
  @defgroup proto Протокол передачи технологических сигналов
  @ingroup intern
  @{
*/

/*!
  @file proto.h

  @short декларации типов и функций для передачи технологических сигналов.

  @see @ref proto @n
  @date 01.12.2013
  @author masolkin@gmail.com
*/

#ifndef PROTO_H_
#define PROTO_H_

#include "util.h"


/*!
  @short тип значения технологического сигнала согласно протоколу.
*/
typedef float SQPsigvalue_t;

/*!
  @short тип определяющий структуру технологического сигнала согласно протоколу.
*/
typedef struct SQPsignal_s {
  uint_fast32_t code; /*!< код сигнала уникальный для всех подсистем */
  SQtime_t ts;       /*!< временная метка сигнала */
  SQPsigvalue_t val; /*!< значение сигнала */
  float  trust; /*!< достоверность сигнала */
} SQPsignal_t;


int* sqp_handshake ( int s, const char** signals, int num_signals );
int sqp_recv ( int s, SQPsignal_t* data, int32_t* N );



#endif // PROTO_H_

//! @}
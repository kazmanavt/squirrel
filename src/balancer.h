//-#- noe -#-

/*!
  @defgroup balancer Балансировщик
  @ingroup intern
  @{
*/

/*!
  @file balancer.h
  @short интерфейс балансировщика.

  @see @ref balancer @n
  @date 19.11.2013
  @author masolkin@gmail.com
*/

#ifndef BALANCER_H_
#define BALANCER_H_

#include <algos.h>

//! @short выполяет распределение алгоритмов по контейнерам.
int sqb_balance ( SQAlgoObject_t**, int );

#endif // BALANCER_H_

//! @}
//-#- noe -#-

/*!
  @defgroup worker Контейнер алгоритмов
  @ingroup intern
  @{
*/

/*!
  @file worker.h
  @short Декларации функций относящихся к @ref worker.

  Декларируется функция создания контейнера для подмножества сконфигурированных/загруженных
  алгоритмов.

  @see @ref worker @n
  @date 27.11.2012
  @author masolkin@gmail.com
*/

#ifndef WORKER_H_
#define WORKER_H_

#include <algos.h>

//! @short запуск циклического выполнения алгоритмов контейнера.
int sqw_start_worker ( int pool_id, SQAlgoObject_t** algos, long tic );

#endif // WORKER_H_

//! @}
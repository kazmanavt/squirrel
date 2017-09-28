#ifndef IN_H_
#define IN_H_
/*!
  @defgroup datain Подсистема приема технологических сигналов
  @ingroup intern
  @{
*/
/*!
  @file in.h
  @short Декларация вызова для запуска [задачи приема технологических сигналов](@ref datain).

  @see @ref datain @n
  @date 15.02.2013
  @author masolkin@gmail.com
*/

//! @short запуск задачи приема технологических сигналов
int sqi_run ( const char** signals, int num_signals, long tic, bool fake_input );

//! @}
#endif // IN_H_

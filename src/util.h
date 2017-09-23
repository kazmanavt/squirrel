/*!
  @defgroup util Вызовы общего назначения
  @ingroup intern
  @{
*/

/*!
  @file util.h
  @short Общие функции, типы и пр.

  @em Декларации функций и прочих конструкций применяемых в различных местах @b squirrel.

  Функции декларируемые здесь не имеют побочных эффектов.

  @see @ref util @n
  @date 05.02.2013
  @author masolkin@gmail.com
*/
#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

/*!
  @short функция сравнения для выполнения <tt>qsort/bseasrch</tt> над массивом указателей
  на c-строки.
*/
int compare_cstr ( const void* p1, const void* p2 );


/*!
  @short тип представления времени в @b squirrel.
*/
typedef struct SQtime_s {
  uint32_t sec;  //!< секунды
  uint32_t usec; //!< микросекунды
} SQtime_t;

// #define  SQ_DeadLine_t SQtime_t /* really it is same  */
// #define  SQ_TestData_t  SQtime_t /* really it is same  */


//! @short возвращает текущее время.
SQtime_t sq_time ( void );


//! @short привязывает вызывающий процесс к определенному CPU.
int sq_set_cpu ( int, int );

//! @short калибровка тайминга.
int sq_calibrate ( const char* who, int32_t num_tests, int32_t max_duration );


#endif // UTIL_H_

//! @}

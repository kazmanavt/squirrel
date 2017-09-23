//-#- noe -#-

/*!
  @file fastlog.h
  @~russian
  @short Декларации функций относящихся к @ref fastlog.

  API, декларация которого помещена здесь, предоставляется для любого модуля в
  приложении, а также доступен для использования в подгружаемых библиотеках (плагинах).
  Использование данного API позволяет сократить накладные расходы на операции выдачи
  сообщений пользователю, переложив их на выделенный для этих целей процесс.

  @~english
  @short Here are grouped declarations of functions of @ref fastlog.

  API declared here are provided for all application modules and loadable plugin
  libraries. Use of this API allow to reduce overhead on I/O operations needed to provide
  message logging. It is gained by passing this overhead to dedicated process.

  @~
  @see @ref fastlog @n
  @date 20.10.2013
  @author masolkin@gmail.com
*/

#ifndef FASTLOG_H_
#define FASTLOG_H_


//! @~russian @short инициализация процесса логирования и дополнительных ресурсов.
//! @~english @short initialize fastlog facilities & logger process.
int fl_init( void );

/*!
  @~russian @short для вызова при завершении приложения, останавливает процесс логирования.
  @~english @short should be called on application compleation to stop logger process.
*/
void fl_fin( void );


//! @~russian @short указатель на функцию для отсылки сообщений.
//! @~english @short points to message log function.
extern int ( *fl_log )( const char *fmt, ... );

//! @~russian @short указатель на функцию сброса в лог автоматически сгенерированных (@ref error) сообщений об ошибках.
//! @~english @short points to function dropping autogenerated error trace messages (@ref  error) to log.
extern int ( *fl_log_ec )( void );

#endif // FASTLOG_H_
//-#- noe -#-

/*!
  @file kz_erch.h

  @~russian
  @short декларация инструментов для проверки стандартных ошибок.

  Здесь содержатся макросы и декларации функций предназначенные для облегчения написания
  кода проверки ошибок. Они позволяют сократить основной код и сделать его более читаемым.

  @~english
  @short instrumentation for standart error handling.

  Here is placed macro definitions and function declarations intended to simplify
  process of writing error checking code. It helps to keep main code short and clear.

  @~
  @see @ref error
  @date 17.09.2013
  @author masolkin@gmail.com
*/

#ifndef KZ_ERCH_H_
#define KZ_ERCH_H_

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


#define DO(code...) \
  {\
    code ;\
  }

#include <assert.h>
#include <errno.h>
#include <stdbool.h>

/*!
  @internal
  @~russian
  @short размер стека сообщений об ошибках.

  @~english
  @short size of error messages stack.

  @~
  @details @attention internam docs
*/
#define EC_MSG_MAX 100

/*!
  @internal
  @~russian
  @short используя ету глобальную переменную мы предотвращаем использование макросов в секции очистки.

  @~english
  @short by use of this global we avoid of use of error checking macros in cleanup section.

  @~
  @details @attention internam docs
*/
extern bool ec_in_cleanup;

/*!
  @internal
  @~russian
  @short тип кода ошибки.

  @details
  Определяет значение кода ошибки передаваемого функции ec_add().


  @~english
  @short error code type.

  @details
  It helps to clarify error code value, that is passed to ec_add() call.

  @~ @attention internam docs
*/
typedef enum ECtype_t {
  ECESYS, /*!< @~russian код ошибки соответствует @c errno
          @~english error code is @c errno value */
  ECEGAI /*!< @~russian код возврата @c getaddrinfo()
          @~english @c getaddrinfo() return code */
} ECtype_t;

//! @~russian @short добавить сообщение к стеку сообщений об ошибках.
//! @~english @short adds item on error messages stack.
void ec_add( const char *func, const char *fname, int lnum, const char *line,
             int ecerrno, ECtype_t type );
//! @~russian @short добавить сообщение к стеку сообщений об ошибках.
//! @~english @short adds item on error messages stack.
void ec_add_str( const char *func, const char *fname, int lnum,
                 const char *line, const char *fmt, ... );


//! @~russian @short распечатать стек сообщений об ошибках в @c stderr с дополнительным сообщением.
//! @~english @short print error messages stack to @c STDERR with arbitrary message.
void ec_print( const char *fmt, ... );
//! @~russian @short распечатать стек сообщений об ошибках в файл.
//! @~english @short print error messages stack to file.
int ec_print_fd( int fd );
//! @~russian @short распечатать стек сообщений об ошибках в буфер.
//! @~english @short print error messages stack to buffer.
int ec_print_str( char *buff, size_t sz );
//! @~russian @short очистить стек сообщений об ошибках.
//! @~english @short clean error messages stack.
void ec_clean( void );

//! @~russian @short отмечает <b>@em начало</b> секции обработки ошибок в коде приложения.
//! @~english @short denotes <b>@em begining</b> of section containing error handling code of application.
/*#define EC_CLEAN_BEG \
  ec_cleanup: {\
    bool ec_in_cleanup = true;\
    assert(ec_in_cleanup);
*/
#define EC_CLEAN_SECTION(actions...)\
  ec_cleanup: {\
    bool ec_in_cleanup = true;\
    assert(ec_in_cleanup);\
    actions ;\
  }

/*! @~russian @short отмечает <b>@em конец</b> секции обработки ошибок в коде приложения.
! @~english @short denotes <b>@em end</b> of section containing error handling code of application.
#define EC_CLEAN_END\
  }
*/
/*!
  @~russian
  @short макрос проверки ошибок общего назначения.
  @details
  Выражение задаваемле аргументом @c pred вычисляется, результат сравнивается с аргументом @c cond
  с помощю операции @c op. Если результат сравнения положительный (что считается признаком ошибочной
  ситуации), то сообщение об ошибке с помощью вызова ec_add() заносится в стек сообщений об ошибках,
  причем информация о номере строки, исходном файле, названии функции сгенерировавшей ошибочную
  ситуацию и самой строке (в виде текстового значения @c pred) - сохраняется.
  @param pred выражение которое должно подвергнутся проверке
  @param cond индикатор ошибочного результата
  @param op бинарная операция сравнения, которая должна быть использована. (Возможные значения:
             >, <, ==, >=, <=, !=)

  @~english
  @short general purpose error checking macro.
  @details
  Result of evaluation of expression given by @c pred argument is compared to condition given by @c cond
  argument, using @c op operation. If comparision give @c true result (that is considered as error
  situation), than error message is added on error messages stack by call to ec_add(). Information on the
  line number, source file, function that generates error situation and self string of code (as text
  value of @c pred) is saved.
  @param pred expression to be checked
  @param cond sign of error situation
  @param op binary comparision operation that should be used. (Possible operation:
             >, <, ==, >=, <=, !=)
*/
#define EC_CHCK0(pred, cond, op) \
  do {\
    assert(!ec_in_cleanup);\
    if ( (intptr_t)(pred) op (intptr_t)(cond) ) {\
      ec_add ( __func__, __FILE__, __LINE__, #pred, errno, ECESYS );\
      goto ec_cleanup;\
    }\
  } while(0)

//! @~russian @short краткий вызов <tt>@ref EC_CHCK0(pred, cond, ==)</tt>.
//! @~english @short shortcut for <tt>@ref EC_CHCK0(pred, cond, ==)</tt>.
#define EC_CHCK(pred, cond) EC_CHCK0(pred, cond, ==)

/*!
  @~russian @short макрос для проверки функций устанавливающих @c errno, но не имеющих однозначно
  ошибочного кода возврата.
  @~english @short checking macro for functions that sets @c errno, but douesn't have definite return
  code for error situation.
*/
#define EC_ERRNO(pred)\
  do {\
    assert(!ec_in_cleanup);\
    errno = 0;\
    (pred) ;\
    if ( errno != 0 ) {\
      ec_add (__func__, __FILE__, __LINE__, #pred, errno, ECESYS);\
      goto ec_cleanup;\
    }\
  } while(0)

/*!
  @~russian
  @short макрос для проверки функций возвращающих @c errno.
  @details
  Возвращаемое значение некоторых вызовов соответствует кодам ошибок переменной @c errno.
  При этом сама переменная @c errno не устанавливается. Для таких случаев и предназначен
  описываемый проверочный макрос.

  @~english
  @short checking macro for functions that returns @c errno.
  @details
  Return value of some functions corresponds to @c errno codes, though they doesn't set @c errno.
  Described macro intended for use with such functions.
*/
#define EC_RC(pred)\
  do {\
    assert(!ec_in_cleanup);\
    int errn;\
    if ( (errn = (pred)) != 0 ) {\
      ec_add (__func__, __FILE__, __LINE__, #pred, errn, ECESYS);\
      goto ec_cleanup;\
    }\
  } while(0)

//! @~russian @short макрос для проверки функции @c getaddrinfo().
//! @~english @short macro to handle error from @c getaddrinfo() function.
#define EC_GAI(pred) \
  do {\
    assert(!ec_in_cleanup);\
    int errn;\
    if ( (errn = (pred)) != 0 ) {\
      ec_add ( __func__, __FILE__, __LINE__, #pred, errn, ECEGAI );\
      goto ec_cleanup;\
    }\
  } while(0)

//! @~russian @short макрос для проверки функций семейства @c dlopen().
//! @~english @short macro to handle error from function of @c dlopen() famaly.
#define EC_DL(pred) \
  do {\
    assert(!ec_in_cleanup);\
    dlerror ();\
    (pred) ;\
    char *errn = dlerror ();\
    if (errn != NULL) {\
      ec_add_str ( __func__, __FILE__, __LINE__, #pred, errn );\
      goto ec_cleanup;\
    }\
  } while(0)

//! @~russian @short краткая форма для <tt>@ref EC_CHCK(pred, -1)</tt>.
//! @~english @short shortcut to <tt>@ref EC_CHCK(pred, -1)</tt>.
#define EC_NEG1(pred) EC_CHCK(pred, -1)

//! @~russian @short краткая форма для <tt>@ref EC_CHCK(pred, NULL)</tt>.
//! @~english @short shortcut to <tt>@ref EC_CHCK(pred, NULL)</tt>.
#define EC_NULL(pred) EC_CHCK(pred, NULL)

//! @~russian @short краткая форма для <tt>@ref EC_CHCK(pred, EOF)</tt>.
//! @~english @short shortcut to <tt>@ref EC_CHCK(pred, EOF)</tt>.
#define EC_EOF(pred) EC_CHCK(pred, EOF)

//! @~russian @short краткая форма для <tt>@ref EC_CHCK0(pred, 0, !=)</tt>.
//! @~english @short shortcut to <tt>@ref EC_CHCK0(pred, 0, !=)</tt>.
#define EC_NZERO(pred) EC_CHCK0(pred, 0, !=)

//! @~russian @short безусловная передача управления в секцию очистки, с сообщением о месте вызова макроса.
//! @~english @short unconditional transition to cleanup section, with message about place where macro was triggered.
#define EC_FAIL EC_CHCK(0, 0)

/*!
  @~russian
  @short определяемая пользователем ошибка с расширенным сообщением.
  @details
  Аргументы для данного макроса аналогичны аргументам функции @c printf.

  @~english
  @short user defined fail with arbitrary message.
  @details
  This macro takes arguments the same way as @c printf does.
*/
#define EC_UMSG(msg...) \
  do {\
    assert(!ec_in_cleanup);\
    ec_add_str ( __func__, __FILE__, __LINE__, "on error", msg );\
    goto ec_cleanup;\
  } while(0)

//! @~russian @short безусловная передача управления в секцию очистки (без сообщений).
//! @~english @short unconditional transition to cleanup section, (without message).
#define EC_CLEAN goto ec_cleanup;


#define EC_REALLOC(ptr, new_size) \
  {\
     void* tmp = NULL;\
    EC_NULL ( tmp = realloc ( ptr, new_size ) );\
    ptr = tmp;\
  }


#endif // KZ_ERCH_H_

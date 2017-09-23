//-#- noe -#-

/*!
  @~russian
  @page error Обработка ошибок

  В процессе написания любого приложения всегда возникает необходимость написания
  кода проверяющего возвращаемые значения системных вызовов, библиотечных функций
  и других вызовов реализованных в приложении. Описываемый микрофрэймворк позволяет
  сократить время и количество кода необходимые для выполнения этой задачи. Помимо прочего
  использование описываемых возможностей делает код более читаемым.

  Для непосредственного использования в приложении предназначается ряд макросов представленных
  в заголовочном файле @ref kz_erch.h. Там же продекларированы некоторые пользовательские
  функции, их реализация находится в файле @ref kz_erch.c.

  Идея заключается в том, что вызовы требующие проверок оборачиваются в макроопределения. Эти
  макроопределения порождают проверочный код и вызов служебных функций, сохраняющих сообщения об
  ошибке в стеке сообщений об ошибках. Последним действием проверочных макросов является переход
  к секции очистки и обработки ошибок. Эта секция должна располагаться в конце тела функции
  использующей данные проверки. Тело секции должно быть заключено между двумя макросами @ref
  EC_CLEAN_BEG и @ref EC_CLEAN_END.

  В любом месте программы стек ошибок может быть распечатан или передан в буфер функциями
  ec_print(), ec_print_fd() и ec_print_str(). Так же он будет распечатан при завершении
  процесса использующего данный API.


  @~english
  @page error Error checking

  In process of creation application code always exists needs to write code responsible for
  checking of return values of system calls, library functions and other calls implemented by
  application itself. Described microframework allows to reduce time and amount of code
  to be spend for this work. Also use of it makes application code more clear.

  Set of macro definition placed in header file @ref kz_erch.h is intended for direct
  use in application. Also mentioned header hold declaration of some users functions implementation
  of which is placed in @ref kz_erch.c.

  Offered practice is that calls that needs to be checked should be wrapped by macro definitions.
  That macros generates suitable checking code and calls to utility functions to save error message
  to error message stack. The last performed action is to pass control to error handling & cleanup
  section. This section should be located on the end of body of function where this error checking
  mechanism is used. Cleanup section should be placed between two special macros: @ref
  EC_CLEAN_BEG and @ref EC_CLEAN_END.

  Error messages stack can be printed out or to string buffer by call to ec_print(),
  ec_print_fd() or ec_print_str(). Also this stack will be printed out on process that use of
  this API exit.

  @~
  @see
  @ref kz_erch.h @n
  @ref kz_erch.c
*/
/*!
  @file kz_erch.c
  @~russian @short реализация функций для @ref error "обработки ошибок".
  @~english @short @ref error functions implementation.

  @~
  @details
  @see @ref error @n
  @date 17.09.2013
  @author masolkin@gmail.com
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


// #include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

// for getaddrinfo errors handling

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>

#include <kz_erch.h>


// short global variable to determine if we out of error cleanup section
bool ec_in_cleanup = false;

/*!
  @var ec_errcodes
  @internal @attention internam docs
  @short
  @~russian массив с символическими именами кодов для @c errno.
  @~english array of @c errno symbolyc codes.
*/
static struct {
  int errn;
  char *symbol;
} ec_errcodes[] = {
  { EPERM,  "EPERM" },
  { ENOENT,  "ENOENT" },
  { ESRCH,  "ESRCH" },
  { EINTR,  "EINTR" },
  { EIO,  "EIO" },
  { ENXIO,  "ENXIO" },
  { E2BIG,  "E2BIG" },
  { ENOEXEC,  "ENOEXEC" },
  { EBADF,  "EBADF" },
  { ECHILD,  "ECHILD" },
  { EAGAIN,  "EAGAIN" },
  { ENOMEM,  "ENOMEM" },
  { EACCES,  "EACCES" },
  { EFAULT,  "EFAULT" },
  { ENOTBLK,  "ENOTBLK" },
  { EBUSY,  "EBUSY" },
  { EEXIST,  "EEXIST" },
  { EXDEV,  "EXDEV" },
  { ENODEV,  "ENODEV" },
  { ENOTDIR,  "ENOTDIR" },
  { EISDIR,  "EISDIR" },
  { EINVAL,  "EINVAL" },
  { ENFILE,  "ENFILE" },
  { EMFILE,  "EMFILE" },
  { ENOTTY,  "ENOTTY" },
  { ETXTBSY,  "ETXTBSY" },
  { EFBIG,  "EFBIG" },
  { ENOSPC,  "ENOSPC" },
  { ESPIPE,  "ESPIPE" },
  { EROFS,  "EROFS" },
  { EMLINK,  "EMLINK" },
  { EPIPE,  "EPIPE" },
  { EDOM,  "EDOM" },
  { ERANGE,  "ERANGE" },
  { EDEADLK,  "EDEADLK" },
  { ENAMETOOLONG,  "ENAMETOOLONG" },
  { ENOLCK,  "ENOLCK" },
  { ENOSYS,  "ENOSYS" },
  { ENOTEMPTY,  "ENOTEMPTY" },
  { ELOOP,  "ELOOP" },
  { EWOULDBLOCK,  "EWOULDBLOCK" },
  { ENOMSG,  "ENOMSG" },
  { EIDRM,  "EIDRM" },
  { ECHRNG,  "ECHRNG" },
  { EL2NSYNC,  "EL2NSYNC" },
  { EL3HLT,  "EL3HLT" },
  { EL3RST,  "EL3RST" },
  { ELNRNG,  "ELNRNG" },
  { EUNATCH,  "EUNATCH" },
  { ENOCSI,  "ENOCSI" },
  { EL2HLT,  "EL2HLT" },
  { EBADE,  "EBADE" },
  { EBADR,  "EBADR" },
  { EXFULL,  "EXFULL" },
  { ENOANO,  "ENOANO" },
  { EBADRQC,  "EBADRQC" },
  { EBADSLT,  "EBADSLT" },
  { EDEADLOCK,  "EDEADLOCK" },
  { EBFONT,  "EBFONT" },
  { ENOSTR,  "ENOSTR" },
  { ENODATA,  "ENODATA" },
  { ETIME,  "ETIME" },
  { ENOSR,  "ENOSR" },
  { ENONET,  "ENONET" },
  { ENOPKG,  "ENOPKG" },
  { EREMOTE,  "EREMOTE" },
  { ENOLINK,  "ENOLINK" },
  { EADV,  "EADV" },
  { ESRMNT,  "ESRMNT" },
  { ECOMM,  "ECOMM" },
  { EPROTO,  "EPROTO" },
  { EMULTIHOP,  "EMULTIHOP" },
  { EDOTDOT,  "EDOTDOT" },
  { EBADMSG,  "EBADMSG" },
  { EOVERFLOW,  "EOVERFLOW" },
  { ENOTUNIQ,  "ENOTUNIQ" },
  { EBADFD,  "EBADFD" },
  { EREMCHG,  "EREMCHG" },
  { ELIBACC,  "ELIBACC" },
  { ELIBBAD,  "ELIBBAD" },
  { ELIBSCN,  "ELIBSCN" },
  { ELIBMAX,  "ELIBMAX" },
  { ELIBEXEC,  "ELIBEXEC" },
  { EILSEQ,  "EILSEQ" },
  { ERESTART,  "ERESTART" },
  { ESTRPIPE,  "ESTRPIPE" },
  { EUSERS,  "EUSERS" },
  { ENOTSOCK,  "ENOTSOCK" },
  { EDESTADDRREQ,  "EDESTADDRREQ" },
  { EMSGSIZE,  "EMSGSIZE" },
  { EPROTOTYPE,  "EPROTOTYPE" },
  { ENOPROTOOPT,  "ENOPROTOOPT" },
  { EPROTONOSUPPORT,  "EPROTONOSUPPORT" },
  { ESOCKTNOSUPPORT,  "ESOCKTNOSUPPORT" },
  { EOPNOTSUPP,  "EOPNOTSUPP" },
  { EPFNOSUPPORT,  "EPFNOSUPPORT" },
  { EAFNOSUPPORT,  "EAFNOSUPPORT" },
  { EADDRINUSE,  "EADDRINUSE" },
  { EADDRNOTAVAIL,  "EADDRNOTAVAIL" },
  { ENETDOWN,  "ENETDOWN" },
  { ENETUNREACH,  "ENETUNREACH" },
  { ENETRESET,  "ENETRESET" },
  { ECONNABORTED,  "ECONNABORTED" },
  { ECONNRESET,  "ECONNRESET" },
  { ENOBUFS,  "ENOBUFS" },
  { EISCONN,  "EISCONN" },
  { ENOTCONN,  "ENOTCONN" },
  { ESHUTDOWN,  "ESHUTDOWN" },
  { ETOOMANYREFS,  "ETOOMANYREFS" },
  { ETIMEDOUT,  "ETIMEDOUT" },
  { ECONNREFUSED,  "ECONNREFUSED" },
  { EHOSTDOWN,  "EHOSTDOWN" },
  { EHOSTUNREACH,  "EHOSTUNREACH" },
  { EALREADY,  "EALREADY" },
  { EINPROGRESS,  "EINPROGRESS" },
  { ESTALE,  "ESTALE" },
  { EUCLEAN,  "EUCLEAN" },
  { ENOTNAM,  "ENOTNAM" },
  { ENAVAIL,  "ENAVAIL" },
  { EISNAM,  "EISNAM" },
  { EREMOTEIO,  "EREMOTEIO" },
  { EDQUOT,  "EDQUOT" },
  { ENOMEDIUM,  "ENOMEDIUM" },
  { EMEDIUMTYPE,  "EMEDIUMTYPE" },
  { ECANCELED,  "ECANCELED" },
  { ENOKEY,  "ENOKEY" },
  { EKEYEXPIRED,  "EKEYEXPIRED" },
  { EKEYREVOKED,  "EKEYREVOKED" },
  { EKEYREJECTED,  "EKEYREJECTED" },
  { EOWNERDEAD,  "EOWNERDEAD" },
  { ENOTRECOVERABLE,  "ENOTRECOVERABLE" },
  { ERFKILL,  "ERFKILL" },
  { 0, "UNKNOWN_ERROR" }
};

/*!
  @var ec_gaicodes
  @internal @attention internam docs
  @short
  @~russian массив символических кодов ошибок для функции @c getaddrinfo.
  @~english array of symbolyc error codes for @c getaddrinfo func.
*/
struct {
  int errn;
  const char *symbol;
} ec_gaicodes[] = {
  { EAI_BADFLAGS, "EAI_BADFLAGS" },
  { EAI_NONAME, "EAI_NONAME" },
  { EAI_AGAIN, "EAI_AGAIN" },
  { EAI_FAIL, "EAI_FAIL" },
  { EAI_FAMILY, "EAI_FAMILY" },
  { EAI_SOCKTYPE, "EAI_SOCKTYPE" },
  { EAI_SERVICE, "EAI_SERVICE" },
  { EAI_MEMORY, "EAI_MEMORY" },
  { EAI_SYSTEM, "EAI_SYSTEM" },
  { EAI_OVERFLOW, "EAI_OVERFLOW" },
#ifdef __USE_GNU
  { EAI_NODATA, "EAI_NODATA" },
  { EAI_ADDRFAMILY, "EAI_ADDRFAMILY" },
  { EAI_INPROGRESS, "EAI_INPROGRESS" },
  { EAI_CANCELED, "EAI_CANCELED" },
  { EAI_NOTCANCELED, "EAI_NOTCANCELED" },
  { EAI_ALLDONE, "EAI_ALLDONE" },
  { EAI_INTR, "EAI_INTR" },
  { EAI_IDN_ENCODE, "EAI_IDN_ENCODE" },
#endif
  { 0, "UNKNOWN_ERROR" }
};


/*!
  @internal
  @~russian @short блок памяти для стека сообщений об ошибках.
  @~english @short static memory block, for error messages stack.
  @~ @details @attention internam docs
*/
char msgs[EC_MSG_MAX + 1][LINE_MAX];
/*!
  @internal
  @~russian @short количество ошибок в стеке.
  @~english @short number of errors in stack.
  @~ @details @attention internam docs
*/
static int msg_n = 0;


/*!
  @internal
  @~russian
  @short очистка на выходе из процесса.

  Данная функция регистрируется библиотечным вызовом @c atexit, и должна быть вызвана
  при завершении процесса, чтобы распечатать стек сообщений об ошибках, если он содержит
  сообщения.

  @~english
  @short finalize on process exit.

  It is registered by @c atexit library call, and shold be called on process exit to
  print error messages stack if it has some messages.

  @~ @attention internam docs
*/
static void ec_atexit( void )
{
  if ( msg_n != 0 )
  { ec_print( "EMG[%d]> FIN!\n", getpid() ); }
}

/*!
  @internal
  @~russian @short флаг показывающий состояние регистрации финализатора ec_atexit().
  @~english @short flag denoting state of registration of finalizer ec_atexit().
  @~ @details @attention internam docs
*/
static bool registered = false;
static pid_t pid = -1;

/*!
  @details
  @~russian
  Добавляет сообщение в стек сообщений об ошибках. Функция получает расширенное сообщение
  об ошибке через вызовы @c strerror или @c gai_strerror, в зависимости от аргумента
  @c type.
  @param[in] func    имя функции, в теле которой произошло обнаружение ошибки.
  @param[in] fname   имя исходного файла в коде которого произошла ошибка.
  @param[in] lnum    номер строки в коде где была сгенерирована ошибка.
  @param[in] line    сама строка кода вызвавшего ошибку.
  @param[in] ecerrno код ошибки, интерпретируется в соответствии зо значением аргумента
                     @c type.
  @param[in] type    определяет значение параметра @c ecerrno:
                     @arg ECESYS @c ecerrno инициализировано значением переменной @c errno.
                     @arg ECEGAI @c ecerrno является кодом возврата функции @c getaddrinfo.

  @~english
  Adds message containing information on error to error messages stack. It will retrieve
  extended information on error happened, by calling @c strerror or @c gai_strerror,
  in dependance of @c type argument.
  @param[in] func    function name where error was detected.
  @param[in] fname   source file containing code where error was detected.
  @param[in] lnum    error detected at this line number.
  @param[in] line    of code which produce the error.
  @param[in] ecerrno error code. It will be interpreted according to @c type argument value.
  @param[in] type    clarify @c ecerrno paarameter:
                     @arg ECESYS @c ecerrno initialized from errno.
                     @arg ECEGAI @c ecerrno is getaddrinfo return code.
*/
void ec_add( const char *func, const char *fname, int lnum, const char *line,
             int ecerrno, ECtype_t type )
{
  // register atexit handler if not yet
  if ( !registered ) {
    atexit( ec_atexit );
    registered = true;
    pid = getpid();
  }

  // check for error messages stack overflow
  if ( msg_n >= EC_MSG_MAX + 1 ) {
    strcpy( msgs[EC_MSG_MAX], "  >> TRUNCATED <<\n" );
    return;
  }

  // format message to look like:
  // function() [source:line] code_line
  //     ERROR_SYMBOL (ERROR_CODE: error message)
  int count = LINE_MAX;
  if ( ( count -= snprintf( msgs[msg_n], LINE_MAX, "%s() [%s:%d] %s\n", func,
                            fname,
                            lnum, line ) ) <= 0 ) {
    msgs[msg_n][LINE_MAX - 3] = '>';
    msgs[msg_n++][LINE_MAX - 2] = '\n';
    return;
  }
  if ( ecerrno != 0 ) {
    int i;
    if ( type == ECESYS ) {
      for ( i = 0; ec_errcodes[i].errn != 0; ++i )
        if ( ec_errcodes[i].errn == ecerrno )
        { break; }
      count -= snprintf( msgs[msg_n] + LINE_MAX - count, count, "    %s (%d: %s)\n",
                         ec_errcodes[i].symbol, ec_errcodes[i].errn, strerror( ecerrno ) );
    } else if ( type == ECEGAI ) {
      for ( i = 0; ec_gaicodes[i].errn != 0; ++i )
        if ( ec_gaicodes[i].errn == ecerrno )
        { break; }
      count -= snprintf( msgs[msg_n] + LINE_MAX - count, count, "    %s (%d: %s)\n",
                         ec_gaicodes[i].symbol, ec_gaicodes[i].errn,
                         gai_strerror( ecerrno ) );
    }
    if ( count <= 0 ) {
      msgs[msg_n][LINE_MAX - 3] = '>';
      msgs[msg_n][LINE_MAX - 2] = '\n';
    }
  }
  ++msg_n;
}

/*!
  @details
  @~russian
  Добавляет сообщение в стек сообщений об ошибках. Функция получает расширенное сообщение
  об ошибке через аргумент @c errstr.
  @param[in] func    имя функции, в теле которой произошло обнаружение ошибки.
  @param[in] fname   имя исходного файла в коде которого произошла ошибка.
  @param[in] lnum    номер строки в коде где была сгенерирована ошибка.
  @param[in] line    сама строка кода вызвавшего ошибку.
  @param[in] fmt     формат для печати последующих аргументов функции, которые вместе должны
                     сформировать расширенное сообщение об ошибке. Строка формата должна соотвовать
                     правилам написания формата для стандартных функций семейства @c printf.

  @~english
  Adds message containing information on error to error messages stack. It will retrieve
  extended information on error happened, by calling @c strerror or @c gai_strerror,
  in dependance of @c type argument.
  @param[in] func    function name where error was detected.
  @param[in] fname   source file containing code where error was detected.
  @param[in] lnum    error detected at this line number.
  @param[in] line    of code which produce the error.
  @param[in] fmt     format for printing of consiquent arguments, which all togather should form
                     extended error message. The format string should conform to the rules defined
                     for the format string rules of library functions of @c printf famaly.
*/
void ec_add_str( const char *func, const char *fname, int lnum,
                 const char *line, const char *fmt, ... )
{
  // register atexit handler if not yet
  if ( !registered ) {
    atexit( ec_atexit );
    registered = true;
    pid = getpid();
  }

  // check for error messages stack overflow
  if ( msg_n >= EC_MSG_MAX + 1 ) {
    strcpy( msgs[EC_MSG_MAX], "  >> TRUNCATED <<\n" );
    return;
  }

  // format message to look like:
  // function() [source:line] code_line
  //     ERROR_SYMBOL (ERROR_CODE: error message)
  long count = LINE_MAX;
  if ( ( count -= snprintf( msgs[msg_n], LINE_MAX, "%s() [%s:%d] %s\n", func,
                            fname, lnum, line ) ) <= 0 ) {
    msgs[msg_n][LINE_MAX - 3] = '>';
    msgs[msg_n++][LINE_MAX - 2] = '\n';
    return;
  }

  if ( ( count -= snprintf( msgs[msg_n] + LINE_MAX - count, count, "    " ) ) <= 0 ) {
    msgs[msg_n][LINE_MAX - 3] = '>';
    msgs[msg_n++][LINE_MAX - 2] = '\n';
    return;
  }

  va_list ap;
  va_start( ap, fmt );
  if ( ( count -= vsnprintf( msgs[msg_n] + LINE_MAX - count, count, fmt,
                             ap ) ) <= 0 ) {
    msgs[msg_n][LINE_MAX - 3] = '>';
    msgs[msg_n++][LINE_MAX - 2] = '\n';
    return;
  }
  va_end( ap );

  if ( snprintf( msgs[msg_n] + LINE_MAX - count, count, "\n" ) >= count ) {
    msgs[msg_n][LINE_MAX - 3] = '>';
    msgs[msg_n][LINE_MAX - 2] = '\n';
  }

  ++msg_n;
}


/*!
  @details
  @~russian
  Печатает содержимое стека сообщений об ошибках в файл дескриптор которого передается
  аргументом @c fd. После завершения этого вызова - стек пуст.
  @param[in] fd дескриптор файл в который необходимо произвести печать.
  @retval -1 при ошибочном завершении.
  @retval 0 в случае нормального завершения.

  @~english
  Prints all messages currently in stack to specefied file descriptor @c fd.
  After call to this function, message stack is empty.
  @param[in] fd file descriptor to print messages to.
  @retval -1 on error.
  @retval 0 on success.
*/
int ec_print_fd( int fd )
{
  int total = msg_n - 1;
  while ( msg_n-- ) {
    if ( dprintf( fd, " %d: %s", total - msg_n, msgs[msg_n] ) < 0 ) {
      msg_n = 0;
      return -1;
    }
  }
  msg_n = 0;
  return 0;
}



/*!
  @details
  @~russian
  Печатает содержимое стека сообщений об ошибках в буфер @c buff размера @c sz.
  Результирующая строка не будет превышать размер заданный параметром @c sz, включая
  завершающий символ @c '\0'. Если размер области @c buff недостаточен, в конец помещается
  признак урезания сообщений - строка <tt>"\nBTR"</tt>.

  После завершения этого вызова - стек пуст.
  @param[in,out] buff указатель области памяти куда необходимо вывести стек сообщений
                      об ошибках.
  @param[in]     sz   размер области памяти на которую указывает @c buff. При
                      <tt>sz < 10</tt> вызов гарантировано вернет @c -1.
  @retval -1 при ошибочном завершении.
  @retval n количество переданных символов.

  @~english
  Prints all messages currently in stack to specefied buffer @c buff of size @c sz.
  Resulting string will not exceed size specefied by @c sz, including terminating
  nulll-character. If @c buff has insufficient size, string <tt>"\nBTR"</tt> denoting
  truncation is placed at the end.

  After call to this function, message stack is empty.
  @param[in,out] buff points to memore area, where error messages stack should be printed.
  @param[in]    sz  size of buffer pointed by @c buff. If <tt>sz < 10</tt> this function
  will unconditionaly returns @c -1.
  @retval -1 on error.
  @retval n number of printed characters.
*/
int ec_print_str( char *buff, size_t _sz )
{
  long sz = _sz;
  if ( sz < 10 ) { return -1; }

  for ( int i = msg_n; i > 0; --i ) {
    sz -= snprintf( buff + _sz - sz, sz, "  %d: %s", msg_n - i, msgs[i - 1] );
    if ( sz <= 0 ) {
      strcpy( buff + _sz - 5, "\nBTR" );
      msg_n = 0;
      return -1;
    }
  }
  msg_n = 0;
  return _sz - sz;
}

//  clean message stack
void ec_clean( void )
{
  msg_n = 0;
}

/*!
  @details
  @~russian
  Распечатывает стек сообщений об ошибках в стандартный вывод ошибок, предваряя его
  пользовательсуим сообщением.
  По входным аргументам функция полностью соответствует библиотечной функции @c printf (C89).
  @param[in] fmt строка формата.

  @~english
  Prints error messages stack to stderror, prepending it with user message.
  Function is fully equivalent to library function @c printf (C89).
  @param[in] fmt format string.
*/
void ec_print( const char *fmt, ... )
{
  va_list ap;

  va_start( ap, fmt );
  vfprintf( stderr, fmt, ap );
  va_end( ap );

  ec_print_fd( STDERR_FILENO );
}


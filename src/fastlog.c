//-#- noe -#-

/*!
  @~russian
  @page fastlog API логирования

  Данный API предназначен для сокращения задержек в операциях вывода предупреждений,
  сообщений информационного характера и сообщений об ошибках. Предназначенные для
  использования вызовы продекларированны в @ref fastlog.h и реализованы в @ref fastlog.c.

  При инициализации запускается выделенный @em логирующий процесс. Этот процесс
  отвечает за вывод переданных сообщений в консоль. Приложение имеет возможность
  передавать сообщения процессу логирования в неблокирующем режиме.

  Такое поведение достигается посредством предоставления выделенной области
  разделяемой памяти, где приложение может разместить передаваемое сообщение
  очень быстро. После размещения сообщения в разделяемой памяти, дескриптор
  этой области разделяемой памяти помещается во входящую очередь сообщений
  процесса логирования. Процесс логирования берет на себя все операции по
  выводу сообщения. После окончания работы с сообщением дескриптор области
  разделяемой памяти используемой сообщением передается в очередь свободных
  дескрипторов, тем самым высвобождая разделяемую память для дальнейшего
  использования.

  @~english
  @page fastlog Fastlog API

  This API should be used to minimize latency in logging informational and
  debuging messages. Calls intended for external use are declared in @ref fastlog.h,
  and are implemented in @ref fastlog.c.

  On initialization dedicated @em fastlog process is launched. This process is responsible
  for handling of log-messages sent by application. Application is able to send
  messages to logger process in non-blocking manner.

  Such behavior is gained by providing block of shared memory where application
  may store messages very fast. After that, descriptor of shared memory block which
  contains message is passed to incomming queue of logger process. Logger process is
  responsible to pass message out. After message is logged, descriptor of
  shared memory block consumed by message is passed to queue of free shared
  memory blocks descriptors, and contents of that shared memory block may be
  overwrited.

  @~
  @see
  @ref fastlog.h @n
  @ref fastlog.c
*/
/*!
  @file fastlog.c
  @~russian @short реализация API продекларированного в @ref fastlog.h.
  @~english @short implementation of API declared in @ref fastlog.h.

  @~
  @see @ref fastlog
  @date 20.10.2013
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


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include <kz_erch.h>

#if defined(FL_SHM_NM) || defined(FL_MSG_SZ) || defined(FL_MSGS) || defined(FL_QEMPTY_NM) || defined(FL_QDATA_NM) || defined(FL_MAX_ETYPE)
#error local defines shud not be redefined elsewhere
#endif

/*!
  @internal
  @~russian @short имя разделяемого блока памяти.
  @~english @short name of shared mem resource.

  @~ @attention internal docs
*/
#define FL_SHM_NM "/sq_fastlog.shm"

/*!
  @internal
  @~russian @short максимальный размер сообщения.
  @~english @short maximum message size.

  @~ @attention internal docs
*/
#define FL_MSG_SZ 2048L

/*!
  @internal
  @~russian @short максимальное количество сообщений.
  @~english @short maximum number of messages.

  @~ @attention internal docs
*/
#define FL_MSGS 100L

/*!
  @internal
  @~russian @short имя очереди дескрипторов свободных блоков.
  @~english @short name of queue containing descriptors of free mem blocks.

  @~ @attention internal docs
*/

#define FL_QEMPTY_NM "/sq_fastlog_empty.mq"
/*!
  @internal
  @~russian @short имя очереди дескрипторов переданных сообщений.
  @~english @short name of queue with pending messages.

  @~ @attention internal docs
*/
#define FL_QDATA_NM "/sq_fastlog_data.mq"

/*!
  @internal
  @~russian @short максимальное значе поля @c type дескриптора, для расширенных сообщений.
  @~english @short max value of @c type field of descriptor for extended messages.

  @~ @attention internal docs
*/
#define FL_MAX_ETYPE 65535L

/*!
  @internal
  @~russian
  @short дескриптор сообщения.

  Тип дескриптора пересылаемого процессу логирования.
  @note
  - Для расширенных сообщений со значением `type = 0` процесс логирования игнорирует поле
    @c extra, и просто выводит содержимое ассоциированого блока разделяемой памяти
    в @c stdout без изменений.
  - Сообщение с полем @c type = @ref FL_MAX_ETYPE + 1 зарезервировано и означает
    что все блоки разделяемой памяти заняты и расширенное сообщение не может быть передано.

  @~english
  @short message descriptor.

  Type of descriptor to be sent to logger proccess.
  @note
  - For extended messages of reserved `type = 0` logger proccess ignores value of field
    @c extra and just print contents of associated shared mem block to @c stdout as is.
  - Message of reserved @c type = @ref FL_MAX_ETYPE + 1 designate situation when
    all shared memory blocks are used and extended message can not be passed.

  @~
  @attention internal docs
*/
typedef struct FLmbDescriptor_t {
  pid_t pid; /*!< @~russian идентификатор процесса пославшего сообщение. @~english PID
                  of message sender */
  long type; /*!< @~russian определяет как процесс логирования будет интерпретировать остальные
              поля дескриптора. @~english defines how logger process will interpret other descriptor
              fields. */
  long idx; /*!< @~russian для дескрипторов с полем @c type <= @ref FL_MAX_ETYPE, это поле содержит
            индекс ассоциированного блока разделяемой памяти содержащего дополнительные данные, для
            других типов поле может быть интерпретированно по разному.
            @~english for descriptors which @c type field value <= @ref FL_MAX_ETYPE, @c idx hold
            index of associated shared memory block. For other types this field may be interpreted
            in arbitrary way. */
  long extra; /*!<  @~russian дополнительное поле, всегда интерпретируется на усмотрение разработчика
              @~english auxilary field, ever is interpreted in arbitrary way. */
} FLmbDescriptor_t;

/*!
  @internal
  @~russian @short указатель на сегмент разделяемой прамяти, для сообщений.
  @~english @short pointer to shared memory pool, for messages.

  @~ @attention internal docs
*/
static void *flbuff = MAP_FAILED; // shared mem ptr
/*!
  @internal
  @~russian @short дескриптор очереди свободных дескрипторов блоков разделяемой памяти.
  @~english @short descriptor of queue for free shared mem blocks descriptors.

  @~ @attention internal docs
*/
static mqd_t flempty_q = -1;
/*!
  @internal
  @~russian @short дескриптор очереди с переданными сообщениями.
  @~english @short descriptor of queue with pending messages.

  @~ @attention internal docs
*/
static mqd_t fldata_q = -1;

/*!
  @internal
  @~russian @short PID процесса логирования.
  @~english @short PID of fastlog dedicated process.

  @~ @attention internal docs
*/
static pid_t pid = -1;


int ( *fl_log )( const char *fmt, ... ) = NULL;
int ( *fl_log_ec )( void ) = NULL;




/*!
  @~russian
  @short послать сообщение.
  @details
  Данная функция передает текстовое сообщение в процесс логирования, полю @c type дескриптора
  сообщения присваивается значение @c 0, обозначая тем самым что сообщение
  должно быть выведено в терминал.

  По входным аргументам функция полностью соответствует библиотечной функции @c printf() (C89).
  @param fmt строка формата.
  @retval 0 в случае успеха
  @retval -1 при возникновении ошибки.

  @~english
  @short log message.
  @details
  This function passes text message to logger process. Value of @c 0 is assigned to descriptors
  field @c type, designating that logger should print it to terminal.

  Function is fully equivalent to library function @c printf() (C89).
  @param fmt format string.
  @retval 0 on success
  @retval -1 on error
*/
static int fl_log_extern( const char *fmt, ... )
{
  static pid_t mypid = 0;
  if ( !mypid ) {
    mypid = getpid();
  }

  // get free shared mem block for writing
  const struct timespec to = {0, 0};
  FLmbDescriptor_t msg = {.type = -1};

  EC_NEG1( mq_timedreceive( flempty_q, ( void * ) &msg, sizeof( msg ), NULL, &to ) );
  msg.pid = mypid;

  // print message to shared mem block
  size_t sz = FL_MSG_SZ;
  va_list ap;
  va_start( ap, fmt );
  vsnprintf( flbuff + msg.idx, sz, fmt, ap );
  va_end( ap );

  // send message with descriptor of filled shared mem block to logger process
  EC_NEG1( mq_send( fldata_q, ( void * ) &msg, sizeof( msg ), 0 ) );

  return 0;

  EC_CLEAN_SECTION(
    // try to send notification to logger about ascence of shared mem
  if ( msg.type == -1 ) {
  msg.type = FL_MAX_ETYPE + 1;
  msg.pid = mypid;
  mq_send( fldata_q, ( void * ) &msg, sizeof( msg ), 0 );
  } else {
    mq_send( flempty_q, ( void * ) &msg, sizeof( msg ), 0 );
  }
  ec_print( "[%d] ERR> fastlog failure in wr\n", mypid );
  fprintf( stderr, "[%d] ERR> attempted to print the following:\n", mypid );
  va_list ap;
  va_start( ap, fmt );
  vprintf( fmt, ap );
  va_end( ap );


  return -1;
  );
}


/*!
  @~russian
  @short сбросить в лог автоматически сгенерированные (@ref error) сообщения об ошибках.
  @details
  Этот вызов передаст в процесс логирования буфер сообщений об ошибках,
  если такие были. Предполагается в данном случае использование заголовочного файла
  @ref kz_erch.h
  @retval 0 в случае успеха
  @retval -1 при возникновении ошибки.

  @~english
  @short drop autogenerated error trace messages (@ref  error) to log.
  @details
  This function will dump trace buffers from error checking subsystem if any.
  It implyes that application is using @ref kz_erch.h
  @retval 0 on success
  @retval -1 on error
*/
static int fl_log_ec_extern()
{
  static pid_t mypid = 0;
  if ( !mypid ) {
    mypid = getpid();
  }

  // get free shared mem block for writing
  const struct timespec to = {0, 0};
  FLmbDescriptor_t msg = {.type = -1};

  // get empty descriptor from empty queue
  EC_NEG1( mq_timedreceive( flempty_q, ( void * ) &msg, sizeof( msg ), NULL, &to ) );
  msg.pid = mypid;

  // print error trace to free shared mem block
  int n = sprintf( flbuff + msg.idx, "error stack:\n" );
  ec_print_str( flbuff + msg.idx + n, FL_MSG_SZ - n );

  // send message with descriptor of filled shared mem block to logger process
  EC_NEG1( mq_send( fldata_q, ( void * ) &msg, sizeof( msg ), 0 ) );

  return 0;

  EC_CLEAN_SECTION(
  if ( msg.type == -1 ) {
  msg.type = FL_MAX_ETYPE + 1;
  msg.pid = mypid;
  mq_send( fldata_q, ( void * ) &msg, sizeof( msg ), 0 );
  } else {
    mq_send( flempty_q, ( void * ) &msg, sizeof( msg ), 0 );
  }
  ec_print( "[%d] ERR> fastlog failure in wr\n", mypid );
  return -1;
  );
}





/*!
  @~russian
  @short Резервная функция используемая для подмены fl_log_extern(), после останова процесса
  логирования.

  @~english
  @short Fallback function intended to use as replacement of fl_log_extern() after logger process
  termination.

  @~
  @see fl_log
  @see fl_log_extern()
*/
static int fl_log_local( const char *fmt, ... )
{
  static pid_t mypid = 0;
  if ( !mypid ) {
    mypid = getpid();
  }

  fprintf( stderr, "[%d] WRN> dedicated logger stopped, next message is printed by local thread:\n", mypid );
  va_list ap;
  va_start( ap, fmt );
  vfprintf( stderr, fmt, ap );
  va_end( ap );

  return 0;
}


/*!
  @~russian
  @short Резервная функция используемая для подмены fl_log_ec_extern(), после останова процесса
  логирования.

  @~english
  @short Fallback function intended to use as replacement of fl_log_ec_extern() after logger process
  termination.

  @~
  @see fl_log_ec
  @see fl_log_ec_extern()
*/
static int fl_log_ec_local()
{
  static pid_t mypid = 0;
  if ( !mypid ) {
    mypid = getpid();
  }

  fprintf( stderr, "[%d] WRN> dedicated logger stopped, next message is printed by local thread:\n", mypid );
  ec_print( "" );

  return 0;
}




/*!
  @internal
  @~russian
  @short освобождает внутренние ресурсы процесса логирования.

  Предназначается для вызова в приложении и в процессе логирования при выходе,
  для запуска завершающего кода.

  @~english
  @short func to free shared resources.

  Intended to be called on client & logger process exit to run clean up code.

  @~ @attention internal docs
*/
static void fastlog_release( void )
{
  //! Closes & unlinks message queue.
  if ( fldata_q != -1 ) {
    mq_close( fldata_q );
    if ( pid == 0 ) {
      mq_unlink( FL_QDATA_NM );
    }
    fldata_q = -1;
  }
  //! Closes & unlinks free descriptors queue.
  if ( flempty_q != -1 ) {
    mq_close( flempty_q );
    if ( pid == 0 ) {
      mq_unlink( FL_QEMPTY_NM );
    }
    flempty_q = -1;
  }
  //! Unmaps & unlinks shared memory pool.
  if ( flbuff != MAP_FAILED ) {
    munmap( flbuff, FL_MSGS * FL_MSG_SZ );
  }
  if ( pid == 0 ) {
    shm_unlink( FL_SHM_NM );
  }
}


/*!
  @~russian
  @short константа передаваемая с сигналом `TERM` для вызова штатного завершения.
  @~english
  @short constant passed with `TERM` signal to launch planed termination.
*/
#define FL_PLANNED_TERMINATION 13
/*
  ------------------------------------
  Function to stop logging to subproces
  ------------------------------------
*/
void fl_fin( void )
{
  fprintf( stderr, "[%d] INF> sending SIGTERM to fastlog process...\n", getpid() );
  //! Change handling of ongoing calls to fl_log()
  fl_log = fl_log_local;
  fl_log_ec = fl_log_ec_local;
  //! Run client cleanup.
  fastlog_release();
  //! Finalize logger process.
  union sigval inf = {.sival_int = FL_PLANNED_TERMINATION};
  sigqueue( pid, SIGTERM, inf );
  waitpid( pid, NULL, 0 );
}

/*!
  @~russian
  @short функция процесса логирования.
  @details Тело данной функции представляет собой процесс логирования, отвечающий
  за прием диагностических сообщений передаваемых другими процессами и частями приложения
  для выдачи пользователю.
  @retval 0 в случае успеха
  @retval -1 при возникновении ошибки.

  @~english
  @short logger process function.
  @details Body of this function represents logger process. It responsible for aqcuiring
  messages passed by other processes and parts of application. It arranges these messages
  for user.
  @retval 0 on success
  @retval -1 on error
*/
static int fl_logger()
{
  pid_t mypid = getpid();
  int rc = 1; // exit code (error by def)

  sigset_t mask;
  EC_NEG1( sigemptyset( &mask ) );
  EC_NEG1( sigaddset( &mask, SIGTERM ) );

  // register exit handler to free shared resources
  // EC_NZERO ( atexit (fastlog_release) );

  fprintf( stderr, "[%d] INF> fastlog: logger process started\n", mypid );

  FLmbDescriptor_t msg;
  // receive and print out messages of type 0
  while ( true ) {
    struct timespec to;
    clock_gettime( CONFIGURED_CLOCK, &to );
    to.tv_sec += 5;
    if ( mq_timedreceive( fldata_q, ( void * ) &msg, sizeof( msg ), 0,
                          &to ) == -1 ) {
      if ( errno != ETIMEDOUT && errno != EINTR ) {
        EC_FAIL;
      }
    } else if ( msg.type <= FL_MAX_ETYPE ) {
      fprintf( stderr, "[%d] ", msg.pid );
      if ( msg.type == 0 ) {
        EC_NEG1( write( STDERR_FILENO, flbuff + msg.idx, strlen( flbuff + msg.idx ) ) );
      } else {
        msg.type = 0;
      }
      EC_NEG1( mq_send( flempty_q, ( void * ) &msg, sizeof( msg ), 0 ) );
    } else if ( msg.type == FL_MAX_ETYPE + 1 ) {
      fprintf( stderr, "[%d] WRN> fastlog: some log messages (from [%d]) removed due to overflow\n", mypid, msg.pid );
    }

    if ( kill( getppid(), 0 ) == -1 ) {
      EC_UMSG( "controlling process [%d] stopped, exiting\n", getppid() );
    }

    // sigset_t pend;
    // EC_NEG1 ( sigpending (&pend) );
    // if ( sigismember (&pend, SIGTERM) ) {
    //   siginfo_t info;
    //   EC_NEG1 ( sigwaitinfo (&mask, &info) );
    //   if (info.si_value.sival_int == FL_PLANNED_TERMINATION) {
    //     rс = 0;
    //     EC_UMSG ("planned termination\n");
    //   } /*else {
    //     EC_UMSG ("externaly terminated\n");
    //   }*/
    // }
    siginfo_t info;
    to = ( struct timespec ) {
      0, 0
    };
    if ( sigtimedwait( &mask, &info, &to ) == -1 ) {
      if ( errno != EAGAIN ) {
        EC_FAIL;
      }
    } else {
      if ( info.si_value.sival_int == FL_PLANNED_TERMINATION ) {
        rc = 0;
        EC_UMSG( "planned termination\n" );
      } /*else {
        EC_UMSG ("externaly terminated\n");
      }*/
    }
  }

  EC_CLEAN_SECTION(
    ec_print( "[%d] INF> fastlog: logger process compleated\n", mypid );
    fastlog_release();
    return rc;
  );
}


//////////////////////////////////////////////////////////////////////
/*!
  @details
  @~russian
  Вызывая эту функцию приложение инициирует подсистему быстрого логгирования.
  @retval 0 в случае успеха
  @retval -1 при возникновении ошибки.

  @~english
  By calling this function application initialize fast logging facilities.
  @retval 0 on success
  @retval -1 on error
*/
int fl_init( void )
{
  //! @~russian - создается сегмент разделяемой памяти (размера = @ref FL_MSG_SZ * @ref FL_MSGS)
  //!             для передачи сообщений
  //! @~english - creates shared memory region (of size = FL_MSG_SZ*FL_MSGS) for passing
  //!             messages
  int buff_fd = -1;
  if ( ( buff_fd = shm_open( FL_SHM_NM, O_CREAT | O_EXCL | O_RDWR,
                             0 ) ) == -1 ) {
    if ( errno == EEXIST ) {
      EC_NEG1( shm_unlink( FL_SHM_NM ) );
      EC_NEG1( buff_fd = shm_open( FL_SHM_NM, O_CREAT | O_EXCL | O_RDWR, 0 ) );
    } else {
      EC_FAIL;
    }
  }

  EC_NEG1( ftruncate( buff_fd, FL_MSGS * FL_MSG_SZ ) );

  EC_CHCK( flbuff = mmap( NULL, FL_MSGS * FL_MSG_SZ, PROT_READ | PROT_WRITE,
                          MAP_SHARED, buff_fd, 0 ), MAP_FAILED );

  EC_NEG1( close( buff_fd ) );

  //! @~russian - создается очередь для индексов свободных блоков разделяемой памяти
  //! @~english - creates message queue for indexes of unused shared memory blocks
  struct mq_attr q_attr;
  memset( &q_attr, 0, sizeof( q_attr ) );
  q_attr.mq_maxmsg = FL_MSGS;
  q_attr.mq_msgsize = sizeof( FLmbDescriptor_t );
  if ( ( flempty_q = mq_open( FL_QEMPTY_NM, O_CREAT | O_EXCL | O_RDWR, 0,
                              &q_attr ) ) == -1 ) {
    if ( errno == EEXIST ) {
      EC_NEG1( mq_unlink( FL_QEMPTY_NM ) );
      EC_NEG1( flempty_q = mq_open( FL_QEMPTY_NM, O_CREAT | O_EXCL | O_RDWR, 0,
                                    &q_attr ) );
    } else {
      EC_FAIL;
    }
  }
  //! @~russian - создается очередь для дескрипторов переданных сообщений
  //! @~english - creates empty message queue for pending messages
  memset( &q_attr, 0, sizeof( q_attr ) );
  q_attr.mq_maxmsg = 2 * FL_MSGS;
  q_attr.mq_msgsize = sizeof( FLmbDescriptor_t );
  if ( ( fldata_q = mq_open( FL_QDATA_NM, O_CREAT | O_EXCL | O_RDWR, 0,
                             &q_attr ) ) == -1 ) {
    if ( errno == EEXIST ) {
      EC_NEG1( mq_unlink( FL_QDATA_NM ) );
      EC_NEG1( fldata_q = mq_open( FL_QDATA_NM, O_CREAT | O_EXCL | O_RDWR, 0,
                                   &q_attr ) );
    } else {
      EC_FAIL;
    }
  }

  //! @~russian - сегмент разделяемой памяти делится на @ref FL_MSGS блоков.
  //!             их индексы помещаются в очередь свободных дескрипторов
  //! @~english - devides shared region on FL_MSGS shared mem blocks. indices of
  //!             that blocks are placed in empty descriptors queue
  FLmbDescriptor_t msg = {0, 0, 0, 0};
  for ( int i = 0; i < FL_MSGS; i++ ) {
    msg.idx = i * FL_MSG_SZ;
    EC_NEG1( mq_send( flempty_q, ( void * ) &msg, sizeof( msg ), 0 ) );
  }



  /************************************************************************************/
  /**************************                              ****************************/
  /***********************           FORK POINT               *************************/
  /**************************                              ****************************/
  /************************************************************************************/
  //! @~russian - порождается процесс логирования ответственный за обработку очереди сообщений
  //! @~english - launches separate logger process responsible for handling pending messages on
  //!             corresponding message queue
  EC_NEG1( pid = fork() );
  if ( pid != 0 ) {
    fl_log = fl_log_extern;
    fl_log_ec = fl_log_ec_extern;
    // ok configuration & initialization is done returning to caller
    return 0;
  }

  int rc = fl_logger();
  exit( rc );

  EC_CLEAN_SECTION(
    fastlog_release();
    ec_print("Fastlog system init failed");
    return -1;
  );
}



/*!
  @defgroup rail_a Механизм шины, реализация A
  @ingroup rail
  @{
  Данная реализация целиком базируется на использовании стандартных механизмов
  POSIX IPC.
  Для синхронизации очередей используются блокировки rwlocks с установленным, посредством вызова pthread_rwlockattr_setkind_np(),
  приоритетом писателя.
*/

/*!
  @file rail_A.h

  @see @ref rail_a @n
  @date 1401.2013
  @author masolkin@gmail.com
 */

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include <kz_erch.h>
#include <fastlog.h>
#include <proto.h>
#include <rail.h>




//===============================================
//===================== beg of rwlocks definition

/*!
  @name Блокировка чтения/записи
  @{
*/

/*!
  @short блокировка чтения/записи.
*/
typedef pthread_rwlock_t SQRPrwLock_t;

/*!
  @short инициализация блокировки чтения/записи.

  @param[in,out] lock указатель на инициализируемую структуру данных
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_lock_init( SQRPrwLock_t *lock )
{
  volatile char state = 0;
  pthread_rwlockattr_t rwattr;
  EC_RC( pthread_rwlockattr_init( &rwattr ) );
  state = 1;
  EC_RC( pthread_rwlockattr_setpshared( &rwattr, PTHREAD_PROCESS_SHARED ) );
  EC_RC( pthread_rwlockattr_setkind_np( &rwattr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP ) );
  EC_RC( pthread_rwlock_init( lock, &rwattr ) );
  state = 2;

  EC_RC( pthread_rwlockattr_destroy( &rwattr ) );


  return 0;

  EC_CLEAN_SECTION(
    int rc;
  switch ( state ) {
  case 2:
    pthread_rwlockattr_destroy( &rwattr );
      fl_log_ec();
      rc = 0;

    case 1:
      pthread_rwlockattr_destroy( &rwattr );
    case 0:
    default:
      rc = -1;
  }
  return rc;
  );
}

/*!
  @short освобождение ресурсов блокировки чтения/записи.

  @param[in] lock указатель на освобождаемую структуру данных
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_lock_destroy( SQRPrwLock_t *lock )
{
  EC_RC( pthread_rwlock_destroy( lock ) );
  return 0;

  EC_CLEAN_SECTION(
    pthread_rwlock_destroy( lock );
    return -1;
  );
}

/*!
  @short захватывает блокировку на запись.

  @param[in,out] lock указатель на блокировку
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_wlock( SQRPrwLock_t *lock )
{
  EC_RC( pthread_rwlock_wrlock( lock ) );
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}


/*!
  @short захватывает блокировку на чтение.

  @param[in,out] lock указатель на блокировку
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_rlock( SQRPrwLock_t *lock )
{
  EC_RC( pthread_rwlock_rdlock( lock ) );
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}


/*!
  @short освобождает блокировку взятую чтения/записи.

  @param[in,out] lock указатель на блокировку
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_unlock( SQRPrwLock_t *lock )
{
  EC_RC( pthread_rwlock_unlock( lock ) );
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}


//! @}
//===================== end of rwlocks definition



//===============================================
//================== beg of shared RB  definition
/*!
  @name Очереди сигналов
  @{
*/

/*!
  @short Представление очереди сигналов.

  В данной реализации очередь представляется просто массивом в разделяемой памяти.
  Доступ к массиву регулируется с помощью блокировки чтения/записи. Допускается
  неэксклюзивный доступ на чтение.
*/
typedef struct SQRPqueue_s {
  char kks[50]; //!< символьное имя (ККС) сигнала записываемого в данную очередь
  uint32_t code;  //!< код сигнала записываемого в данную очередь
  size_t head; //!< голова - индекс позиции в массиве куда ведется запись новых сигналов
  int32_t pass; /*!< счетчик циклов перезаписи кольцевого буфера очереди
                     @note При длинне кольцевого буфера достаточной, чтобы вместить одну
                     секунду считываемых данных, счетчика хватит на ~136 лет. */
  SQRPrwLock_t lock; //!< блокировка упрвляющая доступом к данной очереди
} SQRPqueue_t;

/*!
  @short структура шины.

  После инициализации, сегмент разделяемой памяти представляющий шину
  в данной реализации будет иметь структуру описываемую данным типом.
*/
typedef struct SQRPqsSegment_t {
  sem_t barrier; //!< синхронизация для привязки сигналов к очередям
  size_t size; //!< размер выделенной разделяемой памяти.
  size_t num_queues; //!< количество очередей
  size_t queue_len; //!< длина очередей сигналов
} SQRPqsSegment_t;


/*!
  @short указатель на разделяемый сегмент памяти шины.

  В данной области памяти находятся очереди сигналов шины, а также разделяемые примитивы
  блокировок и вспомогательные структуры данных.
*/
static SQRPqsSegment_t *qs_seg = NULL;
SQPsignal_t **data; //!< массив выделеный под кольцевой буфер очереди сигналов
SQRPqueue_t **q; /*! Массив структур являющихся представлением очередей сигналов в данной
                    реализации. */

/*!
  @short инициализация шины в части очередей сигналов, в данной реализации.

  Выделяет сегмент разделяемой памяти достаточного размера для размещения заданного
  количества очередей сигналов требуемой вместимости.
  @param[in] num_queues количество очередей
  @param[in] queue_len  вместимость каждой очереди
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_q_init( size_t num_queues, size_t queue_len )
{
  char state = 0;
  int qs_segfd;
  EC_NEG1( qs_segfd = shm_open( "/sq_rail.shm", O_RDWR | O_CREAT | O_EXCL,
                                S_IRUSR | S_IWUSR ) );
  state = 1;

  size_t q_size = sizeof( SQRPqsSegment_t ) + num_queues * ( sizeof( SQRPqueue_t ) + queue_len * sizeof( SQPsignal_t ) );
  EC_NEG1( ftruncate( qs_segfd, q_size ) );
  EC_CHCK0( qs_seg = ( SQRPqsSegment_t * ) mmap( 0, q_size, PROT_READ | PROT_WRITE, MAP_SHARED, qs_segfd,
                     0 ), MAP_FAILED, == );
  state = 2;

  qs_seg->size = q_size;
  qs_seg->num_queues = num_queues;
  qs_seg->queue_len = queue_len;
  q = ( SQRPqueue_t ** ) calloc( num_queues, sizeof( SQRPqueue_t * ) );
  data = ( SQPsignal_t ** ) calloc( num_queues, sizeof( SQPsignal_t * ) );

  size_t ready_queues = 0;
  for ( size_t i = 0; i < num_queues; ++i ) {
    q[i] = ( SQRPqueue_t * )( ( ( char * )qs_seg ) + sizeof( SQRPqsSegment_t ) + i * ( sizeof( SQRPqueue_t ) + queue_len * sizeof( SQPsignal_t ) ) );
    EC_NEG1( sqrp_lock_init( &q[i]->lock ) );
    q[i]->code = UINT32_MAX;
    q[i]->pass = 0;
    q[i]->head = 0;
    data[i] = ( SQPsignal_t * )( ( ( char * ) q[i] ) + sizeof( SQRPqueue_t ) );
    data[i][0].trust = 1;
    ++ready_queues;
  }
  close( qs_segfd );

  EC_NEG1( sem_init( &qs_seg->barrier, 1, 0 ) );


  return 0;

  EC_CLEAN_SECTION(
  switch ( state ) {
  case 2:
    for ( size_t i = 0; i < ready_queues; i++ ) {
        sqrp_lock_destroy( &q[i]->lock );
      }
      munmap( qs_seg, q_size );
    case 1:
      close( qs_segfd );
      shm_unlink( "/sq_rail.shm" );
    default:
      break;
  }
  return -1;
  );
}

/*!
  @short освобождение ресурсов шины в части очередей сигналов.
*/
static inline
void sqrp_q_destroy( void )
{
  sem_destroy( &qs_seg->barrier );
  for ( size_t i = 0; i < qs_seg->num_queues; i++ ) {
    sqrp_lock_destroy( &q[i]->lock );
  }
  munmap( qs_seg, qs_seg->size );
  shm_unlink( "/sq_rail.shm" );
}

/*!
  @short настройка доступа к шине в части очередей сигналов для stand-alone алгоритмов.

  Отображает разделяемую память, используемую реализацией шины в качестве очередей
  сигналов, в адресное пространство вызывающего процесса.
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_q_attach( void )
{
  char state = 0;
  int qs_segfd;
  EC_NEG1( qs_segfd = shm_open( "/sq_rail.shm", O_RDWR, S_IRUSR | S_IWUSR ) );
  state = 1;

  EC_CHCK0( qs_seg = ( SQRPqsSegment_t * ) mmap( 0, sizeof( SQRPqsSegment_t ), PROT_READ | PROT_WRITE, MAP_SHARED, qs_segfd,
                     0 ), MAP_FAILED, == );
  size_t real_size = qs_seg->size;
  EC_NEG1( munmap( qs_seg, sizeof( SQRPqsSegment_t ) ) );
  EC_CHCK0( qs_seg = ( SQRPqsSegment_t * ) mmap( 0, real_size, PROT_READ | PROT_WRITE, MAP_SHARED, qs_segfd,
                     0 ), MAP_FAILED, == );

  close( qs_segfd );
  q = ( SQRPqueue_t ** ) calloc( qs_seg->num_queues, sizeof( SQRPqueue_t * ) );
  data = ( SQPsignal_t ** ) calloc( qs_seg->num_queues, sizeof( SQPsignal_t * ) );
  for ( size_t i = 0; i < qs_seg->num_queues; ++i ) {
    q[i] = ( SQRPqueue_t * )( ( ( char * )qs_seg ) + sizeof( SQRPqsSegment_t ) + i * ( sizeof( SQRPqueue_t ) + qs_seg->queue_len * sizeof( SQPsignal_t ) ) );
    data[i] = ( SQPsignal_t * )( ( ( char * ) q[i] ) + sizeof( SQRPqueue_t ) );
  }

  return 0;

  EC_CLEAN_SECTION(
    if ( state > 0 ) close( qs_segfd );
    return -1;
  );
}

/*!
  @short привязка очереди к сигналу.

  @param[in] qid индекс целевой очереди
  @param[in] code код сигнала
  @param[in] kks символьноу имя (ККС) сигнала
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_q_set_code( uint32_t qid, uint32_t code, char *kks )
{
  if ( qid > qs_seg->queue_len - 1 ) {
    errno = EINVAL;
    return -1;
  }
  q[qid]->code = code;
  strcpy( q[qid]->kks, kks );
  return 0;
}

/*!
  @short поиск очереди в которую записывается сигнал.

  @param[in] code код сигнала
  @returns  индекс очереди с сигналом, или @c -1 при возникновении ошибок.
*/
static inline
uint32_t sqrp_q_find_by_code( uint32_t code )
{
  for ( size_t i = 0; i < qs_seg->num_queues; i++ )
    if ( q[i]->code == code ) {
      return i;
    }

  errno = ESRCH;
  return -1;
}

/*!
  @short поиск очереди в которую записывается сигнал по ККС сигнала.

  @param[in] name имя (ККС) сигнала
  @returns  индекс очереди с сигналом, или @c -1 при возникновении ошибок.
*/
static inline
uint32_t sqrp_q_find_by_name( char *name )
{
  for ( size_t i = 0; i < qs_seg->num_queues; i++ )
    if ( strcmp( q[i]->kks, name ) == 0 ) {
      return i;
    }

  errno = ESRCH;
  return -1;
}

/*!
  @short курсор читателя из очереди сигналов.
*/
typedef struct SQRPqueueCursor_t {
  uint32_t pass; //!< счетчик циклов перехода на начало буфера
  uint32_t tail; /*!< индекс позиции чтения из буфера
                      указатель на функцию вычисления позиции чтения сигнала из очереди */
  int ( *compute_index )( uint32_t, struct SQRPqueueCursor_t * );
} SQRPqueueCursor_t;


/*!
  @short вычисление позиции чтения.

  Вычисляет позицию исходя из позиции предидущего чтения данной очереди соответствующим
  читателем. Также учитывается текущая позиция головы и количество прошедших циклов записи
  кольцевого буфера, представляющего очередь сигналов в данной реализации.
  @param[in] qid индекс очереди
  @param[in,out] cursor указатель на дескриптор позиции читателя.
  @retval  1 установлена новая позиция
  @retval  0 позиция не изменена (все сигналы прочитаны, новых нет)
*/
static
int sqrp_reader_compute_index( uint32_t qid, SQRPqueueCursor_t *cursor )
{
  uint32_t head = q[qid]->head;
  uint32_t pass = q[qid]->pass;

  if ( pass - cursor->pass > 1 || ( pass - cursor->pass == 1 && cursor->tail < head ) ) {
    cursor->pass = pass - 1;
    cursor->tail = head;
    fl_log( "WRN> slow reader: reset read position" );
  } else if ( cursor->tail == head && pass == cursor->pass ) {
    return 0;
  }

  if ( cursor->tail == qs_seg->queue_len - 1 ) {
    cursor->tail = 0;
    cursor->pass += 1;
  } else {
    cursor->tail += 1;
  }
  return 1;
}


/*!
  @short инициализация позиции чтения для конкретного читателя.

  @param[in] qid индекс очереди
  @param[in,out] cursor указатель на дескриптор позиции читателя.
  @returns Всегда возвращается @c 1.
*/
static
int sqrp_reader_compute_index1st( uint32_t qid, SQRPqueueCursor_t *cursor )
{
  *cursor = ( SQRPqueueCursor_t ) {
    0, 0, sqrp_reader_compute_index
  };
  uint32_t pass = q[qid]->pass;
  uint32_t head = q[qid]->head;

  if ( pass - cursor->pass > 1 || ( pass - cursor->pass == 1 && cursor->tail < head ) ) {
    cursor->pass = pass - 1;
    cursor->tail = head;
  } else if ( head > 0 ) {
    cursor->tail = 1;
  }

  return 1;
}


/*!
  @short создает структуру для указания позиции чтения

  Создается новая структура для чтения из указанной очереди сигналов.
  Выделенная память может быть освобождена функцией @c free().
  @param[in] qid идентификатор очереди для которой создается структура.
  @reurns указатель на созданную структуру или @c NULL вслучае ошибки.
*/
void *sqrp_new_cursor( uint32_t qid )
{
  SQRPqueueCursor_t *cursor = NULL;
  EC_NULL( cursor = ( SQRPqueueCursor_t * ) malloc( sizeof( SQRPqueueCursor_t ) ) );

  *cursor = ( SQRPqueueCursor_t ) {
    0, 0, sqrp_reader_compute_index1st
  };
  return cursor;

  EC_CLEAN_SECTION(
    return NULL; );
}




/*!
  @short чтение одного сигнала из соответствующей очереди.

  @post После успешного вызова позиция чтения для вызывающего алгоритма из очереди
  сместится к голове.
  @param[out] sig адрес буффера в который будет скопирован счиитанный сигнал
  @param[in] qid номер очереди из которой следует прочесть сигнал
  @param[in,out] cursor область памяти обслуживаемая реализацией. Служит для
                        определения позиции чтения из очереди конкретным читателем.
  @retval  1 сигнал считан
  @retval  0 с момента предидущего чтения новых сигналов не поступало
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_q_read1( SQPsignal_t *sig, uint32_t qid, SQRPqueueCursor_t *cursor )
{
  EC_NEG1( sqrp_rlock( &q[qid]->lock ) );

  if ( cursor->compute_index( qid, cursor ) == 0 ) {
    EC_NEG1( sqrp_unlock( &q[qid]->lock ) );
    return 0;
  }

  memcpy( sig, &data[qid][cursor->tail], sizeof( SQPsignal_t ) );

  EC_NEG1( sqrp_unlock( &q[qid]->lock ) );
  return 1;

  EC_CLEAN_SECTION(
    return -1; );
}



/*!
  @short изменение позиции чтения.

  @pre Значение аргумента @c offset должно быть больше @c 0 и меньше длины очереди.
  @param[in] offset разница между индексом конца очереди и индексом устанавливаемой
                    позиции. (<tt>offset = 0</tt> устанавливает указатель чтения
                    на конец очереди, пропуская все накопленные в буфферах сигналы).
  @param[in] qid индекс целевой очереди
  @param[in,out] cursor область памяти обслуживаемая реализацией. Служит для
                        определения позиции чтения из очереди конкретным читателем.
  @returns установленное в результате вызова смещение от головы очереди,
          -1 при возникновении ошибок.
*/
int sqrp_q_seek( size_t offset, uint32_t qid, SQRPqueueCursor_t *cursor )
{
  if ( offset >= qs_seg->queue_len ) {
    offset = qs_seg->queue_len - 1;
  }

  EC_NEG1( sqrp_rlock( &q[qid]->lock ) );

  uint32_t pass = q[qid]->pass;
  uint32_t head = q[qid]->head;
  int new_pos = head - offset;
  if ( new_pos < 0 ) {
    cursor->pass = pass - 1;
    cursor->tail = new_pos + qs_seg->queue_len;
  } else {
    cursor->pass = pass;
    cursor->tail = new_pos;
  }

  EC_NEG1( sqrp_unlock( &q[qid]->lock ) );
  return offset;

  EC_CLEAN_SECTION(
    return -1; );
}



/*!
  @short запись одного сигнала в очередь.

  @param[out] sig адрес буффера с новым сигналом
  @param[in] qid индекс целевой очереди
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_q_write1( SQPsignal_t *sig, uint32_t qid )
{
  EC_NEG1( sqrp_wlock( &q[qid]->lock ) );

  if ( q[qid]->head == qs_seg->queue_len - 1 ) {
    q[qid]->head = 0;
    ++ ( q[qid]->pass );
  } else {
    ++ ( q[qid]->head );
  }

  memcpy( &data[qid][q[qid]->head], sig, sizeof( *sig ) );

  EC_NEG1( sqrp_unlock( &q[qid]->lock ) );
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}



/*!
  @short ожидание окончательной инициализации (реализация).

  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_wait()
{
  EC_NEG1( sem_wait( &qs_seg->barrier ) );
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}



/*!
  @short открывает барьер синхронизации для \c n процессов (реализация).

  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int sqrp_run( int n )
{
  while ( n-- ) {
    EC_NEG1( sem_post( &qs_seg->barrier ) );
  }
  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}



//! @}
//================== end of shared RB  definition



//===============================================
//================== beg of shared MB  definition

/*!
  @name Блоки разделяемой памяти свободного формата
  @{
*/

/*!
  @short дескриптор разделяемого блока памяти свободного формата.
*/
typedef struct SQRPmbSegment_t {
  char *name; //!< имя разделяемого блока
  size_t len; //!< размер сегмента за вычетом места занимаемого блокировкой чтения/записи
  SQRPrwLock_t *lock; //!< указатель на блокировку чтения/записи
  void *ptr;          //!< указатель на данные блока
  uint8_t *valid; //!< флаг состояния блока памяти
  char owned; //!< флаг владельца
} SQRPmbSegment_t;


//! @short список разделяемых блоков используемых процессом.
static  struct {
  SQRPmbSegment_t *blocks; //!< массив используемых блоков
  size_t len;         //!< длинна массива заданного полем @c blocks
  //  pthread_mutex_t lock; //!< блокировка
} mb_list;


/*!
  @short освобождения занимаемых блоков памяти.
*/
void sqrp_mb_clean_segments( void )
{
  fl_log( "INF> users segments cleanup\n" );
  for ( size_t i = 0; i < mb_list.len; i++ ) {
    if ( mb_list.blocks[i].owned ) {
      *mb_list.blocks[i].valid = 0;
      EC_NEG1( sqrp_lock_destroy( mb_list.blocks[i].lock ) );
    }
    EC_NEG1( munmap( mb_list.blocks[i].lock, mb_list.blocks[i].len + sizeof( SQRPrwLock_t ) + 1 ) );
    EC_NEG1( shm_unlink( mb_list.blocks[i].name ) );
    // }

    // return;
    EC_CLEAN_SECTION(
      ec_print( "WRN> user segment '%s' clean up failed: ", mb_list.blocks[i].name );
    );
  }
}


/*!
  @short инициализация шины в части разделяемых блоков свободного формата.
*/
static inline
int sqrp_mb_init( void )
{
  // pthread_mutex_init (&mb_list.lock, 0);
  mb_list.len = 0;
  mb_list.blocks = NULL;
  // EC_NZERO ( atexit (sqrp_mb_clean_segments) );
  return 0;

  // EC_CLEAN_SECTION (
  //   return -1;
  // );
}

/*!
  @short выделение разделяемого блока по заданному имени
*/
static inline
int sqrp_mb_acquire( const char *name, size_t *size_p, SQRmbCreateFlag_t fl )
{
  char state = 0;
  for ( size_t i = 0; i < mb_list.len; i++ ) {
    if ( strcmp( name, mb_list.blocks[i].name ) == 0 ) {
      if ( fl == SQR_MB_CREATE ) {
        errno = EEXIST;
        EC_FAIL;
      }
      *size_p = mb_list.blocks[i].len;
      return i;
    }
  }

  char if_new = 0;
  int mbfd = shm_open( name, O_RDWR, S_IRUSR | S_IWUSR );
  if ( mbfd == -1 ) {
    if ( errno != ENOENT ) {
      EC_FAIL;
    }

    EC_CHCK0( fl, SQR_MB_GET, == );
    EC_NEG1( mbfd = shm_open( name, O_RDWR | O_CREAT | O_EXCL,
                              S_IRUSR | S_IWUSR ) );
    state = 1;
    EC_NEG1( ftruncate( mbfd, *size_p + sizeof( SQRPrwLock_t ) + 1 ) );
    if_new = 1;
  } else {
    state = 1;
    if ( fl == SQR_MB_CREATE ) {
      close( mbfd );
      errno = EEXIST;
      return -1;
    } else {
      struct stat q;
      EC_NEG1( fstat( mbfd, &q ) );
      *size_p = q.st_size - sizeof( SQRPrwLock_t ) - 1;
    }
  }


  void *ptr;
  EC_CHCK0( ptr = mmap( 0, *size_p + sizeof( SQRPrwLock_t ) + 1, PROT_READ | PROT_WRITE,
                        MAP_SHARED, mbfd, 0 ), MAP_FAILED, == );
  state = 2;
  char owned = 0;
  if ( if_new ) {
    memset( ptr, 0, *size_p + sizeof( SQRPrwLock_t ) + 1 );
    EC_NEG1( sqrp_lock_init( ptr ) );
    state = 3;
    owned = 1;
  }

  // pthread_mutex_lock (&mb_list.lock);
  char *name_tmp = NULL;
  EC_NULL( name_tmp = strdup( name ) );
  EC_REALLOC( mb_list.blocks, ( mb_list.len + 1 ) * sizeof( SQRPmbSegment_t ) );

  mb_list.blocks[mb_list.len].name   = name_tmp;
  mb_list.blocks[mb_list.len].owned  = owned;
  mb_list.blocks[mb_list.len].lock   = ( SQRPrwLock_t * ) ptr;
  mb_list.blocks[mb_list.len].len    = *size_p;
  mb_list.blocks[mb_list.len].valid  = ( uint8_t * )( ( char * ) ptr + sizeof( SQRPrwLock_t ) );
  *mb_list.blocks[mb_list.len].valid = 1;
  mb_list.blocks[mb_list.len].ptr    = ( ( char * ) mb_list.blocks[mb_list.len].valid ) + 1;
  ++mb_list.len;
  // pthread_mutex_unlock (&mb_list.lock);

  close( mbfd );

  return mb_list.len - 1;

  EC_CLEAN_SECTION(
  switch ( state ) {
  case 3:
    sqrp_lock_destroy( ptr );
    case 2:
      munmap( ptr, *size_p + sizeof( SQRPrwLock_t ) + 1 );
    case 1:
      close( mbfd );
      shm_unlink( name );
    default:
      break;
  }
  return -1;
  );
}


/*!
  @short чтение из разделяемого блока свободного формата.

  @param[in] id     дескриптор разделяемого блока памяти
  @param[in] offset смещение от начала блока, с которого необходимо произвести чтение
  @param[in] buf    указатель на буфер, в который поступят считанные данные
  @param[in] len    длинна читаемого подблока
  @param[in] fl     флаг регулирующий блокирующее/неблокирующее чтение
  @returns Количество прочитанных байтов или @c -1 при возникновении ошибок.
*/
static inline
int sqrp_mb_read( size_t id, size_t offset, void *const buf, size_t len,
                  SQRmbIoFlag_t fl )
{
  if ( id >= mb_list.len || offset + len > mb_list.blocks[id].len || *mb_list.blocks[id].valid ) {
    errno = EINVAL;
    return -1;
  }

  EC_NEG1( sqrp_rlock( mb_list.blocks[id].lock ) );

  memcpy( buf, ( char * ) mb_list.blocks[id].ptr + offset, len );

  EC_NEG1( sqrp_unlock( mb_list.blocks[id].lock ) );

  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}

/*!
  @short запись в блок разделяемой памяти свободного формата.

  @param[in] id     дескриптор разделяемого блока памяти
  @param[in] offset смещение от начала блока, с которого необходимо произвести запись
  @param[in] buf    указатель на буфер, в котором находятся данные для записи
  @param[in] len    длинна записываемого подблока
  @param[in] fl     флаг регулирующий блокирующую/неблокирующую запись
  @returns Количество записанных байтов или @c -1 при возникновении ошибок.
*/
static inline
int sqrp_mb_write( size_t id, size_t offset, void *const buf, size_t len,
                   SQRmbIoFlag_t fl )
{
  if ( id >= mb_list.len || offset + len > mb_list.blocks[id].len || *mb_list.blocks[id].valid == 0 ) {
    errno = EINVAL;
    return -1;
  }

  EC_NEG1( sqrp_wlock( mb_list.blocks[id].lock ) );

  memcpy( ( char * ) mb_list.blocks[id].ptr + offset, buf, len );

  EC_NEG1( sqrp_unlock( mb_list.blocks[id].lock ) );

  return 0;

  EC_CLEAN_SECTION(
    return -1; );
}

//! @}
//================== end of shared MB  definition

/*!
  @name Инициализация и освобождение ресурсов шины
  @{
*/
/*void sqrp_clean_sema (void)
{
  printf ("INF> main semaphore cleanup\n");
  if (sem_unlink ("/sq_main.sem")) {
    perror ("WRN> main semaphore cleanup failed: ");
  }
}
*/

static inline
int sqrp_init( size_t num_queues, size_t queue_len )
{
  /*  sem_t *main_sem;
    while ((main_sem = sem_open ("/sq_main.sem", O_CREAT | O_EXCL, S_IRWXU,
                                 0)) == SEM_FAILED) {
      if (errno == EEXIST) {
        if (sem_unlink ("/sq_main.sem") == -1) {
          perror ("ERR> init, main semaphore unlink failure: ");
        }
      } else {
        perror ("ERR> sem_open failed(init): ");
        exit (-1);
      }
    }
    atexit (sqrp_clean_sema);
  */
  EC_NEG1( sqrp_q_init( num_queues, queue_len ) );
  EC_NEG1( sqrp_mb_init() );

  return 0;

  //  for (int i = 0; i < ALG_MAX; i++) sem_post (main_sem);
  EC_CLEAN_SECTION(
    return -1; );
}

/*!
  @short освобождает ресурсы выделенные для работы шины.
*/
static inline
void sqrp_release( void )
{
  sqrp_q_destroy();
}

static inline
int sqrp_attach( void )
{
  /*  sem_t *main_sem;
    while ((main_sem = sem_open ("/sq_main.sem", 0)) == SEM_FAILED)
      if (errno == ENOENT) sleep (2);
      else {
        perror ("ERR> sem_open failed(attach): ");
        exit (-1);
      }

    if (sem_wait (main_sem)) {
      perror ("ERR> sem_wait failed: ");
      exit (-1);
    }
  */
  EC_NEG1( sqrp_q_attach() );
  EC_NEG1( sqrp_mb_init() );
  return 0;

  EC_CLEAN_SECTION(
    return -1;
  );
}
//! @}

//! @}

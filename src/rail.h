/*!
  @addtogroup rail_top
  @{
*/
/*!
  @file rail.h
  @short декларация верхнего уровня API внутренней шины.

  @see @ref rail @n
  @date 22.10.2012
  @author masolkin@gmail.com
*/
#ifndef RAIL_H_
#define RAIL_H_

#include <stddef.h>
#include <stdint.h>
#include <util.h>


#include <algos.h>
#include <proto.h>
#include <tsignals.h>

#define SQR_RB_MAX  400
#define SQR_RB_MAX_SIGNALS 1024
#define ALG_MAX 100

//! @name Инициализация
//! @{

//! @short инициализация шины.
int sqr_init ( size_t num_queues, size_t queue_len );

//! @short инициализация доступа к шине из отдельного процесса.
int sqr_attach ( void );

//! @short освобождает ресурсы выделенные для работы шины.
void sqr_release ( void );


//! указатель на дескриптор активного алгоритма.
extern SQAlgoObject_t *sqr_algo;

//void sqr_bind_algo_signals(SQAlgoDescriptor_t * alg, char *name, int idx, int param);
//! @}

//! @name Работа с кольцевыми очередями сигналов
//! @{

//! установка текущего активного алгоритма.
static inline
void sqr_set_curr_algo ( SQAlgoObject_t *p_algo )
{
  sqr_algo = p_algo;
}

//! @short создание нового курсора чтения, для определенной очереди.
void *sqr_new_cursor ( uint32_t qid );



//! @short чтение сигналов из кольцевых буфферов.
size_t sqr_read ( SQPsignal_t *buff, int *last_upd );

//! @short перемещение позиции чтения из кольцевых буферов.
int sqr_seek ( size_t off );

// @short запись сигналов в кольцевые буфера.
//size_t sqr_write (SQPsignal_t *buff, size_t sz);

//! @short запись одного сигнала в соответствующий кольцевой буфер.
int sqr_write1 ( SQPsignal_t *buff1, size_t qid );

//! @short ожидание окончательной инициализации.
int sqr_wait ();

//! @short открывает барьер синхронизации для \c n процессов.
int sqr_run ( int n );

//! @short получение описателя сигнала.
int sqr_get_sigdef ( const char *name, SQTSigdef_t *sigdef );

//! @}

//! @name Блоки разделяемой памяти свободного формата
//! @{

//! Флаги используемые при получении разделяемого блока памяти.
typedef enum {
  SQR_MB_CREATE, //!< для создания
  SQR_MB_GET,    //!< для поиска существующего
  SQR_MB_ANY     //!< для создания, если не существует искомого
} SQRmbCreateFlag_t;

//! флаги определяющие использование блокирования при вызовах.
typedef enum {
  SQR_MB_BLOCK,    //!< вызов с блокировкой
  SQR_MB_NONBLOCK  //!< без блокировки
} SQRmbIoFlag_t;

//! @short создание/получение блока разделяемой памяти.
int sqr_mb_acquire ( const char *name, size_t *size_p, SQRmbCreateFlag_t fl );

//! @short чтение из блока разделяемой памяти.
int sqr_mb_read ( size_t id, size_t offset, void *const buf, size_t len,
                  SQRmbIoFlag_t fl );

//! @short запись в блок разделяемой памяти.
int sqr_mb_write ( size_t id, size_t offset, void *const buf, size_t len,
                   SQRmbIoFlag_t fl );

//! @short освобождение блоков разделяемой памяти.
int sqr_mb_release ( void );

//! @}


//! @name Привязка кодов к очередям сигналов
//! @{

//! @short привязка очереди к сигналу.
int sqr_bind_queue ( uint32_t qid, uint32_t code, char *kks );

//! @short поиск очереди в которую записывается сигнал.
uint32_t sqr_find_queue ( uint32_t code );
uint32_t sqr_find_queue_by_name ( char *name );

//! @}


#endif // RAIL_H_

//! @}

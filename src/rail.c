/*!
  @defgroup rail Внутренняя шина
  @{
*/
/*!
  @page rail_intro Введение
  Внутренняя шина обеспечивает паралельно выполняющимся процессам из состава @b squirrel
  доступ к общим данным. Механизмы шины гарантируют целостность записываемых и читаемых
  данных.

  Для облегчения разработки и портирования реализация механизмов шины отделена от API
  доступного в плагинах алгоритмов и во внешних алгоритмах. На данный момент используется
  единственная реализация @em А.
*/
/*!
  @defgroup rail_top API верхнего уровня
  @{
  Для связи различных частей приложения используются два механизма. Вызовы данных механизмов
  продекларированы/реализованны соответственно в @ref railh/@ref rail.c.

  @page rail_top_queues Очереди сигналов
  Основной механизм передачи сигналов. Задача [ввода](@ref datain) из состава @b squirrel,
  получающая поток технологических сигналов по сетевому [протоколу](@ref proto), использует
  очереди сигналов для сохранения полученной информации. [Алгоритмы](@ref secalg) входными
  данными для которых являются технологические сигналы имееют доступ к чтению данных из
  очередей сигналов.

  @page rail_top_shm Блоки разделяемой памяти
  Блоки разделяемой памяти предназначены для обмена данными между родственными алгоритмами.
  Цели для которых применяется этот механизм полностью регламентируются нуждами разработчика
  алгоритмов. Здесь представлен лишь унифицированный интерфейс для абстрагирования от
  низлежащего механизма реализации разделяемой памяти.
*/
/*!
  @file rail.c
  @short реализация верхнего уровня API внутренней шины.

  @see @ref rail @n
  @date 22.10.2012
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
#error Open Group Single UNIX Specification, Version 4 (SUSv4) expected to be supported
#endif

#if !defined(__gnu_linux__)
#error GNU Linux is definitely supported by now
#endif

#include <rail.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <kz_erch.h>
#include <util.h>
#include <filters.h>
#include <rail.h>

// low-level realization A inclusion
#include <railA.h>



/*!
  Должен указывать на алгоритм, который в данный момент использует в текущем процессе
  функции sqr_read() и sqr_seek().
*/
SQAlgoObject_t *sqr_algo = NULL;

static bool bus_ready = false;


/*!
  Вызывается функция инициализации шины из реализации.
  @param[in] num_queues количество очередей
  @param[in] queue_len  вместимость каждой очереди
*/
int sqr_init( size_t num_queues, size_t queue_len )
{
  int rc = sqrp_init( num_queues, queue_len );
  if ( rc != -1 ) { bus_ready = true; }
  return rc;
}

/*!
  Вызывается функция инициализации шины из реализации. Для stand-alone процессов,
  на самом деле происходит подключение к уже работающей шине.
*/
int sqr_attach( void )
{
  return sqrp_attach();
}

// release rail resources
void sqr_release( void )
{
  if ( bus_ready ) {
    sqrp_release();
    sqr_mb_release();
  }
}

/*!
  @param[in] qid индекс целевой очереди
  @param[in] code код сигнала
  @param[in] kks символьноу имя (ККС) сигнала
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
int sqr_bind_queue( uint32_t qid, uint32_t code, char *kks )
{
  return sqrp_q_set_code( qid, code, kks );
}

/*!
  @param[in] code код сигнала
  @returns  индекс очереди с сигналом, или @c -1 при возникновении ошибок.
*/
uint32_t sqr_find_queue( uint32_t code )
{
  return sqrp_q_find_by_code( code );
}

uint32_t sqr_find_queue_by_name( char *name )
{
  return sqrp_q_find_by_name( name );
}


/*!
  Память выделенная для курсора может быть освобождена в приложении вызовом @c free().
  @param[in] qid идентификатор очереди
  @returns новый курсор для чтения из указанной очереди.
*/
void *sqr_new_cursor( uint32_t qid )
{
  return sqrp_new_cursor( qid );
}



/*!
  @pre Количество и список сигналов подлежащих считыванию определяется конфигурацией алгоритма,
  указатель на чей десриптор, в настоящий момент хранится в глобальной переменной модуля
  @c sqr_algo. Ее изменение может быть произведено вызовом sqr_set_curr_algo().

  В предоставленный буффер считываются поступившие с момента последнего обращения сигналы.
  @param[out] buff     указатель на массив сигналов для заполнения по результатам чтения.
                       Будут записаны только сигналы изменившиеся за время прошедшее с момента
                       предидущего вызова данной функции текущим алгоритмом.
  @param[out] last_upd массив индексов изменившихся сигналов. Имеют смысл лишь @em n первых
                       элементов, где @em n возвращаемое значение функции.
  @returns количество прочитанных с шины сигналов (изменившихся).
*/
size_t sqr_read( SQPsignal_t *buff, int *last_upd )
{
  int upd = 0, i = 0;
  for ( SQAisigBind_t **isig = sqr_algo->isigs; *isig != NULL; isig++, i++ ) {
    switch ( sqrp_q_read1( buff + i, isig[0]->queue, isig[0]->q_cursor ) ) {
      case 1:
        isig[0]->upd = 1;
        if ( last_upd )
        { last_upd[upd] = i; }
        ++upd;
        break;

      case 0:
        isig[0]->upd = 0;
        break;

      case -1:
        buff[i].trust = 1;
        break;
    }
  }

  for ( SQFilterChain_t **fc = sqr_algo->filter_chains; *fc != NULL; fc++ ) {
    for ( int i = 0; i < fc[0]->inum; i++ ) {
      if ( sqr_algo->isigs[fc[0]->target_ids[i]]->upd == 1 )
      { fc[0]->input[i] = &buff[fc[0]->target_ids[i]].val; }
      else
      { fc[0]->input[i] = NULL; }
    }
    for ( SQFilterObject_t **filter = fc[0]->chain; *filter != NULL; filter++ ) {
      filter[0]->super->run( filter[0]->subj, fc[0]->input );
    }
  }
  return upd;
}

/*!
  Изменяет указатель чтения сигналов с шины, для всех сигналов читаемых текущим алгоритмом,
  дескриптор которого, на данный момент, хранится в глобальной переменной шины @c sqr_algo.
  @param[in] offset разница между индексом конца очереди и индексом устанавливаемой
                    позиции. (<tt>offset = 0</tt> устанавливает указатель чтения
                    на конец очереди, пропуская все накопленные в буфферах сигналы).
  @returns минимальное смещение от конца очереди среди всех очередей, для которых проведена
           операция. Таким образом, если возвращаемое значение меньше @c offset, то для
           какого-то сигнала не удалось переместить указатель чтения шины на заданное место.
*/
int sqr_seek( size_t offset )
{
  int seek_min = SIZE_MAX;
  for ( SQAisigBind_t **isig = sqr_algo->isigs; *isig != NULL; isig++ ) {
    int shifted = sqrp_q_seek( offset, isig[0]->queue, isig[0]->q_cursor );
    if ( shifted < seek_min ) { seek_min = shifted; }
  }
  return seek_min;
}

// writes signal to the specified queue
int sqr_write1( SQPsignal_t *buff1, size_t qid )
{
  return sqrp_q_write1( buff1, qid );
}


/*!
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
int sqr_wait()
{
  return sqrp_wait();
}

/*!
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
int sqr_run( int n )
{
  return sqrp_run( n );
}

/*!
  Возвращает описатель сигнала, присоединенного к именованному входу текущего алгоритма.
  @param[in]     name   имя входа текущего алгоритма.
  @param[in,out] sigdef буфер куда будет помещено описание сигнала, должен быть предоставлен
                        вызывающей стороной.
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
int sqr_get_sigdef( const char *name, SQTSigdef_t *sigdef )
{
  for ( SQAisigBind_t **bind = sqr_algo->isigs; bind != 0; bind++ ) {
    if ( strcmp( name, bind[0]->inp ) == 0 ) {
      const SQTSigdef_t *_sigdef;
      EC_NULL( _sigdef = sqts_get_sigdef( bind[0]->kks ) );
      *sigdef = *_sigdef;
      return 0;
    }
  }

  EC_CLEAN_SECTION(
    errno = EINVAL;
    return -1;
  );
}



/*
size_t sqr_write (SQPsignal_t *buff, size_t sz)
{
  for (int i = 0; sqr_algo->osigs[i] != NULL; i++) {
    sqrp_set_signal (&buff[i], sqr_algo->osigs[i]->queue);
  }
  //TODO implement correct return val
  return 0;
}
*/


/*!
  Выделяет блок разделяемой памяти и возвращает его дескриптор. Поведение функции варьируется в
  зависимости от значения флага @c fl:
  @arg @ref SQR_MB_CREATE  реализация проверяет наличие уже выделенного блока памяти с именем
                          @c name, при его наличии возвращается @c -1 (завершение с ошибкой).
                          В противном случае выделяется новый блок с заданным именем и размером
                          задаваемым аргументом @c size_p.
  @arg @ref SQR_MB_GET   реализация проверяет наличие уже выделенного блока памяти с именем
                        @c name и возвращает его дескриптор, при этом размер блока возвращается
                        через аргумент @c size_p, если блок с заданным именем отсутствует
                        возвращается @c -1 (завершение с ошибкой).
  @arg @ref SQR_MB_ANY сначала производится действия аналогичные варианту @c SQR_MB_CREATE, затем
                      в отсутствии успеха действия аналогичные варианту @c SQR_MB_GET.
  @returns Дескриптор выделенного блока памяти или @c -1 при возникновении ошибок.
*/
int sqr_mb_acquire( const char *name, size_t *size_p, SQRmbCreateFlag_t fl )
{
  return sqrp_mb_acquire( name, size_p, fl );
}

/*!
  @param[in] id     дескриптор разделяемого блока памяти
  @param[in] offset смещение от начала блока, с которого необходимо произвести чтение
  @param[in] buf    указатель на буфер, в который поступят считанные данные
  @param[in] len    длинна читаемого подблока
  @param[in] fl     флаг регулирующий блокирующее/неблокирующее чтение
  @returns Количество прочитанных байтов или @c -1 при возникновении ошибок.
*/
int sqr_mb_read( size_t id, size_t offset, void *const buf, size_t len,
                 SQRmbIoFlag_t fl )
{
  return sqrp_mb_read( id, offset, buf, len, fl );
}


/*!
  @param[in] id     дескриптор разделяемого блока памяти
  @param[in] offset смещение от начала блока, с которого необходимо произвести запись
  @param[in] buf    указатель на буфер, в котором находятся данные для записи
  @param[in] len    длинна записываемого подблока
  @param[in] fl     флаг регулирующий блокирующую/неблокирующую запись
  @returns Количество записанных байтов или @c -1 при возникновении ошибок.
*/
int sqr_mb_write( size_t id, size_t offset, void *const buf, size_t len,
                  SQRmbIoFlag_t fl )
{
  return sqrp_mb_write( id, offset, buf, len, fl );
}

int sqr_mb_release( void )
{
  sqrp_mb_clean_segments();
  return 0;
}

//! @}
//! @}

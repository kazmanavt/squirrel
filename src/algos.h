//-#- noe -#-

/*!
  @file algos.h
  @short декларация API для работы с плагинами алгоритмов.

  @see @ref algos @n
  @date 19.11.2013
  @author masolkin@gmail.com

*/
#ifndef ALGOS_H_
#define ALGOS_H_


#define ALGO_OUTSIGS_MAX 10

#include <stdint.h>

#include <filters.h>

// #include <aio.h>

// #include <util.h>

/*
typedef struct SQA_rb_bind_s {
  size_t code;
  size_t tail;
  size_t pass;
} SQA_rb_bind_t;
*/

/*!
  @short дескриптор плагина с алгоритмом.

  Дескриптор представляет подключаемый алгоритм. Каждый загруженный алгоритм
  имеет свой уникальный дескриптор, который, после успешной загрузки всех алгоритмов
  функцией sqa_load_algos(), может быть получен с помощью вызова sqa_getalgo().
*/
typedef struct SQAlgoDescriptor_t {
  void *lib_h; /*!< handle загруженного модуля */
  const char *name; /*!< имя алгоритма. */
  uint32_t version; /*!< версия алгоритма */
  uint32_t protocol; /*!< протокол поддерживаемый алгоритмом, определяет способ
                          инициализации и передачи параметров */
  char *path; /*!< имя файла из которого загружен плагин */
  const void *( *create )(); /*!<
                  @short указатель на функцию фабрику алгоритмов.

                  Механизм работы вызова должен соответствовать
                  [соглашению по работе с плагинами алгоритмов](@ref a_agree).
                  @returns указатель на созданный экземпляр алгоритма или @c NULL при возникновении
                  ошибки.
                */
  void ( *init )(); /*!< @short указатель на функцию
                              иницилизирующую экземпляр алгоритма */

  void *( *run )(); /*!<
                  @short указатель на функцию алгоритма.

                  Выполняет итерацию алгоритма.

                  Механизм работы вызова должен соответствовать
                  [соглашению по работе с плагинами алгоритмов](@ref a_agree).
                  @returns зарезервировано
                */
  // SQA_rb_bind_t **isigs;
  // SQA_rb_bind_t **osigs;
  // struct aiocb aio;
  // struct {
  //   int (* **func) ();
  //   int *ds;
  // } filter;
  // returns pointer to test data.
  // func to return pointer to data, suitable for test run of algo.
  // TODO which kind of data to provide ???
  // SQ_TestData_t * (*query_tdata) (void);
} SQAlgoDescriptor_t;

/*!
  привязка входа алгоритма к очереди сигналов.
*/
typedef struct {
  int queue;      //!< дескриптор очереди с сигналом соответствующим входу алгоритма
  const char *kks;//!< имя входного сигнала в очереди
  const char *inp;//!< имя входа алгоритма
  char upd;       //!< флаг для фильтра о факте обновления сигнала
  void *q_cursor;/*!< дополнительная структура данных заполняемая реализацией шины,
                       для внутренних нужд */
} SQAisigBind_t;

/*!
  экземпляр алгоритма.
*/
typedef struct {
  SQAlgoDescriptor_t
  *super;    /*!< указатель на родительский дескриптор плагина, от которого
                  порожден данный экземпляр */
  const char *init_str; /*!< инициализирующая строка с которой порожден данный
                             экземпляр */
  const void *subj;     //!< идентифицирующий указатель экземпляра
  SQAisigBind_t **isigs;        //!< нуль-терминированный массив описателей входов алгоритма
  SQFilterChain_t **filter_chains; /*!< нуль-терминированный массив цепочек фильтров применяемых
                                      в данном экземпляре алгоритма */
  int pool_id; /*!<
                идентифицирует группу алгоритмов ассоциированную с соответствующим
                воркером.
                @arg @c -1 время выполнения итерации данного алгоритма превышает проектного цикла.
                @arg @c -2 недостаточно ресурсов в системе */
} SQAlgoObject_t;

/*!
  @short загрузка плагинов с алгоритмами.
*/
int sqa_load_algos( const char *path, const char **desired_algos,
                    int num_algos );

/*!
  @short поиск определенного плагина по имени.
*/
SQAlgoDescriptor_t *sqa_getalgo( const char *name );


#endif // ALGOS_H_

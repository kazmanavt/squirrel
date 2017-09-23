//-#- noe -#-

/*!
  @file filters.h
  @short Декларация API для работы с плагинами фильтров.

  @see @ref filters @n
  @date 19.11.2013
  @author masolkin@gmail.com
*/

#ifndef FILTERS_H_
#define FILTERS_H_

#include <stdint.h>

/*!
  @short дескриптор плагина с фильтром.

  Дескриптор представляет подключаемый алгоритм фильтрации. Каждый загруженный фильтр
  имеет свой уникальный дескриптор, который, после успешной загрузки всех фильтров
  функцией sqf_load_filters(), может быть получен с помощью вызова sqf_getfilter().
*/
typedef struct SQFilterDescriptor_t {
  void* lib_h; /*!< handle загруженного модуля */
  const char
  * name; /*!< имя фильтра */
  uint32_t version; /*!< версия алгоритма */
  uint32_t protocol; /*!< протокол поддерживаемый фильтром, определяет способ
                    инициализации и передачи параметров */
  char* path; /*!< имя файла из которого загружен плагин */
  const void* ( *create )  (); /*!<
                  @short указатель на функцию фабрику фильтров.

                  Механизм работы вызова должен соответствовать
                  [соглашению по работе с плагинами фильтров](@ref f_agree).
                  @returns указатель на созданный экземпляр фильтра или @c NULL при возникновении
                  ошибки.
                */
  int ( *run ) (); /*!<
                  @short указатель на функцию фильтра.

                  Выполняет фильтрацию входных данных.

                  Механизм работы вызова должен соответствовать
                  [соглашению по работе с плагинами фильтров](@ref f_agree).
                  @retval  0 в случае успеха
                  @retval -1 при возникновении ошибок
                */
} SQFilterDescriptor_t;

/*!
  @short дескриптор экземпляра фильтра.
*/
typedef struct  {
  SQFilterDescriptor_t* super; /*!< указывает на родительский дескриптор плагина, от которого
                                    порожден данный экземпляр */
  const char* init_str;        //!< содержит строку с которой был инстанциирован экземпляр
  const void* subj;            //!< идентифицирующий указатель экземпляра фильтра
} SQFilterObject_t;

/*!
  @short цепочка фильтров.

  Для выстраивания цепочки фильтров на одинаковых входов.
*/
typedef struct {
  int inum;            //!< количество входных данных цепочки фильтров
  int* target_ids;     /*!< массив дескрипторов целевых объектов фильтрации, для фильтров
                            закрепленных за алгоритмами - это номера входов алгоритма, для
                            глобальных фильтров - это номера очередей входных сигналов. */
  float** input;       /*!< нуль-терминированный массив указателей на данные для фильтрации
                            (вход/выход цепочки) */
  SQFilterObject_t** chain; /*!< собственно указатель на нуль-терминированный массив
                                 дескрипторов экземпляров фильтров в порядке их применения. */
} SQFilterChain_t;

/*!
  @short загрузка плагинов с фильтрами.
*/
int sqf_load_filters ( const char* path, const char** desired_filters, int num );

/*!
  @short поиск определенного плагина по имени.
*/
SQFilterDescriptor_t* sqf_getfilter ( const char* name );

#endif // FILTERS_H_
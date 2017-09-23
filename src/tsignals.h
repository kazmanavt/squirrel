//-#- noe -#-

/*!
  @defgroup tsigs API для работы с технологическими сигналами
  @ingroup intern
  @{
*/

/*!
  @file tsignals.h
  @short Декларации функций относящихся к @ref tsigs.

  API, декларация которого помещена здесь, предоставляется для любого модуля в
  приложении, а также доступен для использования в подгружаемых библиотеках (плагинах).

  @see @ref tsigs @n
  @date 27.04.2013
  @author masolkin@gmail.com
*/

#ifndef TSIGNALS_H_
#define TSIGNALS_H_

//! @short максимальная длина имени технологического сигнала.
#define KKS_LEN 50

//! @short тип сигнала.
typedef enum SQTSignalType_t {
  IA = 0, /*!< Входной аналоговый сигнал. */
  END = 1 /*!< Максимальное значение типа (не может быть типом реального сигнала). */
} SQTSignalType_t;

//! @short структура определяющай все параметры технологического сигнала.
typedef struct SQTSigdef_s {
  SQTSignalType_t type; /*!< Тип сигнала. */
  char kks[KKS_LEN + 1]; /*!< Имя сигнала. */
  int code; /*!< Внутренний код сигнала. */
  double accuracy; /*!< Точность для аналогого сигнала. */
  double delta; /*!< Ожидаемя дисперсия для аналогого сигнала. */
  double smin; /*!< Нижняя граница значений для аналогого сигнала. */
  double smax; /*!< Верхняя граница значенийдля аналогого сигнала. */
} SQTSigdef_t;

/*!
  @short загрузка таблицы принимаемых сигналов.
*/
int sqts_load_signals ( const char** desired_signals, int num );

//! @short поиск описания сигнала по его имени.
const SQTSigdef_t* sqts_get_sigdef ( const char* kks );

//! @short формирорвание списка имен всех загруженных сигналов.
void sqts_list_sig_names ( const char* const** siglist, int* num );


#endif // TSIGNALS_H_
//! @}

//-#- noe -#-

/*!
  @page filters API для работы с плагинами фильтров

  Данный набор вызовов предоставляет следующие возможности:
  - загрузка плагинов с фильтрами, требующимися для работы @b squirrel в заданной конфигурации.
  - проверка доступности всех сконфигурированных плагинов фильтров.

  @section f_agree Соглашение
  Для каждого загруженного плагина создается дескриптор типа @ref SQFilterDescriptor_t, через
  который доступны некоторые функции и переменные модуля.

  По соглашению приводимому здесь, для того чтобы плагин с фильтром мог быть успешно загружен и
  использован @b squirrel, должны быть выполнены следующие условия:
  - имя фильтра должно быть заданно строковым литералом в глобальной переменной
    `const char *name`. Оно должно быть уникально для каждого фильтра.
  - версия алгоритма фильтра должна быть задана целым числом в глобальной переменной
    `uint32_t version`.
  - протокол по которому работает плагин должен быть задан целым числом в глобальной переменной
    `uint32_t protocol`. Протокол определяет как приложение должно инициализировать и использовать
    фильтр реализуемый плагином. На настоящий момент поддерживается только протокол @c 0.
  - в плагине должна быть реализована функция фабрика с экспортируемым именем `create()`.
    - для плагинов протокола @c 0, данная функция должна принимать не формализованную
      инициализирующую строку. При отсутствии ошибок создавать независимый экземпляр фильтра
      и возвращать указатель типа `void *` однозначно идентифицирующий созданный экземпляр.
      В ошибочной ситуации функция должна возвращать @c NULL. Сигнатура для протокола @c 0 дожна
      быть следующей:
~~~~~~~~~~~~~~~{.c}
void * create(const char *);
~~~~~~~~~~~~~~~
  - в плагине должна быть реализована функция собственно фильтра с экспортируемым именем `run()`.
    - для плагинов протокола @c 0, данная функция должна принимать два аргумента, первым из которых
      должен быть указатель типа `void *` идентифицирующий требуемый экземпляр фильтра.
      Вторым аргументом должен быть указатель на входные данные для алгоритма фильтрации, через
      этот же укатель должны возвращаться результаты фильтрации. При отсутствии ошибок функция
      должна возвращать @c 0, или @c -1 в противном случае. Сигнатура для протокола @c 0 дожна быть
      следующей:
~~~~~~~~~~~~~~~{.c}
int run(const void *, void *);
~~~~~~~~~~~~~~~

  @see
  @ref filters.h @n
  @ref filters.c
*/

/*!
  @defgroup deffilters Предопределенные фильтры
  @details Здесь сгруппированно описание стандартных фильтров поставляемх вместе со @b squirrel.
*/

/*!
  @file filters.c
  @short реализация API для работы с плагинами фильтров.

  @see @ref filters @n
  @date 19.11.2013
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



#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>



#include <kz_erch.h>
#include <util.h>
#include <filters.h>


/*!
  статическая таблица загруженных фильтров.
*/
static SQFilterDescriptor_t *filters = NULL;
/*!
  количество загруженных фильтров.
*/
static long num_filters = 0;


/*!
  @short загрузка плагина с фильтром по имени файла.
*/
static SQFilterDescriptor_t *try_load ( const char *plugin_path ) {
  void *fplug = NULL;
  EC_DL ( fplug = dlopen ( plugin_path, RTLD_NOW ) );

  EC_REALLOC ( filters, ( num_filters + 1 ) * sizeof ( SQFilterDescriptor_t ) );

  filters[num_filters].lib_h = fplug;
  filters[num_filters].path = NULL;
  EC_DL ( filters[num_filters].name = * ( char ** ) dlsym ( fplug, "name" ) );
  EC_DL ( filters[num_filters].version = * ( uint32_t * ) dlsym ( fplug, "version" ) );
  EC_DL ( filters[num_filters].protocol = * ( uint32_t * ) dlsym ( fplug,
                                                                   "protocol" ) );
  if ( filters[num_filters].protocol == 0 ) {
    EC_DL ( filters[num_filters].create = dlsym ( fplug, "create" ) );
    EC_DL ( filters[num_filters].run = dlsym ( fplug, "run" ) );
  }
  EC_NULL ( filters[num_filters].path = strdup ( plugin_path ) );
  return &filters[num_filters++];

  EC_CLEAN_SECTION (
    if ( fplug != NULL ) dlclose ( fplug );
    return NULL;
  );
}

/*!
  @short функция фильтрации файлов с плагинами.
*/
static int filfil ( const struct dirent *what ) {
  size_t len = strlen ( what->d_name );
  if ( len <= 3 ) {
    return 0;
  }
  if ( strcmp ( ".so", &what->d_name[len - 3] ) == 0 ) {
    return 1;
  }
  return 0;
}

/*!
  @details
  Загружает плагины с фильтрами и строит список активных фильтров. Загружаются только
  фильтры необходимые для работы @b squirrel в текущей конфигурации.

  Функция может быть успешно вызвана только один раз, при повторных вызовах возвращается @c -1.

  @param[in] path каталог в котором следует искать файлы плагинов. Если @c path нулевой
                  указатель то по умолчанию сканируется каталог "./filters".
  @param[in] desired_filters список сконфигурированных фильтров
  @param[in] num количество элементов в списке задаваемом аргументом @c desired_filters.
  @retval  0 при успешном завершении
  @retval -1 при обнаружении ошибок.
*/
int sqf_load_filters ( const char *path, const char **desired_filters, int num ) {
  if ( desired_filters == NULL ) {
    return 0;
  }

  static char loaded = 0;
  // fall to default path (./filters) if path not specefied
  if ( path == NULL ) {
    path = "./filters";
  }

  int re = -1;
  struct dirent **list = NULL;

  // find all entries in the specefied path according to
  // pattern *.so (see filfil filter function)
  EC_NEG1 ( re = scandir ( path, &list, filfil, alphasort ) );

  // try load plugins one by one
  printf ( "Loading filters plugins:\n" );
  char relpath[LINE_MAX];
  for ( int i = 0; i < re; i++ ) {
    snprintf ( relpath, LINE_MAX, "%s/%s", path, list[i]->d_name );
    SQFilterDescriptor_t *filtr = NULL;
    if ( ( filtr = try_load ( relpath ) ) == NULL ) {
      ec_print ( " load failed" );
    } else {
      // check if we need this plugin
      if ( bsearch ( &filtr->name, desired_filters, num, sizeof ( *desired_filters ),
                     compare_cstr ) != NULL ) {
        printf ( " #%-3ld: [%s-%d.%d] (proto: %d)\n", num_filters, filtr->name,
                 filtr->version / 100, filtr->version % 100,
                 filtr->protocol );
      } else {
        printf ( " Filter %s from path %s not required by configuration, skipping...\n",
                 filtr->name, filtr->path );
        free ( filtr->path );
        dlclose ( filtr->lib_h );
        --num_filters;
      }
    }
    free ( list[i] );
  }
  if ( re > 0 ) {
    free ( list );
  }
  re = 0;
  // check if all required filters are loaded
  if ( num_filters != num ) {
    for ( int i = 0; i < num; i++ ) {
      char loa = 0;
      for ( int j = 0; j < num_filters; j++ ) {
        if ( strcmp ( desired_filters[i], filters[j].name ) == 0 ) {
          loa = 1;
        }
      }
      if ( loa == 0 ) {
        fprintf ( stderr, "ERR> filter [%s] required by configuration is not found\n",
                  desired_filters[i] );
      }
    }
    EC_UMSG ( "Required filters number mismatched!" );
  }

  loaded = 1;
  return 0;

  EC_CLEAN_SECTION (
    // clean results of scandir if any
  if ( re > 0 ) {
  for ( int i = 0; i < re; i++ ) {
      free ( list[i] );
    }
    free ( list );
  }
  // unload any loaded plugins
  for ( int i = 0; i < num_filters; i++ ) {
  if ( filters[i].path != NULL ) {
      free ( filters[i].path );
    }
    dlclose ( filters[i++].lib_h );
  }
  return -1;
  );
}

/*!
  @details
  Ищет в таблице загруженных фильтров фильтр с именем @c name, и возвращает
  указатель на его дескриптор.
  @param name строка содержащая имя искомого фильтра
  @returns указатель на дескриптор фильтра, при возникновении ошибок возвращается
  нулевой указатель (@c NULL)
*/
SQFilterDescriptor_t *sqf_getfilter ( const char *name ) {
  if ( name == NULL ) {
    errno = EINVAL;
    return NULL;
  }
  for ( int i = 0; i < num_filters; ++i ) {
    if ( strcmp ( filters[i].name, name ) == 0 ) {
      return &filters[i];
    }
  }
  return NULL;
}

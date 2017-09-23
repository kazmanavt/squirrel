//-#- noe -#-

/*!
  @page algos API для работы с плагинами алгоритмов

  Данный набор вызовов предоставляет следующие возможности:
  - загрузка плагинов с алгоритмами, требующимися для работы @b squirrel в заданной конфигурации.
  - проверка доступности всех сконфигурированных плагинов алгоритмов.

  @section a_agree Соглашение
  Для каждого загруженного плагина создается дескриптор типа @ref SQAlgoDescriptor_t, через
  который доступны некоторые функции и переменные плагина.

  По соглашению приводимому здесь, для того чтобы плагин с алгоритмом мог быть успешно загружен и
  использован @b squirrel, должны быть выполнены следующие условия:
  - имя алгоритма должно быть заданно строковым литералом в глобальной переменной
    `const char *name`. Оно должно быть уникально для каждого алгоритма.
  - версия алгоритма должна быть задана целым числом в глобальной переменной `uint32_t version`.
  - протокол по которому работает плагин должен быть задан целым числом в глобальной переменной
    `uint32_t protocol`. Протокол определяет как приложение должно инициализировать и использовать
    алгоритм реализуемый плагином. На настоящий момент поддерживается только протокол @c 0.
  - в плагине должна быть реализована функция фабрика с экспортируемым именем `create()`.
    - для плагинов протокола @c 0, данная функция должна принимать не формализованную
      инициализирующую строку. При отсутствии ошибок создавать независимый экземпляр алгоритма
      и возвращать указатель типа `void *` однозначно идентифицирующий созданный экземпляр.
      В ошибочной ситуации функция должна возвращать `NULL`. Сигнатура для протокола @c 0 дожна быть
      следующей:
~~~~~~~~~~~~{.c}
void * create(const char *);
~~~~~~~~~~~~
  - в плагине должна быть реализована функция инициализации экземпляра алгоритма с экспортируемым
    именем `init()`.
    - для плагинов протокола @c 0, данная функция должна принимать один аргумент, указатель типа
      `void *` идентифицирующий требуемый экземпляр алгоритма. При отсутствии ошибок функция
      должна возвращать @c 0, или @c -1 в противном случае. Сигнатура для протокола @c 0 дожна быть
      следующей:
~~~~~~~~~~~~{.c}
int init(const void *);
~~~~~~~~~~~~
  - в плагине должна быть реализована функция собственно алгоритма с экспортируемым именем `run()`.
    - для плагинов протокола @c 0, данная функция должна принимать один аргумент, указатель типа
      `void *` идентифицирующий требуемый экземпляр алгоритма. При отсутствии ошибок функция
      должна возвращать @c 0, или @c -1 в противном случае. Сигнатура для протокола @c 0 дожна быть
      следующей:
~~~~~~~~~~~~{.c}
int run(const void *);
~~~~~~~~~~~~
@note Примечание:Тип uint32 определен <c><stdint.h></c>

  @see
  @ref algos.h @n
  @ref algos.c
*/

/*!
  @defgroup defalgos Предопределенные алгоритмы
  @details Здесь сгруппированно описание стандартных алгоритмов поставляемх вместе со @b squirrel.
*/

/*!
  @file algos.c
  @short реализация API для работы с плагинами алгоритмов.

  @see @ref algos @n
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
#include <algos.h>
//#include <../sigs.h>
//#include <../rail.h>

/*!
  статическая таблица загруженных алгоритмов.
*/
static SQAlgoDescriptor_t *algos = NULL;
/*!
  количество загруженных алгоритмов.
*/
static long num_algos = 0;


/*!
  @short загрузка плагина с алгоритмом по имени файла.
*/
static SQAlgoDescriptor_t *try_load ( const char *plugin_path ) {
  void *aplug = NULL;
  EC_DL ( aplug = dlopen ( plugin_path, RTLD_NOW ) );

  EC_REALLOC ( algos, ( num_algos + 1 ) * sizeof ( SQAlgoDescriptor_t ) );

  algos[num_algos].lib_h = aplug;
  algos[num_algos].path = NULL;
  EC_DL ( algos[num_algos].name = * ( char ** ) dlsym ( aplug, "name" ) );
  EC_DL ( algos[num_algos].version = * ( uint32_t * ) dlsym ( aplug, "version" ) );
  EC_DL ( algos[num_algos].protocol = * ( uint32_t * ) dlsym ( aplug,
                                                               "protocol" ) );
  if ( algos[num_algos].protocol == 0 ) {
    EC_DL ( algos[num_algos].create = dlsym ( aplug, "create" ) );
    EC_DL ( algos[num_algos].init = dlsym ( aplug, "init" ) );
    EC_DL ( algos[num_algos].run = dlsym ( aplug, "run" ) );
  }
  EC_NULL ( algos[num_algos].path = strdup ( plugin_path ) );
  return &algos[num_algos++];

  EC_CLEAN_SECTION (
    if ( aplug != NULL ) dlclose ( aplug );
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
  Загружает плагины с алгоритмами и строит список активных алгоритмов. Загружаются только
  алгоритмы необходимые для работы @b squirrel в текущей конфигурации.

  Функция может быть успешно вызвана только один раз, при повторных вызовах возвращается @c -1.

  @param[in] path каталог в котором следует искать файлы плагинов. Если @c path нулевой
                  указатель то по умолчанию сканируется каталог "./algos".
  @param[in] desired_algos список сконфигурированных алгоритмов
  @param[in] num количество элементов в списке задаваемом аргументом @c desired_algos.
  @retval  0 при успешном завершении
  @retval -1 при обнаружении ошибок.
*/
int sqa_load_algos ( const char *path, const char **desired_algos, int num ) {
  static char loaded = 0;
  // fall to default path (./algos) if path not specefied
  if ( path == NULL ) {
    path = "./algos";
  }

  int re = -1;
  struct dirent **list = NULL;

  // find all entries in the specefied path according to
  // pattern *.so (see filfil filter function)
  EC_NEG1 ( re = scandir ( path, &list, filfil, alphasort ) );

  // try load plugins one by one
  printf ( "Loading algorythms plugins:\n" );
  char relpath[LINE_MAX];
  for ( int i = 0; i < re; i++ ) {
    snprintf ( relpath, LINE_MAX, "%s/%s", path, list[i]->d_name );
    SQAlgoDescriptor_t *alg = NULL;
    if ( ( alg = try_load ( relpath ) ) == NULL ) {
      ec_print ( " load failed" );
    } else {
      // check if we need this plugin
      if ( bsearch ( &alg->name, desired_algos, num, sizeof ( *desired_algos ),
                     compare_cstr ) != NULL ) {
        printf ( " #%-3ld: [%s-%d.%d] (proto: %d)\n", num_algos, alg->name,
                 alg->version / 100, alg->version % 100,
                 alg->protocol );
      } else {
        printf ( " Algorythm %s from path %s not required by configuration, skipping...\n",
                 alg->name, alg->path );
        free ( alg->path );
        dlclose ( alg->lib_h );
        --num_algos;
      }
    }
    free ( list[i] );
  }
  if ( re > 0 ) {
    free ( list );
  }
  re = 0;
  // check if all required algorythms are loaded
  if ( num_algos != num ) {
    for ( int i = 0; i < num; i++ ) {
      char loa = 0;
      for ( int j = 0; j < num_algos; j++ ) {
        if ( strcmp ( desired_algos[i], algos[j].name ) == 0 ) {
          loa = 1;
        }
      }
      if ( loa == 0 ) {
        fprintf ( stderr, "ERR> algorythm [%s] required by configuration is not found\n",
                  desired_algos[i] );
      }
    }
    EC_UMSG ( "Required algorythms number mismatched!" );
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
  for ( int i = 0; i < num_algos; i++ ) {
  if ( algos[i].path != NULL ) {
      free ( algos[i].path );
    }
    dlclose ( algos[i++].lib_h );
  }
  return -1;
  );
}

/*!
  @details
  Ищет в таблице загруженных алгоритмов алгоритм с именем @c name, и возвращает
  указатель на его дескриптор.
  @param name строка содержащая имя искомого алгоритма
  @returns указатель на дескриптор алгоритма, при возникновении ошибок возвращается
  нулевой указатель (@c NULL)
*/
SQAlgoDescriptor_t *sqa_getalgo ( const char *name ) {
  if ( name == NULL ) {
    errno = EINVAL;
    return NULL;
  }
  for ( int i = 0; i < num_algos; ++i ) {
    if ( strcmp ( algos[i].name, name ) == 0 ) {
      return &algos[i];
    }
  }
  return NULL;
}

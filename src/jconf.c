//-#- noe -#-

/*!
  @~russian
  @page jconf API для чтения конфигурационных файлов в формате JSON

  Для чтения конфигурационных файлов в формате [JSON](http://www.json.org),
  а также для доступа к загруженным параметрам конфигурации используются API продекларированные в
  @ref jconf.h и реализованные в @ref jconf.c. Они являются удобной оберткой для работы с
  @c C реализацией [JSON](http://www.json.org)-парсера -
  [nxjson](https://bitbucket.org/yarosla/nxjson/src) (Заголовочный файл и реализация:
  @ref nxjson.h, @ref nxjson.c)

  Параметры из указанного конфигурационного файла загружаются в память, и доступны для чтения с
  помощю данного API. После использования конфигурационные данные могут быть выгружены из памяти.

  [JSON](http://www.json.org)-объект из файла конфигурации после загрузки в память
  представляет собой дерево обьектов различного типа. В узлах дерева располагаются составные обьекты
  и массивы. Обьекты листья, в свою очередь, содержат значения параметров конфигурации и имеют тип
  соответствующий содержащемуся значению. Всего представлено четыре элементарных типа:
   - строка;
   - целое число типа @c long;
   - число с плавающей точкой типа @c double.

  Для доступа к элементам дерева используется ряд функций, которые принимают в качестве адреса
  элемента дерева два аргумента. Первый является указателем на элемент дерева с которого начинается
  поиск требуемого элемента, если этот аргумент имеет значении NULL, то поиск осуществляется от
  корня дерева, элемент от которого осуществляется поиск в данном контексте называется корневым.
  Второй елемент представляет собой строку задающую имя обьекта в dot-нотации, для выбора элементов
  массива используются квадратные скобки с индексом элемента масива в виде десятичного числа.

  Например строка вида <tt> .algos[1].filters[0].name </tt> инициирует следующий поиск:
  1. В корневом элементе ишется объект с именем @c algos.
  2. Предполагается что найденный элемент является массивом и выбирается его первый элемент.
  3. В выбранном элементе ищется объект с именем @c filters.
  4. Предполагается что найденный элемент является массивом и выбирается его нулевой элемент.
  5. В выбранном элементе ищется объект с именем @c name, что и является результатом поиска.

  @~english
  @page jconf API for reading JSON style configuration files

  API declared in @ref jconf.h and implemented in @ref jconf.c intended for loading configuration
  files of [JSON](http://www.json.org) format and for access to loaded configuration
  parameters. This API is convinience wrapper above
  [nxjson](https://bitbucket.org/yarosla/nxjson/src) (header and implementation:
  @ref nxjson.h, @ref nxjson.c), that is @c C language realization of
  [JSON](http://www.json.org)-parser.

  Configuration parameters from pointed file are loaded in memory and are accessible for reading
  with help of this API. After use of configuration it can be unloaded from memory.

  [JSON](http://www.json.org)-object loaded from configuration file to memory is
  represented as tree of objects of various types. Array objects and composit objects is presented
  as tree nodes. While leaf elements are objects containing configuration parameters, that objects
  have type of containing parameter. Four types of leaf objects exists:
   - string;
   - integer value of type @c long;
   - floating point number of type @c double.

  A set of functions intended for access to tree elements, takes two arguments as object address.
  First argument is pointer to tree element from which search of desired element will starts.
  NULL value for this argument initiates search from the tree root. Element from which search
  will starts is called root element in this context. The second argument is string containing
  object name in dot-notation, for selection of array elements square brakets with decimal array
  index should be used.

  For example string <tt> .algos[1].filters[0].name </tt> will initiate search with next steps:
  1. Search object named @c algos in root element.
  2. Assume found element is array, take element with index 1.
  3. Search object named @c filters in selected element.
  4. Assume found element is array, take element with index 0.
  5. Search object named @c name in selected element, that is result.

  @~
  @see
  @ref jconf.h @n
  @ref jconf.c
*/

/*!
  @file jconf.c
  @~russian
  @short Реализация @ref jconf.

  @~english
  @short @ref jconf implementation.

  @~
  @details
  @see @ref jconf
  @date 19.11.2013
  @author masolkin@gmail.com
*/
/*!
  @file nxjson.h
  @~russian
  @short Декларация API [nxjson](https://bitbucket.org/yarosla/nxjson/src)-парсера.
  @~english
  @short [nxjson](https://bitbucket.org/yarosla/nxjson/src)-parser API declaration.

  @~
  @details
  @copyright
  Copyright (c) 2013 Yaroslav Stavnichiy <yarosla@gmail.com>

  This file is part of NXJSON.

  NXJSON is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License
  as published by the Free Software Foundation, either version 3
  of the License, or (at your option) any later version.

  NXJSON is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NXJSON. If not, see <http://www.gnu.org/licenses/>.
*/
/*!
  @file nxjson.c
  @~russian
  @short Реализация API [nxjson](https://bitbucket.org/yarosla/nxjson/src)-парсера.
  @~english
  @short [nxjson](https://bitbucket.org/yarosla/nxjson/src)-parser API implementation.

  @~
  @details
  @copyright
  Copyright (c) 2013 Yaroslav Stavnichiy <yarosla@gmail.com>

  This file is part of NXJSON.

  NXJSON is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License
  as published by the Free Software Foundation, either version 3
  of the License, or (at your option) any later version.

  NXJSON is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with NXJSON. If not, see <http://www.gnu.org/licenses/>.
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


#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <nxjson.h>

#include <kz_erch.h>

/*!
  @internal
  @~russian
  @short загруженная конфигурация.

  @details
  Указатель на структуру, реализующую дерево обьектов с параметрами конфигурации.

  @~english
  @short loaded configuration.

  @details
  Pointer to tree like structure holding loaded configuration.

  @~
  @attention internal docs
*/
static const nx_json *cfg = NULL;
/*!
  @internal
  @~russian
  @short буффер для строковых параметров конфигурации.

  @~english
  @short buffer to hold loaded config strings.

  @~
  @details
  @attention internal docs
*/
static char *buffer = NULL;


//////////////////////////////////////////////////////
/*!
  @details
  @~russian
  Выделяет память для дерева с загруженным [JSON](http://www.json.org) объектом,
  представляющим параметры конфигурации загруженные из указанного файла.
  @param[in] path имя конфигурационного файла.
  @retval 0 при успешном завершении
  @retval -1 в случае ошибки

  @~english
  Will allocate memory to hold tree of loaded [JSON](http://www.json.org) object, which represents
  parameters read from supplied config file.
  @param[in] path pathname of the configuration file, containig [JSON](http://www.json.org) object
                  to be loaded
  @retval 0 if everything is OK
  @retval -1 is returned on error
*/
int jcf_load( char *path )
{
  int cfg_d = -1;
  EC_NEG1( cfg_d = open( path, O_RDONLY, 0 ) );
  struct stat st;

  // get size of config to be loaded & allocate buffer for it
  EC_NEG1( fstat( cfg_d, &st ) );
  EC_NULL( buffer = malloc( st.st_size + 1 ) );

  // load config content to buffer as null-terminated string
  int count = st.st_size;
  char *tbuf = buffer;
  while ( count > 0 ) {
    int rc = -1;
    EC_NEG1( rc = read( cfg_d, tbuf, count ) );
    count -= rc;
    tbuf += rc;
  }
  *tbuf = '\0';
  close( cfg_d );

  EC_NULL( cfg = nx_json_parse_utf8( buffer ) );
  return 0;

  EC_CLEAN_SECTION(
    if ( cfg_d != -1 ) close( cfg_d );
    free( buffer ); buffer = NULL;
    return -1;
  );
}

//////////////////////////////////////////////////////
/*!
  @details
  @~russian
  Освобождает память используемую загруженной конфигурацией. После выгрузки
  конфигурационные параметры становятся недоступны, указатели на строковые значения полученные ранее
  перестают быть валидными.
  @~english
  Free memorey used by loaded configuration. After unloading configuration parameters are non accessible.
  Pointers to strings obtained becames invalid.
*/
void jcf_free()
{
  if ( cfg != NULL ) {
    nx_json_free( cfg );
  }
  free( buffer );
}


/*!
  @internal
  @~russian
  @short поиск элемента дерева с указанным именем.

  @details
  @param[in] node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Всегда возвращается корректный указатель на объект. В случае неуспешного поиска возвращается
           указатель на объект типа @c NX_JSON_NULL, иначе возвращается указатель на искомый объект.

  @~english
  @short searhch for named element of tree.

  @details
  @param[in] node element of tree that should be used as root element for search. NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns Correct pointer to object is returned ever. In case search fails - pointer to object of
           @c NX_JSON_NULL type is returned, in other cases function returns pointer to desired object.

  @~
  @attention internal docs
*/
static inline
const nx_json *find_node( const nx_json *node, const char *name )
{
  if ( name == NULL ) {
    return node;
  }

  const nx_json *cursor = node;
  static nx_json null_node = { .type = NX_JSON_NULL};
  char *_buff, *buff;
  EC_NULL( buff = _buff = strdup( name ) );
  char tok = *buff++;

  while ( tok != '\0' ) {
    int l;
    switch ( tok ) {
      // object name should follow
      case '.':
        l = strcspn( buff, ".[" );
        if ( l == 0 ) {
          EC_UMSG( "@{%.*s}: next obj name has zero length", buff - _buff, name );
        }
        tok = * ( buff + l );
        * ( buff + l ) = '\0';
        cursor = nx_json_get( cursor, buff );
        if ( cursor->type == NX_JSON_NULL ) {
          EC_UMSG( "@{%.*s}: has no obj named '%s'", buff - _buff, name, buff );
        }
        buff += l + 1;
        break;

      // array index  expected
      case '[':
        l = strcspn( buff, "]" );
        if ( l == 0 ) {
          EC_UMSG( "@{%.*s}: arr index of zero length", buff - _buff - 1, name );
        }
        tok = * ( buff + l + 1 );
        * ( buff + l ) = '\0';
        char *en;
        errno = 0;
        int idx = strtol( buff, &en, 10 );
        if ( *en != '\0' || errno ) {
          EC_UMSG( "@{%.*s}: bad arr index [%s]", buff - _buff - 1, name, buff );
        }
        cursor = nx_json_item( cursor, idx );
        if ( cursor->type == NX_JSON_NULL ) {
          EC_UMSG( "@{%.*s}: arr has no obj[%d]", buff - _buff - 1, name, idx );
        }
        buff += l + 2;
        break;

      default:
        EC_UMSG( "@{%.*s}: bad obj notation follow", buff - _buff - 1 , name );
        break;
    }
  }
  free( _buff );
  return cursor;

  EC_CLEAN_SECTION(
    free( _buff );
    return &null_node;
  );
}


/*!
  @~russian
  @details
  Возвращает указатель на искомый объект. Полученное значение может быть использовано для указания
  корневого элемента в последующих операциях доступа к параметрам.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Вслучае успеха возвращается корректный указатель на искомый объект.
           В случае неуспешного поиска возвращается @c NULL, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns pointer to desired object. Obtained value can be passed as root element in next operations
  on access to parameters.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns On success pointer to desired object is returned. In case search fails @c NULL pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
const nx_json *jcf_o( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_OBJECT, != );
  return node;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve object {'%s'}", name );
    errno = EINVAL;
    return NULL;
  );
}

/*!
  @param[in] _node указатель на объект с которого будет начат поиск
  @param[in] name имя искомого объекта в dot-нотации.
  @returns количество потомков.

  @param[in] _node pointer to object where search will starts
  @param[in] name name of desired element in dot-notation.
  @returns number of children for requested object.
*/
int jcf_ol( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_OBJECT, != );
  return node->length;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve object {'%s'}", name );
    errno = EINVAL;
    return 0;
  );
}

/*!
  @~russian Возвращает имя объекта относительно его родителя.
  @param node указатель на объект чьё имя требуется получить
  @returns имя возвращается в виде строки.

  @~english Returns name of object regarding to its parent.
  @param node pointer to object whose name should to be retrieved
  @returns name of object as a string.
*/
const char *jcf_oname( const nx_json *node )
{
  return node->key;
}


/*!
  @~russian
  @details
  Предназначается для сканирования элементов дерева конфигурации. Возвращает первый дочерний элемент
  заданного аргументом @c node элемента. При возникновении ошибок возвращается @c NULL, и переменная
  @c errno устанавливается в @c EINVAL.
  @param[in] _node указатель на объект с которого будет начат поиск
  @param[in] name имя искомого объекта в dot-нотации.

  @~english
  @details
  Intended for scaning of elements of configuration tree. Returns first child element of element
  given by @c node argument. On errors @c NULL is returned, and @c errno is set to @c EINVAL.
  @param[in] _node pointer to object where search will starts
  @param[in] name name of desired element in dot-notation.
*/
const nx_json *jcf_o1st( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_OBJECT, != );
  return node->child;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve child {%s}", node ? node->key : "NULL" );
    errno = EINVAL;
    return NULL;
  );
}

/*!
  @~russian
  @details
  Предназначается для сканирования элементов дерева конфигурации. Возвращает следующий
  элемент имеющий того же родителя, что и заданный аргументом @c node элемент. Значение
  @c NULL при отсутствии ошибок означает, что больше элементов нет.
  При возникновении ошибок переменная @c errno устанавливается в @c EINVAL.

  @~english
  @details
  Intended for scaning of elements of configuration tree. Returns next element having the same
  parent as element given by @c node argument. @c NULL value in lack of errors designate no more
  elements situation. On errors @c NULL is returned, and @c errno is set
  to @c EINVAL.
*/
const nx_json *jcf_onext( const nx_json *node )
{
  EC_NULL( node );
  return node->next;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "attempt to operate on NULL" );
    errno = EINVAL;
    return NULL;
  );
}

/*!
  @~russian
  @details
  Возвращает указатель на искомый массив. Полученное значение может быть использовано в последующих
  операциях с массивом и  для указания корневого элемента в последующих операциях доступа к параметрам.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого массива в dot-нотации.
  @returns Вслучае успеха возвращается корректный указатель на искомый массив.
           В случае неуспешного поиска возвращается @c NULL, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns pointer to desired array. Obtained value can be used in next operations with array and can be
  passed as root element in next operations on access to parameters.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired array in dot-notation.
  @returns On success pointer to desired array is returned. In case search fails @c NULL pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
const nx_json *jcf_a( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_ARRAY, != );
  return node;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve array {'%s'}", name );
    errno = EINVAL;
    return NULL;
  );
}

/*!
  @~russian
  @details
  Возвращает указатель на элемент массива. Полученное значение может быть использовано для указания
  корневого элемента в последующих операциях доступа к параметрам.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого массива в dot-нотации.
  @param[in] idx индекс искомого элемента массива.
  @returns Вслучае успеха возвращается указатель на элемент массива.
           В случае неуспешного поиска возвращается @c NULL, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns pointer to desired object. Obtained value can be passed as root element in next operations
  on access to parameters.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired array in dot-notation.
  @param[in] idx index of desired array element.
  @returns On success pointer to desired object is returned. In case search fails @c NULL pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
const nx_json *jcf_ai( const nx_json *_node, const char *name, int idx )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_ARRAY, != );
  if ( idx < 0 || idx > node->length - 1 ) {
    EC_UMSG( "index [%d] out of bounds [0:%d]", idx, node->length - 1 );
  }
  return nx_json_item( node, idx );

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve array element [%d]", idx );
    errno = EINVAL;
    return NULL;
  );
}


int jcf_al( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_ARRAY, != );
  return node->length;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "attempt to get length of non array object" );
    errno = EINVAL;
    return 0;
  );
}

/*!
  @~russian
  @details
  Возвращает значение искомого объекта строкового типа.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Вслучае успеха возвращается значение искомого объекта.
           В случае неуспешного поиска возвращается @c NULL, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns value of desired object of string type.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns On success value of desired object is returned. In case search fails @c NULL pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
const char *jcf_s( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_STRING, != );
  return node->text_value;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve value of {'%s'}", name );
    errno = EINVAL;
    return NULL;
  );
}

/*!
  @~russian
  @details
  Возвращает значение искомого объекта целого типа.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Вслучае успеха возвращается значение искомого объекта.
           В случае неуспешного поиска возвращается @c 0, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns value of desired object of integer type.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns On success value of desired object is returned. In case search fails @c 0 value is
           returned, and @c errno value is set to @c EINVAL.
*/
long jcf_l( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_INTEGER, != );
  return node->int_value;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve value of {'%s'}", name );
    errno = EINVAL;
    return 0;
  );
}

/*!
  @~russian
  @details
  Возвращает значение искомого объекта @em float типа.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Вслучае успеха возвращается значение искомого объекта.
           В случае неуспешного поиска возвращается @c 0, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns value of desired object of @em float type.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns On success value of desired object is returned. In case search fails @c 0 pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
double jcf_d( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_DOUBLE, != );
  return node->dbl_value;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve value of {'%s'}", name );
    errno = EINVAL;
    return 0;
  );
}

/*!
  @~russian
  @details
  Возвращает значение искомого объекта логического типа.
  @param[in] _node элемент дерева который будет использован как корневой для поиска. Значение
                  @c NULL инициирует поиск от корня дерева.
  @param[in] name имя искомого объекта в dot-нотации.
  @returns Вслучае успеха возвращается значение искомого объекта.
           В случае неуспешного поиска возвращается @c 0, и значение переменной @c errno
           устанавливается в @c EINVAL.

  @~english
  @details
  Returns value of desired object of boolean type.
  @param[in] _node element of tree that should be used as root element for search. @c NULL value for this
                  argument initiates search from root of the whole tree.
  @param[in] name name of desired element in dot-notation.
  @returns On success value of desired object is returned. In case search fails @c 0 pointer is
           returned, and @c errno value is set to @c EINVAL.
*/
int jcf_b( const nx_json *_node, const char *name )
{
  const nx_json *node = find_node( _node == NULL ? cfg : _node , name );
  EC_CHCK0( node->type, NX_JSON_BOOL, != );
  return node->int_value;

  EC_CLEAN_SECTION(
    ec_add_str( __func__, __FILE__, __LINE__, "on error", "failed to retrieve value of {'%s'}", name );
    errno = EINVAL;
    return 0;
  );
}

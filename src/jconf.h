//-#- noe -#-

/*!
  @file jconf.h
  @~russian @short API для конфигурационных файлов формата [JSON](http://www.json.org).

  @~english @short API for configuration files of [JSON](http://www.json.org) format.

  @~
  @details
  @see @ref jconf @n
  @date 19.11.2013
  @author masolkin@gmail.com
*/
#ifndef CONF_H_
#define CONF_H_

/*!
  @~russian
  @name Загрузка и освобождение
  @~english
  @name Loading and release
  @~ @{
*/
//! @~russian @short загружает конфигурационные параметры из файла @c path.
//! @~english @short loads configuration parameters from file named @c path.
int jcf_load ( char* path );

//! @~russian @short освобождает ресурсы.
//! @~english @short free resources.
void jcf_free ();

//! @}

/*!
  @~russian
  @name Доступ к загруженной конфигурации
  @~english
  @name Access to loaded configuration
  @~ @{
*/
/*!
  @~russian
  @short абстрактный тип для [JSON](http://www.json.org)-объекта или массива объектов.

  @details
  @note Не для прямого использования.
  @par
  Предназначен для того чтобы хранить промежуточные результаты при работе со сложными объектами
  и массивами.

  @~english
  @short abstract type for [JSON](http://www.json.org)-object or array of objects.

  @details
  @note Not for direct use.
  @par
  Intended for holding intermediate results, while working with complex objects and with arrays.
*/
typedef const void* JCFobj;

//! @~russian @short получить составной объект по @ref jconf "имени".
//! @~english @short retrieve composit object by @ref jconf "name".
JCFobj jcf_o ( JCFobj node, const char* name );

//! @~russian @short получить количество потомков.
//! @~english @short retrieve number of children.
int jcf_ol ( JCFobj node, const char* name );

//! @~russian @short получить имя(ключ) объекта.
//! @~english @short retrieve name(key) of object.
const char* jcf_oname ( JCFobj node );

//! @~russian @short получить первый дочерний элемент.
//! @~english @short get first child element.
JCFobj jcf_o1st ( JCFobj node, const char* name );

//! @~russian @short возвращает следующий элемент.
//! @~english @short returns next element.
JCFobj jcf_onext ( JCFobj node );

//! @~russian @short получить объект типа массив по @ref jconf "имени".
//! @~english @short retrieve array object by @ref jconf "name".
JCFobj jcf_a ( JCFobj node, const char* name );

//! @~russian @short получить элемент массива по индексу.
//! @~english @short retrieve array element by index.
JCFobj jcf_ai ( JCFobj node, const char* name, int idx );

//! @~russian @short получить размер массива.
//! @~english @short retrieve array length.
int jcf_al ( JCFobj node, const char* name );

//! @~russian @short получить значение строкового объекта по заданному @ref jconf "имени".
//! @~english @short retrieve string value of @ref jconf "named" object.
const char* jcf_s ( JCFobj node, const char* name );

//! @~russian @short получить значение объекта типа @c long по заданному @ref jconf "имени".
//! @~english @short retrieve value of @ref jconf "named" object of type @c long.
long jcf_l ( JCFobj node, const char* name );

//! @~russian @short получить значение объекта типа @c double по заданному @ref jconf "имени".
//! @~english @short retrieve value of @ref jconf "named" object of type @c double.
double jcf_d ( JCFobj node, const char* name );

//! @~russian @short получить значение объекта типа @c bool по заданному @ref jconf "имени".
//! @~english @short retrieve value of @ref jconf "named" object of type @c bool.
int jcf_b ( JCFobj node, const char* name );

//! @}

#endif // CONF_H_

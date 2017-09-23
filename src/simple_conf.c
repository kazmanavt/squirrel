/*!
  @page simple_conf Простые конфигурационные файлы
  Функция реализованная в @ref simple_conf.c и продекларированная в
  @ref simple_conf.h предназначается для работы с простейшими конфигурационными файлами.
  Такие файлы не обязаны следовать какому-либо определенному формату, способ чтения
  параметров из такого файла определяется непосредственно при самом вызове.

  Особенностью простых конфигурационных файлов помимо свободного формата считается отсутствие
  определенной структуры, их разбор ведется на уровне строк. И значение параметра в какой-либо
  строке никак не влияет на интерпретацию других строк.
*/

/*!
  @file simple_conf.с
  @short тривиальные конфигурационные файлы, реализация.

  @see @ref simple_conf @n
  @date 15.09.2013
  @author masolkin@gmail.com, v1925@mail.ru
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <kz_erch.h>

/*!
  @details
  Первым аргументом функции задается источник для чтения конфигурационных параметров.
  Остальные аргументы функции должны представлять собой пары - формат/указатель.
  Список аргументов должен в обязательном порядке завершаться значением @c NULL.

  Формат должен быть строкой подходящей для передачи в качестве формата библиотечной функции
  @c scanf(), и задающей чтение @em только @em одного! параметра. Указатель должен указывать на
  переменную подходящую для приема читаемого по формату значения.

  Из заданного источника строки вычитываются последовательно и циклически читаются с помощью
  вызова @c scanf() для каждой пары формат/указатель. Таким образом если формат/указатель
  могут быть прочитаны из нескольких строк, то по завершении функции сохраняется только
  последнее успешно прочитанное значение.

  @param[in] cfname имя конфигурационного файла с параметрами для чтения. Если аргумент
                    задан строкой вида @c "-", то вместо файла читается стандартный ввод,
                    до @em конца!
  @returns При возникновении ошибок возвращается @c -1, при успешном завершении возвращается
           количество успешно прочитанных значений.
*/
int sc_read_parms ( char *cfname, ... ) {
  FILE *CFG = NULL;
  /* -1 - ERROR, 0 - no items found in file, >0 number of instances matched to pattern */
  int retval = 0;
  if ( strcmp ( cfname, "-" ) == 0 ) {
    CFG = stdin;
  } else {
    EC_NULL ( CFG = fopen ( cfname, "rt" ) );
  }

  char *line = NULL;
  size_t n;

  while ( 1 ) {
    EC_NEG1 ( getline ( &line, &n, CFG ) );
    // int i = 0;
    va_list ap;
    va_start ( ap, cfname );
    char *fmt;
    void *val;
    while ( ( fmt = va_arg ( ap, char * ) ) != NULL ) {
      if ( ( val = va_arg ( ap, void * ) ) == NULL ) {
        va_end ( ap );
        EC_FAIL;
      }
      if ( sscanf ( line, fmt, val ) == 1 ) {
        int width = 1;
        if ( sscanf ( fmt, " %*s = %%%dc", &width ) == 1 ) {
          ( ( char * ) val ) [width - 1] = '\0';
          char *lf;
          if ( ( lf = strchr ( val, '\n' ) ) != NULL ) {
            * lf = '\0';
          }
          retval++;
        }
      }
    }
    va_end ( ap );
    free ( line );
    line = NULL;
  }

  if ( CFG != stdin ) {
    EC_NZERO ( fclose ( CFG ) );
  }

  return retval;

  EC_CLEAN_SECTION (
    if ( line != NULL ) free ( line );
  if ( CFG != NULL ) {
    if ( feof ( CFG ) ) {
        ec_clean ();
        if ( CFG != stdin ) {
          fclose ( CFG );
        }
        return retval;
      }
      if ( CFG != stdin ) {
        fclose ( CFG );
      }
    }
  return -1;
  );
}

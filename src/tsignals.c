/*!
  @weakgroup tsigs
  @ingroup intern
  @{
*/

/*!
  @file tsignals.c
  @short реализация функций для работы с описаниями технологических сигналов.

  @see @ref tsigs @n
  @date 27.04.2013
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdint.h>
#include <math.h>

#include <kz_erch.h>
#include <tsignals.h>


/*!
  указатель на таблицу загруженных описателей сигналов.
*/
static SQTSigdef_t *signals = NULL;
const char **siglist = NULL;

/*!
  указатель на таблицу загруженных описателей сигналов.
*/
size_t num_signals = 0;

/*!
  @short читает из файла описания сигналов определенного типа.

  @param[in] fname имя файла из которого загружаются описания технологических сигналов
  @param[in] type тип загружаемых сигналов.
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
static inline
int read_sigs_definitions( const char *fname, SQTSignalType_t type )
{
  static const unsigned MAX_CFG_SLEN = 200;
  char kks[KKS_LEN + 1], tmp[MAX_CFG_SLEN + 1], *ttmp;

  FILE *F;
  EC_NULL( F = fopen( fname, "rt" ) );

  // create pattern for matching line format of config file, it should looks like
  // <SIG_NAME> <SIGCODE>[rest of string]
  regex_t m_head;
  EC_NZERO( regcomp( &m_head, "(\\w+?)\\s+([0-9]+?)\\s*(.*)", REG_EXTENDED ) );
  // create patterns for searching of key-value pairs in the rest of config line
  regex_t m_min, m_max, m_accuracy, m_delta;
  EC_NZERO( regcomp( &m_min, "min=([+-]?[0-9.]+|[+-]?INF)", REG_EXTENDED ) );
  EC_NZERO( regcomp( &m_max, "max=([+-]?[0-9.]+|[+-]?INF)", REG_EXTENDED ) );
  EC_NZERO( regcomp( &m_accuracy, "accuracy=([+-]?[0-9.]+)", REG_EXTENDED ) );
  EC_NZERO( regcomp( &m_delta, "delta=([+-]?[0-9.]+)", REG_EXTENDED ) );
  //struct for results
  regmatch_t found_subs[10];

  char *line = NULL;
  size_t n, l_num = 0;
  unsigned code;
  // reading signal table line by line
  while ( getline( &line, &n, F ) != -1 ) {
    ++l_num;
    double smin = -INFINITY, smax = INFINITY, saccuracy = 0, sdelta = 0;
    // extract leading part with <KKS> and numeric <CODE> and optional rest of line
    if ( regexec( &m_head, line, 10, found_subs, 0 ) == 0 ) {
      // check if we have 1st two fields
      if ( found_subs[1].rm_so != -1 && found_subs[2].rm_so != -1 ) {
        // extract KKS
        size_t len = found_subs[1].rm_eo - found_subs[1].rm_so;
        if ( len > KKS_LEN ) {
          fprintf( stderr, "WRN> Too long KKS (%s:%d)\n", fname, l_num );
          continue;
        }
        strncpy( kks, &line[found_subs[1].rm_so], len );
        kks[len] = '\0';

        // extract signal code
        len = found_subs[2].rm_eo - found_subs[2].rm_so;
        if ( len > MAX_CFG_SLEN ) {
          fprintf( stderr, "WRN> Too large code field (%s:%d)\n", fname, l_num );
          continue;
        }
        strncpy( tmp, &line[found_subs[2].rm_so], len );
        tmp[len] = '\0';
        errno = 0;
        char *eoc; // to catch end of conversion
        code = strtoul( tmp, &eoc, 10 );
        if ( errno ) {
          fprintf( stderr, "WRN> failed to get signal code (%s:%d)", fname, l_num );
          continue;
        } else if ( eoc == tmp ) {
          fprintf( stderr, "WRN> failed to get signal code (%s:%d): no digits", fname,
                   l_num );
          continue;
        } else if ( eoc != tmp + len ) {
          fprintf( stderr, "WRN> extra characters after code (%s:%d)", fname, l_num );
          continue;
        }

        //check if we have additional info for current signal
        int roffset = found_subs[3].rm_so;
        if ( roffset != -1 ) {
          //check for min value
          if ( regexec( &m_min, &line[roffset], 10, found_subs, 0 ) == 0 ) {
            len = found_subs[1].rm_eo - found_subs[1].rm_so;
            strncpy( tmp, &line[roffset + found_subs[1].rm_so], len );
            tmp[len] = '\0';
            errno = 0;
            smin = strtod( tmp, &eoc );
            if ( errno || eoc != tmp + len ) {
              ttmp = tmp;
              double sign = 1.;
              if ( tmp[0] == '-' ) {
                sign = -1.;
                ++ttmp;
              } else if ( tmp[0] == '+' ) {
                ++ttmp;
              }
              if ( strcmp( tmp, "INF" ) == 0 ) {
                smin = sign * INFINITY;
              } else {
                fprintf( stderr, "WRN> failed to get min value (%s:%d)\n", fname, l_num );
                smin = -INFINITY;
              }
            }
          }

          //check for max value
          if ( regexec( &m_max, &line[roffset], 10, found_subs, 0 ) == 0 ) {
            len = found_subs[1].rm_eo - found_subs[1].rm_so;
            strncpy( tmp, &line[roffset + found_subs[1].rm_so], len );
            tmp[len] = '\0';
            errno = 0;
            smax = strtod( tmp, &eoc );
            if ( errno || eoc != tmp + len ) {
              ttmp = tmp;
              double sign = 1.;
              if ( tmp[0] == '-' ) {
                sign = -1.;
                ++ttmp;
              } else if ( tmp[0] == '+' ) {
                ++ttmp;
              }
              if ( strcmp( tmp, "INF" ) == 0 ) {
                smax = sign * INFINITY;
              } else {
                fprintf( stderr, "WRN> failed to get max value (%s:%d)\n", fname, l_num );
                smax = INFINITY;
              }
            }
          }

          //check for accuracy value
          if ( regexec( &m_accuracy, &line[roffset], 10, found_subs, 0 ) == 0 ) {
            len = found_subs[1].rm_eo - found_subs[1].rm_so;
            strncpy( tmp, &line[roffset + found_subs[1].rm_so], len );
            tmp[len] = '\0';
            errno = 0;
            saccuracy = strtod( tmp, &eoc );
            if ( errno || eoc != tmp + len ) {
              fprintf( stderr, "WRN> failed to get accuracy value (%s:%d)\n", fname, l_num );
              saccuracy = 0;
            }
          }

          //check for accuracy value
          if ( regexec( &m_delta, &line[roffset], 10, found_subs, 0 ) == 0 ) {
            len = found_subs[1].rm_eo - found_subs[1].rm_so;
            strncpy( tmp, &line[roffset + found_subs[1].rm_so], len );
            tmp[len] = '\0';
            errno = 0;
            sdelta = strtod( tmp, &eoc );
            if ( errno || eoc != tmp + len ) {
              fprintf( stderr, "WRN> failed to get delta value (%s:%d)\n", fname, l_num );
              sdelta = 0;
            }
          }

        }
        // //TODO
        //       printf("DBG> line to split [%s]\n", line);
        //       printf("DBG> Converted kks(%s):code(%d):min(%f):max(%f):accuracy(%f):delta(%f)\n", kks, code, smin, smax, saccuracy, sdelta);
      } else {
        fprintf( stderr, "WRN> bad line in signal table (%s:%d)\n", fname, l_num );
      }
    } else {
      fprintf( stderr, "WRN> line dousn't fit (%s:%d)", fname, l_num );
    }

    // add signal definition to table
    EC_REALLOC( signals, ( num_signals + 1 ) * sizeof( SQTSigdef_t ) );
    signals[num_signals].type = type;
    strcpy( signals[num_signals].kks, kks );
    signals[num_signals].code = code;
    signals[num_signals].smin = smin;
    signals[num_signals].smax = smax;
    signals[num_signals].accuracy = saccuracy;
    signals[num_signals++].delta = sdelta;
    EC_REALLOC( siglist, ( num_signals ) *sizeof( char * ) );
    EC_NULL( siglist[num_signals - 1] = strdup( kks ) );
  }
  free( line );

  if ( !feof( F ) ) {
    fprintf( stderr, "WRN> read from <%s> failure", fname );
  }

  regfree( &m_head );
  regfree( &m_min );
  regfree( &m_max );
  regfree( &m_accuracy );
  regfree( &m_delta );

  fclose( F );

  return 0;

  EC_CLEAN_SECTION(
    if ( F != NULL ) fclose( F );
    regfree( &m_head );
    regfree( &m_min );
    regfree( &m_max );
    regfree( &m_accuracy );
    regfree( &m_delta );
    return -1;
  );
}

/*!
  @short сравнивает два описателя сигнала @ref SQTSigdef_t по полю @c kks.
*/
int compare_sigdef( const void *p1, const void *p2 )
{
  return strcmp( ( ( SQTSigdef_t * ) p1 )->kks, ( ( SQTSigdef_t * ) p2 )->kks );
}


/*!
  \details
  Функция загружает описания сигналов запрошенных приложением, попутно производится
  проверка на наличие всех запрошенных сигналов. Отсутствие какого-либо из сигналов
  является ошибкой.

  После успешного завершения данного вызова все последующие обращения вызовут ошибку.
  @param[in] desired_signals массив указателей на строки содержащие имена сигналов
                             описания которых должны быть загружены.
  @param[in] num_desired_signals   размерность массива передаваемого в аргументе @c desired_signals.
  @retval  0 в случае успеха
  @retval -1 при неудачном завершении
*/
int sqts_load_signals( const char **desired_signals, int num_desired_signals )
{
  static char loaded = 0;

  if ( loaded ) {
    EC_UMSG( "attempt to load signal tables twice" );
  }

  char *sigdef_fname[END] = { "./name_ia.dat" };
  // load sig tables file by file one sig type per table/file
  for ( int i = 0; i < END; i++ ) {
    EC_NEG1( read_sigs_definitions( sigdef_fname[i], i ) );
  }
  qsort( signals, num_signals, sizeof( SQTSigdef_t ), compare_sigdef );

  SQTSigdef_t *_signals = NULL;
  if ( desired_signals != NULL ) {
    EC_NULL( _signals = calloc( num_desired_signals, sizeof( SQTSigdef_t ) ) );
    for ( int s = 0; s < num_desired_signals; s++ ) {
      SQTSigdef_t key;
      strncpy( key.kks, desired_signals[s], KKS_LEN );
      SQTSigdef_t *sig_loa = bsearch( &key, signals, num_signals, sizeof( SQTSigdef_t ), compare_sigdef );
      if ( sig_loa == NULL ) {
        EC_UMSG( "Configured signal '%s' has not found in signal tables!", desired_signals[s] );
      }
      _signals[s] = *sig_loa;
    }
    free( signals );
    signals = _signals;
    num_signals = num_desired_signals;
  }

  loaded = 1;
  return 0;

  EC_CLEAN_SECTION(
    if ( signals != NULL )
    free( signals );
    if ( _signals != NULL )
      free( _signals );
      signals = NULL;
      num_signals = 0;
      return -1;
    );
}


/*!
  @param[in] kks имя сигнала описание которого требуется найти.
  @returns указатель на структуру SQTSigdef_t в которой при успешном завершении
           будет содержаться описание сигнала с именем заданным аргументом @c kks.
           или -1 в случае возникновения ошибок.
*/
const SQTSigdef_t *sqts_get_sigdef( const char *kks )
{
  SQTSigdef_t key;
  strncpy( key.kks, kks, KKS_LEN );
  key.kks[KKS_LEN] = '\0';
  return bsearch( &key, signals, num_signals, sizeof( SQTSigdef_t ),
                  compare_sigdef );
}


/*!
  @details В случае успеха создается массив указателей на строки с именами загруженных сигналов.
    @warning выделенная память должна освобождаться вызывающей стороной.
  @param[out] siglist в случае успешного завершения, здесь возвращается указатель на
              сформированный список сигналов.
  @param[out] num длинна сформированного списка сигналов.
  @retval  0 в случае успеха
  @retval -1 при неудачном завершении
*/
void sqts_list_sig_names( const char *const **_siglist, int *num )
{
  *_siglist = siglist;
  *num = num_signals;
  // const char**  list;
  // EC_NULL ( list = malloc ( num_signals * sizeof ( char* ) ) );
  // for ( int i = 0; i < num_signals; ++i ) {
  //   list[i] = signals[i].kks;
  // }
  // *num = num_signals;
  // *siglist = list;
  // return 0;
  // EC_CLEAN_SECTION (
  //   return -1;
  // );
}
//! @}

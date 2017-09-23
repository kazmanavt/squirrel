/*!
  @addtogroup deffilters
  @{

  @defgroup fgrmedian Медианный фильтр

  @short Это простейший фильтр выполняющий сглаживание со скользящим окном.

  Входом для данного фильтра служит вектор размерности @b n содержащий новые значения
  параметров подлежащих фильтрации в формате @c float. Ширина окна фильтрации @b w и
  размерность входного вектора могут быть заданы при создании экземпляра фильтра
  строкой инициализации вида:@n
  @verbatim "n w" @endverbatim

  Алгоритм фильтрации задается формулой вида:
  @f[
    \overline{x_k} = \frac{1}{N} \sum_{i=k-N}^k x_i
  @f]
  здесь @f$ N @f$ - соответствует ширине окна @b w. А для @f$ i < 0, x_i = 0 @f$.

  Отфильтрованые значения входных параметров возвращаются во входном векторе в
  соответствующем порядке.

  @{
*/

/*!
  @file median.c

  @short реализация медианного фильтра.

  @see @ref fgrmedian @n
  @date 12.11.2013
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
#include <stdint.h>

#include <kz_erch.h>

//! имя фильтра
const char* name = "median";
//! мажорная версия фильтра
#define VERSION_MAJOR 1
//! минорная версия фильтра
#define VERSION_MINOR 0
//! версия фильтра
const uint32_t version = 100 * VERSION_MAJOR + VERSION_MINOR;
//! версия протокола
const uint32_t protocol = 0;

//! таблица дескрипторов экземплярорв фильтра
typedef struct {
  long length;  //!< длина интервала для медианного фильтра
  long mask;
  long dim;     //!< incomming vector size
  long* pos;
  float* sum;
  float** data; /*!< массив для входных данных, выделяется на этапе инициализации
                    как `data[dim][length]` */
  int ( *func ) ();
} FMedian_t;

FMedian_t** filters_tbl = NULL;
int num_filters = 0;


int filter_next ( FMedian_t* ds, float* val )
{
  for ( int i = 0; i < ds->dim; ++i ) {
    ds->sum[i] += val[i] - ds->data[i][ds->pos[i]];
    ds->data[i][ds->pos[i]] = val[i];
    ++ds->pos[i];
    ds->pos[i] &= ds->mask;
    val[i] = ds->sum[i] / ds->length;
  }

  return 0;
}

int filter_1st ( FMedian_t* ds, float* val )
{
  ds->func = filter_next;

  for ( int i = 0; i < ds->dim; ++i ) {
    for ( int j = 0; j < ds->length; ++j ) {
      ds->data[i][j] = val[i];
      ds->sum[i] += val[i];
    }
  }

  return 0;
}
int run ( FMedian_t* ds, float* val )
{
  return ds->func ( ds, val );
}
// int (*run) (void *, void *) = (int (*) (void *, void *))filter_1st;

/*! @short creates median filter instance

  will create and initialize median filter instance
  @param len decoded as follows:
             - ((int*)one)[0] -- dimention of input vector, may be initialized to arbitrary number of incomming signals
             - ((int*)one)[1] -- length of filter
  @param ds huhum
  @return num of filter created -1 on error
*/
void* create ( const char* init )
{
  int dim, width;
  EC_CHCK0 ( sscanf ( init, "%d %d", &dim, &width ), 2, != );
  int l = width, pow = 0;
  while ( l >>= 1 ) ++pow;
  l = 1 << pow;

  FMedian_t* mfd;
  EC_NULL ( mfd = calloc ( 1, sizeof ( FMedian_t ) ) );
  mfd->dim = dim;
  mfd->func = filter_1st;
  mfd->length = l;
  mfd->mask = l - 1;
  EC_NULL ( mfd->data = calloc ( dim, sizeof ( float* ) ) );
  EC_NULL ( mfd->pos = calloc ( dim, sizeof ( long ) ) );
  EC_NULL ( mfd->sum = calloc ( dim, sizeof ( float ) ) );
  for ( int i = 0; i < dim; ++i ) {
    EC_NULL ( mfd->data[i] = calloc ( l, sizeof ( float ) ) );
    mfd->pos[i] = 0;
  }
  printf ( "  median filter created (window: %d)\n", l );
  return mfd;

  EC_CLEAN_SECTION (
    return NULL;
  );
}

//! @}
//! @}

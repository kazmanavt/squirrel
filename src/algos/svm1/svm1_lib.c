#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
// #include "algo.h"
#include <rail.h>
#include "svm.h"
#include "svm1.h"

#ifndef Malloc
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
#endif


#define MODELSERILIZE_MOD(model,member, size){\
    byteOut=realloc(byteOut,shift+(offset=size)); \
    memcpy(&byteOut[shift],model->member,size);\
    pszDesc->sz_##member=size;\
    shift+=offset;\
  }

#define MODEL_DE_SERILIZE_MOD(model,member){\
    if(pszDesc->sz_##member != 0)\
    {\
      memcpy(model->member,&byteIn[shift],(offset=pszDesc->sz_##member)); \
      shift += offset;\
    }\
  }

#define DESER_ALLOC(member) {\
    if(pszDesc->sz_##member != 0)\
      model->member=malloc(pszDesc->sz_##member); \
  }

void printSparceNodes ( struct svm_node* pn )
{
  int j = 0;

  while ( pn[j].index != -1 ) {
    printf ( "%d:%f ", pn[j].index, pn[j].value );
    j++;
  }
  printf ( "\n" );

}

/* **************************************************************************************
 */
int svm_fake_read ( char* fileName, SQPsignal_t* buff, int step )
{
  int i = step, k;
  FILE* IN = fopen ( fileName, "r" );
  if ( IN == NULL ) {
    perror ( "ERR> open incomming signals file failure: " );
    return 0;
  }
  size_t bs = 1024;
  double ( *a ) [17], dval;
  size_t n = 0, N = bs;
  a = malloc ( N * 17 * sizeof ( double ) );

  while ( fscanf ( IN, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
                   &a[n][0],  &a[n][1],  &a[n][2],  &a[n][3],
                   &a[n][4],  &a[n][5],  &a[n][6],  &a[n][7],
                   &a[n][8],  &a[n][9],  &a[n][10], &a[n][11],
                   &a[n][12], &a[n][13], &a[n][14], &a[n][15], &a[n][16] ) == 17 ) {

    ++n;
    if ( n == N ) {
      a = realloc ( a, ( N + bs ) * 17 * sizeof ( double ) );
      if ( a == NULL ) {
        perror ( "ERR> memory alloc0 for inp task failed: " );
        /// TODO change to exit
        _exit ( -1 );
      }
      N += bs;
    }
  }

  i = step % n;
  a = realloc ( a, n * 17 * sizeof ( double ) );

  for ( k = 0; k < 16; k++ ) {
    buff[k].trust = 0.0;
    buff[k].val = a[i][k + 1];
    buff[k].ts.sec = ( uint_fast32_t ) ( a[i][0] );
    dval = ( ( ( double ) a[i][0] - ( double ) buff[k].ts.sec ) * 1000. );
    buff[k].ts.usec = ( uint_fast32_t ) ( dval + 0.5 );
    printf ( "----fake read  K=%d Time=%d s(%d ms) Cod=%d %f Trust=%f \n", k, buff[k].ts.sec, buff[k].ts.usec,
             buff[k].code, buff[k].val, buff[k].trust );

  }
  fclose ( IN );
  return 16;

}
/* *******************************************************************************************************
*  */
struct svm_model* svm1_deserilizeModel ( char* byteIn, SerilizeModelStruct* pszDesc )
/* de serlize svm model from  byte stream, return the model structure  */
{
  struct svm_model* model = Malloc ( struct svm_model, 1 );
  int l, nr_class, m, i  ;
  int shift = 0, offset = 0;
#ifdef DEBUG_PRINT
  static int iter;
#endif
  model->sv_indices = NULL;
  model->rho = NULL;
  model->probA = NULL;
  model->probB = NULL;
  model->label = NULL;
  model->nSV = NULL;
  MODEL_DE_SERILIZE_MOD ( &model, param )
  MODEL_DE_SERILIZE_MOD ( &model, nr_class )
  MODEL_DE_SERILIZE_MOD ( &model, l )
  l = model->l;
  nr_class = model->nr_class;
  m = model->nr_class - 1;

  DESER_ALLOC ( rho )
  MODEL_DE_SERILIZE_MOD ( model, rho )
  DESER_ALLOC ( label )
  MODEL_DE_SERILIZE_MOD ( model, label )
  DESER_ALLOC ( probA )
  MODEL_DE_SERILIZE_MOD ( model, probA )
  DESER_ALLOC ( probB )
  MODEL_DE_SERILIZE_MOD ( model, probB )
  DESER_ALLOC ( nSV )
  MODEL_DE_SERILIZE_MOD ( model, nSV )

  model->sv_coef = Malloc ( double*, m );

  for ( i = 0; i < m; i++ ) {
    model->sv_coef[i] = malloc ( pszDesc->sz_sv_coef );
    offset = pszDesc->sz_sv_coef;
    memcpy ( model->sv_coef[i], &byteIn[shift], offset );
    shift += offset;
  }

  model->SV = Malloc ( struct svm_node*, l );

  for ( i = 0; i < l; i++ ) {

    model->SV[i] = malloc ( pszDesc->sz_svm_node[i] );
    if ( ( offset = pszDesc->sz_svm_node[i] ) == -1 )
      break;
    memcpy ( model->SV[i], &byteIn[shift], offset );
    shift += offset;
  }
#if 0
  {
    char modName[80];
    static int iter;
    sprintf ( modName, "mod%dOut.txt", iter++ );
    svm_save_model ( modName, model );
  }

#endif

  return model;


}


/* *******************************************************************************************************
 */
char*  svm1_serilizeModel ( struct svm_model* model, SerilizeModelStruct* pszDesc, int problemSize, int modelWInSize )
/* serlize svm model to byte stream, return the byte stream and serilize size dimension */
{
  char* byteOut = malloc ( sizeof ( struct svm_parameter ) );
  int i, j, sum_sv_len = 0;
  int shift = 0, offset = 0;
  int nr_class = model->nr_class;
  int l = model->l, m = nr_class - 1, L = modelWInSize;
  double* const* sv_coef = model->sv_coef;
  struct svm_node** SV = model->SV;
#ifdef DEBUG_PRINT
  static int iter;
#endif


  MODELSERILIZE_MOD ( &model, param, sizeof ( struct svm_parameter ) )
  MODELSERILIZE_MOD ( &model, nr_class, sizeof ( int ) )
  MODELSERILIZE_MOD ( &model, l, sizeof ( int ) )
  MODELSERILIZE_MOD ( model, rho, sizeof ( double ) * ( nr_class * ( nr_class - 1 ) / 2 ) )
  if ( model->label )
    MODELSERILIZE_MOD ( model, label, sizeof ( int ) * ( nr_class ) )
    if ( model->probA ) // regression has probA only
      MODELSERILIZE_MOD ( model, probA, sizeof ( double ) * ( nr_class * ( nr_class - 1 ) / 2 ) )
      if ( model->probB ) // regression has probA only
        MODELSERILIZE_MOD ( model, probB, sizeof ( double ) * ( nr_class * ( nr_class - 1 ) / 2 ) )
        if ( model->nSV ) // regression has probA only
          MODELSERILIZE_MOD ( model, nSV, sizeof ( int ) * ( nr_class * ( nr_class - 1 ) / 2 ) )


          byteOut = realloc ( byteOut, shift + ( offset = m * L * sizeof ( double ) ) ); /* allocate maximum size */

  for ( i = 0; i < m; i++ ) {
    memcpy ( &byteOut[shift], sv_coef[i], ( offset = l * sizeof ( double ) ) );
    shift += ( offset = L * sizeof ( double ) );
  }
  pszDesc->sz_sv_coef = offset;

  for ( i = 0; i < L; i++ ) {
    byteOut = realloc ( byteOut, shift + ( offset = ( problemSize ) * sizeof ( struct svm_node ) ) );

    if ( i < l ) {
      j = 0;
      while ( SV[i][j].index != -1 )
        j++;

      /* allocate maximum space regradlsess of sparce matrix */
      memcpy ( &byteOut[shift], SV[i], ( j + 1 ) *sizeof ( struct svm_node ) ); /* copy only real number of items not maximum */
    }

    shift += offset;

    if ( i < SVM_NODE_MAX_LENGTH - 1 ) {
      pszDesc->sz_svm_node[i] = offset;
      sum_sv_len += offset;
    } else
      break;
  }
  pszDesc->sz_svm_node[i] = -1; /* terminate the sequence */


  pszDesc->sz_totalModel = pszDesc->sz_param + pszDesc->sz_nr_class + pszDesc->sz_l + sum_sv_len +
                           m * pszDesc->sz_sv_coef + pszDesc->sz_rho + pszDesc->sz_probA + pszDesc->sz_probB +
                           pszDesc->sz_label + pszDesc->sz_nSV + pszDesc->sz_free_sv;
  pszDesc->sz_total = pszDesc->sz_totalModel + pszDesc->sz_vmax + pszDesc->sz_vmin + pszDesc->sz_lower + pszDesc->sz_upper + sizeof ( int ) + sizeof ( SerilizeModelStruct );

#ifdef DEBUG_PRINT
  {
    char modName[80];
    sprintf ( modName, "mod%dIn.txt", iter++ );
    svm_save_model ( modName, model );
  }

#endif
  return byteOut;


}

double  output ( double value, double upper, double lower,  double vmax, double vmin )
{
  /* skip single-valued attribute */
  if ( vmax == vmin )
    return value;

  if ( value ==  vmin )
    value = lower;
  else if ( value ==  vmax )
    value = upper;
  else
    value = lower + ( upper - lower ) *
            ( value - vmin ) /
            ( vmax - vmin );

  return value;

}



/* *******************************************************************************************
*/
/* locate min max in column order of x array */
void find_min_max ( struct svm_node* x[], double* maxar, double* minar,  int ysize )
{
  int i, j, l = 0,  next_index = 2, index;

  /* init default values */
  for ( j = 0; j < ysize; j++ ) {
    next_index = 2;
    l = 0;

    while ( ( index = x[j][l].index ) != -1 ) {

      if ( j == 0 ) {
        maxar[index - 1] = -DBL_MAX;
        minar[index - 1] = DBL_MAX;
      }
      for ( i = next_index; i < index; i++ ) {
        // ??????????
        if ( j == 0 ) {
          maxar[i - 1] = -DBL_MAX;
          minar[i - 1] = DBL_MAX;
        }
        maxar[i - 1] = max ( maxar[i - 1], 0 );
        minar[i - 1] = min ( minar[i - 1], 0 );
      }

      maxar[index - 1] = max ( maxar[index - 1], x[j][l].value );
      minar[index - 1] = min ( minar[index - 1], x[j][l].value );
      next_index = index + 1;
      l++;

    }
  }



}
/* ***********************************************************************************************
*/
int  scale_svm_array ( struct svm_node* x,  struct svm_node* y , double lower, double upper, double* pvmin, double* pvmax )
{
  int i = 0, j = 0, next_index = 1, index;

  while ( ( index = x[j].index ) != -1 ) {
    for ( i = next_index; i < index; i++ ) {
      y[i - 1].value = output ( 0, upper, lower, pvmax[i - 1], pvmin[i - 1] );
      y[i - 1].index = i;
    }

    y[index - 1].value = output ( x[index - 1].value, upper, lower, pvmax[index - 1], pvmin[index - 1] );
    y[index - 1].index = index;
    next_index = index + 1;
    j++;
  }


  /* copy from temporary y to permanent x location */
  y[next_index - 1].index = -1;
  j = 0; i = 0;

  while ( y[j].index != -1 ) {
    if ( y[j].value != 0 ) {
      x[i] = y[j];
      i++;
    }
    j++;
  }
  x[i] = y[j]; /* -1 terminate it */



  return next_index;

}
#if 0
int _main ( int argc, char** argv )
{
  double lower = -1.0, upper = 1.0;
  double vmax = -DBL_MAX;
  double vmin = DBL_MAX;
}
#endif


/* **********************************************************************************************
*/
void svm1_cnvt_sq_to_svm_data ( SQPsignal_t* svm1_buf, struct svm_node* xx, int svm1_size )
{
  int i;
  int max_index = 1;

  /* convert sq signals to svm structure */
  for ( i = 0; i <  svm1_size; i++ ) {
    /* check that value is sparce  */
    if ( svm1_buf[i].val != 0 ) {
      // printf("Got value Cod=%d %f\n",svm1_buf[i].code,svm1_buf[i].val);
      xx[max_index - 1].index = i + 1;
      xx[max_index - 1].value = ( double ) svm1_buf[i].val;
      max_index++;
    }


    /* really it is necessary only for model */

    /* that number of bad data */

  }
#if 0
  printf ( "----svm1_cnvt_sq_to_svm_data-lst data Cod=%d %f Trust=%f Time= %d.%d \n",
           svm1_buf[svm1_size - 1].code, svm1_buf[svm1_size - 1].val, svm1_buf[svm1_size - 1].trust,
           svm1_buf[svm1_size - 1].ts.sec, svm1_buf[svm1_size - 1].ts.usec );
#endif
  xx[max_index - 1].index = -1;

}

/* *********************************************************
 * */
/* check if data is valid */
int svm1_dataIsValid ( SQPsignal_t* buff, int buf_len )
{
  int i;

  for ( i = 0; i < buf_len; i++ ) {
    if ( buff[i].trust != 1.0 && ( buff[i].ts.sec != 0 ||  buff[i].ts.usec != 0 ) )
      return 1; /* AT LEASE ONE trusted data found
                       * the trust is 0 !!!! if data is valid */
  }

  return 0;
}

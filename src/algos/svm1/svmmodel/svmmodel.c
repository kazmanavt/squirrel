#define _ISOC99_SOURCE 1
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <mcheck.h>
#include <malloc.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <float.h>
#include <ext_rail.h>
#include <time.h>
#include "simple_conf.h"
/* #include "worker.h" */
#include "svm.h"
#include "svm1.h"

/* svm globals */
static int PROBLEM_SIZE_SVM = 0;
static int model_window = 400; /* model learn space size */

SQPsignal_t **svm1buf;
SQPsignal_t **svm2buf;
int *updateDignalsFlgs;
int no_svm_scale = 1;

double upper = 1.0, lower = -1.0;
struct svm_parameter param;             // set by parse_command_line
struct svm_problem prob;                // set by read_problem
struct svm_model *model;
struct svm_node **x;
struct svm_node **y;
double *vmax;
double *vmin;
int shmid;
int cross_validation;
int nr_fold;
int updateModelOnlyOnce = 1;

void svmmodel_init()
{
  int n;
  char fconf[128] = "svm.conf";


  /* read the model step size in lags */
  sc_read_parms( fconf, "MODEL_WINDOW = %d", &model_window, NULL );
  sc_read_parms( fconf, "NO_SVM_SCALE = %d", &no_svm_scale, NULL );
  sc_read_parms( fconf, "UPDATE_MODEL_ONLY_ONCE = %d", &updateModelOnlyOnce, NULL );






  /* read set of signals */

  svm1buf = ( SQPsignal_t ** )malloc( sizeof( SQPsignal_t * )*model_window );
  svm2buf = ( SQPsignal_t ** )malloc( sizeof( SQPsignal_t * )*model_window );
  x = ( struct svm_node ** )malloc( sizeof( struct svm_node * )*model_window );
  y = ( struct svm_node ** )malloc( sizeof( struct svm_node * )*model_window );

  for ( n = 0; n < model_window; n++ ) {
    svm1buf[n] = ( SQPsignal_t * )malloc( sizeof( SQPsignal_t ) * PROBLEM_SIZE_SVM );
    svm2buf[n] = ( SQPsignal_t * )malloc( sizeof( SQPsignal_t ) * PROBLEM_SIZE_SVM );
    x[n] = calloc( PROBLEM_SIZE_SVM + 1, sizeof( struct svm_node ) );
    y[n] = calloc( PROBLEM_SIZE_SVM + 1, sizeof( struct svm_node ) );
  }
  vmax = ( double * )malloc( PROBLEM_SIZE_SVM * sizeof( double ) );
  vmin = ( double * )malloc( PROBLEM_SIZE_SVM * sizeof( double ) );
  updateDignalsFlgs = calloc( PROBLEM_SIZE_SVM, sizeof( int ) );



}

/* ************************************************************************************
*/
void parse_command_line( int argc, char **argv )
{
  int i;
  void ( *print_func )( const char * ) = NULL; // default printing to stdout

  // default values
  param.svm_type = ONE_CLASS;
  param.kernel_type = RBF;
  param.degree = 3;
  param.gamma = 0;        // 1/num_features
  param.coef0 = 0;
  param.nu = 0.025;
  param.cache_size = 100;
  param.C = 1;
  param.eps = 1e-3;
  param.p = 0.1;
  param.shrinking = 1;
  param.probability = 0;
  param.nr_weight = 0;
  param.weight_label = NULL;
  param.weight = NULL;
  cross_validation = 0;

  // parse options
  for ( i = 1; i < argc; i++ ) {
    if ( argv[i][0] != '-' ) { break; }
    if ( ++i >= argc )
    { exit_with_help(); }
    switch ( argv[i - 1][1] ) {
      case 'c':
        param.C = atof( argv[i] );
        break;
      case 's':
        param.svm_type = atoi( argv[i] );
        break;


      case 't':
        param.kernel_type = atoi( argv[i] );
        break;
      case 'd':
        param.degree = atoi( argv[i] );
        break;
      case 'g':
        param.gamma = atof( argv[i] );
        break;
      case 'r':
        param.coef0 = atof( argv[i] );
        break;
      case 'n':
        param.nu = atof( argv[i] );
        break;
      case 'm':
        param.cache_size = atof( argv[i] );
        break;
      case 'e':
        param.eps = atof( argv[i] );
        break;
      case 'h':
        param.shrinking = atoi( argv[i] );
        break;
      case 'q':
        print_func = &print_null;
        i--;
        break;
      case 'v':
        cross_validation = 1;
        nr_fold = atoi( argv[i] );
        if ( nr_fold < 2 ) {
          fprintf( stderr, "n-fold cross validation: n must >= 2\n" );
          exit_with_help();
        }
        break;
      case 'w':
        ++param.nr_weight;
        param.weight_label = ( int * )realloc( param.weight_label, sizeof( int ) * param.nr_weight );
        param.weight = ( double * )realloc( param.weight, sizeof( double ) * param.nr_weight );
        param.weight_label[param.nr_weight - 1] = atoi( &argv[i - 1][2] );
        param.weight[param.nr_weight - 1] = atof( argv[i] );
        break;
      default:
        fprintf( stderr, "Unknown option: -%c\n", argv[i - 1][1] );
        exit_with_help();
    }
  }

  // svm_set_print_string_function(print_func);

  // determine filenames

}


int main( int argc, char *argv[] )
{
  SQAlgoContext_t *psqc;
  unsigned long  SLEEP_SOFT;
  char shm_name[] = "/SVM1_MODEL";
  // static SQA_AlgoDescriptor_t *ha[2];
  int i, elements, inst_max_index = -1, max_index = -1;
  int sig_len;
  int realread,  initPass = 0;
  static SerilizeModelStruct serlizeSize;
  char *ser_model;
  char  *atomicBuffer;
  int cnt = 0;

  mcheck( NULL );
  mallopt( M_TRIM_THRESHOLD, 0 );
  mallopt( M_MMAP_MAX, 0 );


  chdir( "../../../.." ); /* change to squirrel working didrectory */

  /* parse command line */
  parse_command_line( argc, argv );


  if ( fl_init() == -1 ) {
     fprintf(stderr, "Failed to init logger\n");
     exit(1);
  }

  if ( ( psqc = sq_rail_init( "svm1" ) ) == 0 ) {
    fprintf( stderr, "Error in nit rail  \n" );
    exit( 1 );
  }
  PROBLEM_SIZE_SVM = sig_len = psqc->num_sig;
  SLEEP_SOFT = psqc->tic / 1000L;
  svmmodel_init();







  for ( ;; ) { /* do it forever */
    i = 0;

    fprintf( stderr, " Collect data\n" );
#if 1
    if ( initPass ) { /* first time start from very begining */

      if ( sqr_seek( 0 ) != 1 )
      { fprintf( stderr, "*E* Can't set signal's position\n" ); }
    }
#endif
    do {
      /* reset the input signal buffer position  to the haad of the signal's  data */

         fprintf(stderr,"Start data collection for window....%d \n",i);
      if ( i > 0 )
        /* copy previous sigbuf because updated will be only signal that changed science last update */
      { memcpy( svm1buf[i], svm2buf[i - 1], sizeof( SQPsignal_t ) * ( sig_len ) ); }

      /* get data  poll if necessary for the data */
#ifndef  INPUT_DEBUG
      if ( ( realread = sqr_read( svm1buf[i], NULL ) ) != sig_len ) {
#if 0
        fprintf( stderr, "*W* No new data\n" );
#else
        //   fprintf(stderr,"*W* No new data\n");
        usleep( SLEEP_SOFT / 4 ); /* waite the next lag */
        continue;
#endif
      }
#else  /* debug signals */
      realread = svm_fake_read( "testgood.dlm", svm1buf[i], cnt );
#endif
      if ( svm1_dataIsValid( svm1buf[i], sig_len ) == 0 ) {

        fprintf( stderr, "*W* No valid  data\n" );
        usleep( SLEEP_SOFT ); /* waite the next lag */
        continue;
      }
      memcpy( svm2buf[i], svm1buf[i], sizeof( SQPsignal_t ) * ( sig_len ) );
#ifdef  INPUT_DEBUG
      cnt++;
#endif
      i++;
      usleep( SLEEP_SOFT / 2 ); /* waite the next lag */

    } while ( i < model_window );

    /* model space initialization */
    {
      prob.l = model_window;
      elements = model_window * ( sig_len + 1 );
      prob.y = Malloc( double, prob.l );
      prob.x = Malloc( struct svm_node *, prob.l );
    }




    /* convert SQ to SVM data */
    for ( i = 0; i < model_window; i++ ) {

      svm1_cnvt_sq_to_svm_data( svm1buf[i], x[i], sig_len );
#if 0
      printSparceNodes( x[i] );
#endif

    }

    if ( no_svm_scale == 0 )
    { find_min_max( x, vmax, vmin,  model_window ); }

    /* scale data */
    for ( i = 0; i < model_window; i++ ) {
      if ( no_svm_scale == 0 ) {
        inst_max_index = scale_svm_array( x[i], y[i] , upper, lower, vmax, vmin ); /* the X wil be overwritten there */
        max_index = max( max_index, inst_max_index );
      }

      prob.x[i] = x[i];

      prob.y[i] = 1.0;

    }

    if ( param.gamma == 0 && max_index > 0 )
    { param.gamma = 1.0 / max_index; }


    model = svm_train( &prob, &param );
    serlizeSize.sz_vmin = sig_len * sizeof( vmin );
    serlizeSize.sz_vmax = sig_len * sizeof( vmax );
    serlizeSize.sz_lower = sizeof( lower );
    serlizeSize.sz_upper = sizeof( upper );

    ser_model = svm1_serilizeModel( model, &serlizeSize, PROBLEM_SIZE_SVM + 1, model_window );

    if ( initPass == 0 ) /* initialize the SHM memory segment as we
                               * only know the real model serilized size
                               */
    {
      int sz = serlizeSize.sz_total;
      atomicBuffer = malloc( serlizeSize.sz_total );
      if ( ( shmid = sqr_mb_acquire( shm_name, &sz, SQR_MB_ANY ) ) == -1 ) {
        fprintf( stderr, "Can't allocate shared model structure %s\n", shm_name );
        exit( 1 );
      }
      initPass = 1;
    }

    /* prepare atomic write */
    memcpy( atomicBuffer, ser_model, serlizeSize.sz_totalModel );
    memcpy( &atomicBuffer[serlizeSize.sz_totalModel] , vmin, serlizeSize.sz_vmin );
    memcpy( &atomicBuffer[serlizeSize.sz_totalModel + serlizeSize.sz_vmin], vmax, serlizeSize.sz_vmax );
    memcpy( &atomicBuffer[serlizeSize.sz_totalModel + serlizeSize.sz_totalModel + serlizeSize.sz_vmax + serlizeSize.sz_vmin]
            , &lower, serlizeSize.sz_lower );
    memcpy( &atomicBuffer[serlizeSize.sz_totalModel + serlizeSize.sz_totalModel + serlizeSize.sz_vmax + serlizeSize.sz_vmin + serlizeSize.sz_lower]
            , &upper, serlizeSize.sz_upper );
    memcpy( &atomicBuffer[serlizeSize.sz_totalModel + serlizeSize.sz_vmax + serlizeSize.sz_vmin +
                          serlizeSize.sz_lower + serlizeSize.sz_upper], &serlizeSize, sizeof( SerilizeModelStruct ) );

    {
      /* notify that model is ready */
      int flg = 1;
      fprintf(stderr,"Model is rteady \n");
      memcpy( &atomicBuffer[serlizeSize.sz_total - sizeof( int )], &flg, sizeof( int ) );

      /* make atomic write */
      if ( sqr_mb_write( shmid, 0, atomicBuffer, serlizeSize.sz_total, 0 ) )

      {
        fprintf( stderr, "can't save model to shm %s\n", shm_name );
        exit( 1 );
      }
      /* write min and max data */

    }
    svm_free_and_destroy_model( &model );
    fprintf( stderr, "Update model flags updateModelOnlyOnce=%d\n", updateModelOnlyOnce );

    if ( updateModelOnlyOnce == 1 ) {
      fprintf( stderr, "SVMMODEL:Model ready \nWe are sleeping calmly, all work done. Press <CTR>+C to abort as RT finishes .....\n" );
      pause();
    }


    // sleep(10000);

  }

}

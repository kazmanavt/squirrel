/*
 * svm1_main.c
 *
 *  Created on: 12.11.2012
 *      Author: user
 */

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <signal.h>
#include <rail.h>
#include <time.h>
#include <sys/timeb.h>
#include "rail.h"
#include "out.h"
#include "simple_conf.h"
#include "svm.h"
#include "svm1.h"
#include "fastlog.h"


char* name = "SVM1";
int version = 101;
int protocol = 0;

// #define DEBUG_VP_4 1
//#define DEBUG_VP 1
typedef struct SVMAlgoDescriptor_s {
  int REP_COUNTER; /* repeated counter failure window */

  char** sigs;
  SQTSigdef_t* siginfo;
  int PROBLEM_SIZE_SVM;
  int modelStep;
  int no_svm_scale;
  int updateModelOnlyOnce;


  SQPsignal_t* svm1buf;
  ; /* read buffer maximum and fixed length */
  SQPsignal_t* svm1bufwrite; /* some usefull write buffer */

  /* svm globals */
  struct svm_model* model;
  struct svm_node* x;
  struct svm_node* y;
  double* vmax;
  double* vmin;
  double upper;
  double lower;
  int shmid; /* set default as non initialized */
  int useStaticModel; /* non zero if using precompiled model  (recognize known situation)*/
  char modName[128];

  SerilizeModelStruct szDesc;
  unsigned int updateCounter; /* simple counter of the iteration */
  size_t  mb_size; /* model will be updated and each step */
  char* byteIn;
  int k;
  int repCounter;
} SVMAlgoDescriptor_t;

const SVMAlgoDescriptor_t SVM_DEFAULTS = {
  .REP_COUNTER = 4,
  .sigs = NULL,
  .siginfo = NULL,
  .PROBLEM_SIZE_SVM = 0,
  .modelStep = 400,
  .no_svm_scale = 1,
  .updateModelOnlyOnce = 1,
  .model = NULL,
  .x = NULL,
  .y = NULL,
  .vmax = NULL,
  .vmin = NULL,
  .upper = 1,
  .lower = 0xBADD,
  .shmid = -1,
  .modName = "svm.model"
};

int run ( SVMAlgoDescriptor_t* psvm )
{
  double target_label = 1.0, predict_label;
  size_t realread;
  int updateflug = 0; /*  will be non ther if the model was computed at least one time */

      // fprintf ( stderr, "SVM1 starting run\n");

  if ( psvm->useStaticModel == 0 ) {
    if ( psvm->shmid == -1 ) {
      /* get some space for flag it is zero till first update */
      if ( ( psvm->shmid = sqr_mb_acquire ( "/SVM1_MODEL", &psvm->mb_size, SQR_MB_GET ) ) == -1 ) {
        return 0;
      }
      fprintf ( stderr, "Done  shared model allocation  %s\n", "SVM1_MODEL" );
      psvm->byteIn = malloc ( psvm->mb_size ); /* allocate serilized buffer */
    }

    sqr_mb_read ( psvm->shmid, 0, psvm->byteIn, psvm->mb_size, 0 );
    memcpy ( &updateflug, &psvm->byteIn[psvm->mb_size - sizeof ( int )], sizeof ( int ) );
    sqr_mb_read ( psvm->shmid, psvm->mb_size - sizeof ( int ), &updateflug, sizeof ( int ), 0 );

    if ( !updateflug ) /* check that at least one model avaliable */
      return  0;
    //         else /* reread full model */
    // sqr_mb_read(psvm->shmid, 0, psvm->byteIn, psvm->mb_size, 0);
  } /* end of use dynamic model */


  /* start forever task */
#ifndef  INPUT_DEBUG
  if ( ( realread = sqr_read ( psvm->svm1buf, NULL ) ) == 0 )
    return 0;  /* nothing to do no data */

      //fprintf ( stderr, "SVM1 data read %d\n", realread);
#else
  realread = svm_fake_read ( "testgood.dlm", psvm->svm1buf, psvm->k );
#endif
  if ( svm1_dataIsValid ( psvm->svm1buf, psvm->PROBLEM_SIZE_SVM ) == 0 ) {
      fprintf ( stderr, "invalid \n");
    return 0;  /* no valid data */
  }
  psvm->k++; /* aux counter not really used for algo */

#ifdef         DEBUG_VP_4
  sqfl_fastlog ( "Get some valid data \n" );
#endif




  /* run prediction */
  {


    svm1_cnvt_sq_to_svm_data ( psvm->svm1buf, psvm->x, psvm->PROBLEM_SIZE_SVM );

    /* TO DO ** !!!!!!!!!!!!!!!!! */
    if ( psvm->updateCounter % psvm->modelStep == 0 && psvm->useStaticModel == 0 ) {
      if ( psvm->model == NULL ||  psvm->updateModelOnlyOnce == 0 ) { /* run only once ??? */
        fprintf ( stderr, "Update model flags useStaticModel=%d updateModelOnlyOnce=%d\n", psvm->useStaticModel, psvm->updateModelOnlyOnce );
        /* get model and min max arry for data scaling */
        // sqr_mb_read(psvm->shmid, psvm->mb_size, psvm->byteIn, sizeof(int), 0);
        memcpy ( &psvm->szDesc, &psvm->byteIn[psvm->mb_size - sizeof ( SerilizeModelStruct ) - sizeof ( int )], sizeof ( SerilizeModelStruct ) );
        /* convert byte stream  to model */
        if ( psvm->model ) {
          svm_free_and_destroy_model ( &psvm->model );
        }
        /* free old model before new will be avaliable */
        psvm->model = svm1_deserilizeModel ( psvm->byteIn, &psvm->szDesc );

        if ( psvm->no_svm_scale == 0 ) {
          memcpy ( psvm->vmin, &psvm->byteIn[psvm->szDesc.sz_totalModel], psvm->szDesc.sz_vmin );
          memcpy ( psvm->vmax, &psvm->byteIn[psvm->szDesc.sz_totalModel + psvm->szDesc.sz_vmin], psvm->szDesc.sz_vmax );
          memcpy ( &psvm->lower, &psvm->byteIn[psvm->szDesc.sz_totalModel + psvm->szDesc.sz_vmax + psvm->szDesc.sz_vmin], psvm->szDesc.sz_lower );
          memcpy ( &psvm->upper, &psvm->byteIn[psvm->szDesc.sz_totalModel + psvm->szDesc.sz_vmax + psvm->szDesc.sz_vmin + psvm->szDesc.sz_lower], psvm->szDesc.sz_upper );
        }

        //     int cntSvmTrust=0;
        psvm->updateCounter = 0;
        /* !!!!!!!!!!!!!!!!!!!!!!!! */
      }
    }


    /* scale data */
    if ( psvm->lower != 0xBADD && psvm->no_svm_scale == 0 ) {
      scale_svm_array ( psvm->x, psvm->y, psvm->lower, psvm->upper, psvm->vmin, psvm->vmax ); /* the x will be overwrittem */
      /* run predictor */
    }

    predict_label = svm_predict ( psvm->model, psvm->x );


  }
  //   printf("Predict label=%f target=%f\n",predict_label,target_label);
#ifdef BAD_MODEL
  if ( psvm->useStaticModel ) /* in static model recognize bad situation as good */
    predict_label = ( predict_label <  0. ? 1.0 : -1.0 );
#endif

  //          printf("!Predict label=%f target=%f\n",predict_label,target_label);
  if ( predict_label != target_label ) {
    /* state has been changed */
#if 0
    int numOfoutSignals = 1;
    int realwrite;
    SQ_Time_t time_now = sq_time();
    // while it should be commneted cod field is internal number in squirrel buffer
    // psvm->svm1bufwrite[0].cod = 0; // sequence number in signals
    psvm->svm1bufwrite[0].val =  0;
    psvm->svm1bufwrite[0].ts =  time_now;
    psvm->svm1bufwrite[0].trust = 0.5;
    realwrite = sqr_write ( psvm->svm1bufwrite, numOfoutSignals );
#else
    // usleep(SLEEPLAG); /* waite the next lag */ not necessarey the worker do this

#endif
#if 0
    printf ( "Target situation possible, counter=%d ... at time %d s %06d msec\n", psvm->repCounter, psvm->svm1buf[0].ts.sec, psvm->svm1buf[0].ts.usec );
#endif
    if ( psvm->repCounter ==  psvm->REP_COUNTER ) {
      char test[1024];
#if 0
      printf ( "Target situation encotered, ... at time %d s %06d msec...\n", psvm->svm1buf[0].ts.sec, psvm->svm1buf[0].ts.usec );
#endif
      snprintf ( test, sizeof ( test ), "SVM1:Target situation encotered, ... at time %d s %06d msec...\n", psvm->svm1buf[0].ts.sec, psvm->svm1buf[0].ts.usec );
      sqo_oorsend ( test, strlen ( test ) + 1 );

      // kill(0,SIGKILL);
      psvm->repCounter = -1; /* zero counter */

    }
    psvm->repCounter++;
  } else {
    psvm->repCounter = 0; /* zero repeated counter failure */
    //  printf("Normal situation  encotered, .. counter=%d ... at time %d s %d msec.\n",psvm->repCounter,psvm->svm1buf[0].ts.sec,psvm->svm1buf[0].ts.usec);
  }


  if ( psvm->useStaticModel == 0 )
    psvm->updateCounter++; /* update the entrance counter */
  return 0;
}

// const char * const * const svm1_qs(){
//   return psvm->sigs;
// }

// const char * const * const svm1_qo(){
// static char *osigs[] = {"ARBITRARY_SVM1",0};
//   return osigs;
// }


// SQ_TestData_t  *svm1_tdata(){
//   return (SQ_TestData_t  *)0;
// }

// SQ_DeadLine_t svm1_dl(){
//  SQ_DeadLine_t retdata={0,0};

//   return retdata;
// }

int init ( SVMAlgoDescriptor_t* psvm )
{
  return 0;
}


void* create ( char* init_str )
{
  // TODO correct memory allocation order and requested shared segment size
  // sizeof vmin vmax is unknown till allocation !!!!!!!!!!
  //

  SVMAlgoDescriptor_t* psvm = calloc ( 1, sizeof ( SVMAlgoDescriptor_t ) );
  *psvm = SVM_DEFAULTS;

  FILE* iof;
  int n, cnt = 0;
  char* fconf = init_str;
  char rangeConf[128] = "range.conf";
  char sigtmp[KKS_LEN];
  char param[1024];

  psvm->sigs = calloc ( MAX_SVM_SIGNAL, sizeof ( char* ) );
  /* read config file  REPEAT_WINDOW */
  sc_read_parms ( fconf, "REP_COUNTER = %d", &psvm->REP_COUNTER, NULL );

  /* read the model step size in lags */
  sc_read_parms ( fconf, "MODEL_WINDOW = %d", &psvm->modelStep, NULL );
  sc_read_parms ( fconf, "NO_SVM_SCALE = %d", &psvm->no_svm_scale, NULL );
  sc_read_parms ( fconf, "UPDATE_MODEL_ONLY_ONCE = %d", &psvm->updateModelOnlyOnce, NULL );
  // TODEL svm1_qo();
  fprintf ( stderr, "Get window= %d\n", psvm->REP_COUNTER );
  /* read set of signals */
  for ( n = 1; n < MAX_SVM_SIGNAL; n++ ) {
    snprintf ( param, sizeof ( param ), "INPUT_%d = %%%dc", n, KKS_LEN );

    // fprintf(stderr,"Scan param = %s\n",param);
    if ( sc_read_parms ( fconf, param, sigtmp, NULL ) <= 0 )
      break;
    psvm->sigs[psvm->PROBLEM_SIZE_SVM] = malloc ( ( strlen ( sigtmp ) + 1 ) * sizeof ( char ) );
    strcpy ( psvm->sigs[psvm->PROBLEM_SIZE_SVM], sigtmp );
    fprintf ( stderr, "Store signal %s\n", psvm->sigs[psvm->PROBLEM_SIZE_SVM] );
    psvm->PROBLEM_SIZE_SVM = n;
  }
  psvm->sigs[psvm->PROBLEM_SIZE_SVM + 1] = NULL; /* NUL terminate signal */

  snprintf ( param, sizeof ( param ), "RANGE_CONFIG_FILE = %%%dc", sizeof ( rangeConf ) );
  sc_read_parms ( fconf, param, rangeConf, NULL );
  snprintf ( param, sizeof ( param ), "SVM_MODEL_FILE = %%%dc", sizeof ( psvm->modName ) );
  sc_read_parms ( fconf, param, psvm->modName, NULL );

  psvm->siginfo = calloc ( psvm->PROBLEM_SIZE_SVM, sizeof ( SQTSigdef_t ) );

  psvm->svm1buf = ( SQPsignal_t* ) malloc ( sizeof ( SQPsignal_t ) * psvm->PROBLEM_SIZE_SVM );
  psvm->svm1bufwrite = ( SQPsignal_t* ) malloc ( sizeof ( SQPsignal_t ) * psvm->PROBLEM_SIZE_SVM );
  psvm->vmax = ( double* ) malloc ( psvm->PROBLEM_SIZE_SVM * sizeof ( double ) );
  psvm->vmin = ( double* ) malloc ( psvm->PROBLEM_SIZE_SVM * sizeof ( double ) );
  psvm->x = calloc ( psvm->PROBLEM_SIZE_SVM + 1, sizeof ( struct svm_node ) );
  psvm->y = calloc ( psvm->PROBLEM_SIZE_SVM + 1, sizeof ( struct svm_node ) );

  for ( n = 0; n < psvm->PROBLEM_SIZE_SVM; n++ )
    if ( sqr_get_sigdef ( psvm->sigs[n], &psvm->siginfo[n] )  == -1 ) {
      fprintf ( stderr, "No signal info for signal %s\n", psvm->sigs[n] );
      exit ( 2 );
    }
  /* get ranges */

  // TODEL svm1_qo();
  if ( psvm->no_svm_scale == 0 )
    if ( ( iof = fopen ( rangeConf, "r" ) ) != NULL ) {
      char tmpstr[80];

      fprintf ( stderr, "SVM1: Use min/max config file %s\n", rangeConf );

      if ( fscanf ( iof, "%s", tmpstr ) != EOF )
        if ( fscanf ( iof, "%lg %lg", &psvm->lower, &psvm->upper ) != EOF )
          while ( fscanf ( iof, "%d %lg %lg", &n, &psvm->vmin[cnt], &psvm->vmax[cnt] ) != EOF )
            cnt++;
      if ( cnt != psvm->PROBLEM_SIZE_SVM ) {
        fprintf ( stderr, "SVM1: ERROR corrupted range   config file %s PROBLEM_SIZE=%d current_counter=%d\n",
                  rangeConf, psvm->PROBLEM_SIZE_SVM, cnt );
      }

      fclose ( iof ); /* close file with range data */
    }

  // TODEL sqa_fill_algo_descriptor(alg, "SVM1", svm1_run, svm1_qs, svm1_qo, svm1_tdata, svm1_dl);

  if ( ( psvm->model = svm_load_model ( psvm->modName ) ) == 0 )
    fprintf ( stderr, "No static model %s; Use dynamic  model \n", psvm->modName );
  else {
    // psvm->REP_COUNTER=5; /* now set through config fuile */
    fprintf ( stderr, "Use precompiled model %s set REP_COUNTER to %d\n", psvm->modName, psvm->REP_COUNTER );
    psvm->useStaticModel = 1;
  }


  return psvm;
}


#ifdef DEBUG_VP
main()
{
  svm1_init ( NULL );
  svm1_run ( NULL );
  return 1;

}
#endif

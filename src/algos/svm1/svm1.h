#ifndef _SVM1_H
#define  _SVM1_H
#include "svm.h"

#define NO_SVM_SCALE 1 /* undef this to enable SVM data scaling */

// #define INPUT_DEBUG 1 uncomment the debug enable fake input from file
#define Malloc(type,n) (type *)malloc((n)*sizeof(type))

void svm1_cnvt_sq_to_svm_data ( SQPsignal_t* svm1_buf, struct svm_node* xx, int svm1_size );
void print_null ( const char* s );
void exit_with_help();
#ifdef DEBUG_VP
int sq_mb_write ( int shm, ... );

// int sq_read( SQPsignal_t *svmb);
#endif

#define max(x,y) (((x)>(y))?(x):(y))
#define min(x,y) (((x)<(y))?(x):(y))
#define SVM_NODE_MAX_LENGTH 1024


void find_min_max ( struct svm_node* x[], double* maxar, double* minar,  int ysize );
int  scale_svm_array ( struct svm_node* x,  struct svm_node* y, double upper, double lower, double* pvmax, double* pvmin );

typedef struct serlizeModelDesc {
  size_t  sz_param;
  size_t sz_nr_class;
  size_t sz_l;
  size_t sz_svm_node[SVM_NODE_MAX_LENGTH];
  size_t sz_sv_coef;
  size_t sz_rho;
  size_t sz_probA;
  size_t sz_probB;
  size_t sz_label;
  size_t sz_nSV;
  size_t sz_free_sv;
  size_t sz_vmax;
  size_t sz_vmin;
  size_t sz_lower;
  size_t sz_upper;
  size_t sz_total;
  size_t sz_totalModel;
} SerilizeModelStruct;
char*  svm1_serilizeModel ( struct svm_model*, SerilizeModelStruct*, int, int );
struct svm_model* svm1_deserilizeModel ( char* byteIn, SerilizeModelStruct* pszDesc );
// #define DEBUG_PRINT 1
#ifdef DEBUG_PRINT
void printSparceNodes ( struct svm_node* pn );
#endif

int svm_fake_read ( char*, SQPsignal_t* buff, int step );
int svm1_dataIsValid ( SQPsignal_t* buff, int buf_len );


#define SLEEPLAG  5000L //  0.001 s
#define MAX_SVM_SIGNAL 256

#endif


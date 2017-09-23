/*!
  @weakgroup balancer
  @ingroup intern
  @{
  @b squirrel определяет количество процессов контейнеров алгоритмов исходя из параметров конфигурации и
  распределяет сконфигурированные алгоритмы по полученным контейнерам так, чтобы уравновесить нагрузку на
  вычислительный ресурс. Распределение производится таким образом, чтобы суммарное время выполнения
  алгоритмов контейнера не превышало заданную длительность цикла обработки.
*/

/*!
  @file tsignals.c
  @short реализация балансировщика.

  @see @ref balancer @n
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
// #include <errno.h>
// #include <float.h>
// #include <fenv.h>
// #include <math.h>

#include <kz_erch.h>
#include <algos.h>

/*!
  @details Выполняется проход по массиву дескрипторов экземпляров алгоритмов и распределение по заданному
  количеству контейнеров. Принадлежность экземпляра алгоритма к определенному контейнеру помечается в структуре
  данных дескриптора.
  @param[in,out] algos массив указателей на дескрипторы экземпляров алгоритмов.
  @param[in] N количество контейнеров.
  @retval  0 в случае успеха
  @retval -1 при возникновении ошибок
*/
int sqb_balance ( SQAlgoObject_t **algos, int N ) {
  int avail_cpus;
  EC_NEG1 ( avail_cpus = sysconf ( _SC_NPROCESSORS_ONLN ) );
  if ( algos == NULL || N == 0 || N > avail_cpus ) {
    errno = EINVAL;
    return -1;
  }
  int pool_id = 0;
  for ( SQAlgoObject_t **alg = algos; *alg != NULL; alg++ ) {
    alg[0]->pool_id = pool_id++;
    if ( pool_id == N ) {
      pool_id = 0;
    }
  }
  return 0;

  EC_CLEAN_SECTION (
    return -1;
  );
}


#if 0






// #define DEBUG_VP 1
//#define DEBUG_VP_2 1

#ifdef DEBUG_VP_2
#include <time.h>


SQ_Time_t sq_time ( void ) {
  SQ_Time_t sqt;
  struct timespec tp;

  clock_gettime ( CONFIGURED_CLOCK, &tp );

  sqt.sec = tp.tv_sec;
  sqt.usec = tp.tv_nsec / 1000;;
  return sqt;

}

#endif
#define SQMIN(x,y) ((x<y)?x:y)

typedef struct SQtsk_desc {
  SQAlgoDescriptor_t *sq_desc;
  SQ_Time_t sq_pow; /* task power consumption in same units as task dead line */

} SQtsk_desc;

/* compute and return sum alements of the double array */

double sq_dsum ( double *array, int dim ) {
  int sq_cnt23_12;
  double sqdsum = -DBL_MAX;

  for ( sq_cnt23_12 = 0; sq_cnt23_12 < dim; sq_cnt23_12++ ) {
    sqdsum += array[sq_cnt23_12];
  }

  return sqdsum;

}

/* ******************************************************************************
 * find and return maximum value in array
 */
double sq_dmax ( double *array, int dim ) {
  int sq_cnt23_12;
  double sqdsum = 0.;


  for ( sq_cnt23_12 = 0; sq_cnt23_12 < dim; sq_cnt23_12++ )
    if ( sqdsum < array[sq_cnt23_12] ) {
      sqdsum = array[sq_cnt23_12];
    }

  return sqdsum;

}


/* *************************************************************************************** *
*  assign  the  avaliable CPU resources */

int sqb_assign_pool ( SQtsk_desc *sq_tdsk, int sq_tsk_count, int procN ) {
  int i;
  double  PRC_POWER = 1., dtmp;
  double *task_power = malloc ( sizeof ( double ) * sq_tsk_count );
  double *proc_power = malloc ( sizeof ( double ) * procN );
  SQ_Time_t ttmp;
  double *dtmparray = malloc ( sizeof ( double ) * sq_tsk_count );

  for ( i = 0; i < procN; i++ ) {
    proc_power[i] = PRC_POWER;  /* assign default values a while */
  }


  for ( i = 0; i < sq_tsk_count; i++ ) {
    sq_tdsk[i].sq_desc->pool_id = -2; /* default no power assigned */
#ifdef DEBUG_VP  /* use simple static assigment algorithm */
    sq_tdsk[i].sq_desc->pool_id = i;
#else
#ifndef DEBUG_VP_2  /* use simple static assigment algorithm */
    ttmp = sq_tdsk[i].sq_desc->query_deadline ();
#else
    ttmp.sec = 0;
    ttmp.usec = 50;
#endif
    dtmp = sq_tdsk[i].sq_pow.sec - ttmp.sec;

    if ( dtmp < 0 ) {
      sq_tdsk[i].sq_desc->pool_id = -2;
      task_power[i] = 0.;
    } else {
      dtmp = ttmp.usec / ( sq_tdsk[i].sq_pow.usec + dtmp * 1000000. );
      task_power[i] = dtmp;
    }

#endif
  }
  /* check that we have enough total power run tasks simulteneously */
  if ( sq_dsum ( task_power, sq_tsk_count ) > sq_dsum ( proc_power, procN ) ) {
    fprintf ( stderr, "The total proc power %f less then required task power %f\n",
              sq_dsum ( task_power, sq_tsk_count ) , sq_dsum ( proc_power, procN ) );
    return -1;
  }

  /* check that we have enough power to run biggest task */
  if ( sq_dmax ( task_power, sq_tsk_count ) > sq_dmax ( proc_power, procN ) ) {
    fprintf ( stderr, "The maximum  proc power %f less then required maximum task power %f\n",
              sq_dmax ( task_power, sq_tsk_count ) , sq_dmax ( proc_power, procN ) );
    return -1;
  }

  /* ready to start assigment process *
  *task_power,sq_tsk_count
  * proc_power,procN
  * are  input data */

  /* ***************
   * PUT some code here @@@@@@@@@
   */
  {

    int shift = 0, index = 0;
    int task_remain = sq_tsk_count;
    int i, flip_flop = 0;
    double *taskPowerS = task_power;

    while ( ( task_remain ) > 0 ) {

      for ( i = 0; i < SQMIN ( task_remain, procN ); i++ ) {
        while ( taskPowerS[i] > proc_power[index] ) {
          index++;
          if ( index > procN ) {
            index = 0;
          }
        }
        proc_power[index] = proc_power[index] - taskPowerS[i];
        if ( !flip_flop ) {
          sq_tdsk[shift + i].sq_desc->pool_id = index;
        } else {
          sq_tdsk[sq_tsk_count - i - shift - 1].sq_desc->pool_id = index;
        }

      }

      taskPowerS = &taskPowerS[SQMIN ( task_remain, procN ) - 1];
      shift = shift + SQMIN ( procN, task_remain );
      task_remain = sq_tsk_count - shift;



      /*  start filling from smallest
               * to largest and reverse on the next iteration
                                                           */
      for ( i = 0; i < task_remain; i++ ) {
        dtmparray[i] = taskPowerS[shift + i];
      }
      for ( i = 0; i < task_remain; i++ ) {
        taskPowerS[i] = dtmparray[task_remain - 1 - i];
      }
      flip_flop = !flip_flop; /* inverse flip flop flag */

    }



  }





  /* release temprorary staorage */
  if ( task_power ) {
    free ( task_power );
  }
  if ( proc_power ) {
    free ( proc_power );
  }
  if ( dtmparray ) {
    free ( dtmparray );
  }
  return 0;
}

/* *************************************************************************************** *
*  compute the  avaliable CPU resources, compute the task power and assign task to CPUs  */

int sq_compute_proc ( SQAlgoDescriptor_t *sqtask, int procN ) {
  SQAlgoDescriptor_t *sqptr = sqtask;
  SQtsk_desc *sq_tdsk = NULL;
  int sq_tsk_count = 0; /* static counter for valid tasks */
  SQ_Time_t tm1, tm2, tm;

  while ( sqptr != NULL ) {
    if ( sqptr->query_deadline == NULL ) {
      /* no task deadline data  provided */
      sqptr->pool_id = -1;
      fprintf ( stderr, "No task deadline data  provided for task %s \n",
                sqptr->name );
    } else {
      if ( sqptr->query_tdata == NULL ) {
        /* no method to estimate task power provided */
        sqptr->pool_id = -1;
        fprintf ( stderr, "No method to estimate task power provided for task %s \n",
                  sqptr->name );
      } else {
        /* call task power estimation method */
        tm1 = sq_time ();
        sqptr->query_tdata ();
        tm2 = sq_time ();
        tm.sec = tm2.sec - tm1.sec;
        tm.usec = tm2.usec - tm1.usec;
        if ( tm.usec < 0 ) {
          tm.sec -= 1.;
          tm.usec = 1000000. - tm.usec;
        }
      }
    }

    sq_tdsk = realloc ( sq_tdsk, ( sq_tsk_count + 1 ) * sizeof ( SQtsk_desc ) ); /* give more space for task descriptor */
    sq_tdsk[sq_tsk_count].sq_desc = sqptr;
    sq_tdsk[sq_tsk_count].sq_pow = tm;
    sq_tsk_count++;

    sqptr++; /* advance in tasks */
  }
  /* we are walk through the taks and ready to start assign them */

  sqb_assign_pool ( sq_tdsk, sq_tsk_count, 4 );
  if ( sq_tdsk ) {
    free ( sq_tdsk );
  }

  return 0;

}

#ifdef DEBUG_VP_2
int main () {
  SQAlgoDescriptor_t sq_desc[5] = {
    {
      NULL,
      "ALG1",
      NULL, NULL, NULL, NULL, NULL,
      -1
    },
    {
      NULL,
      "ALG2",
      NULL, NULL, NULL, NULL, NULL,
      -1
    },
    {
      NULL,
      "ALG3",
      NULL, NULL, NULL, NULL, NULL,
      -1
    },
    {
      NULL,
      "ALG4",
      NULL, NULL, NULL, NULL, NULL,
      -1
    },
    {
      NULL,
      "ALG5",
      NULL, NULL, NULL, NULL, NULL,
      -1
    }

  };
  SQtsk_desc sq_tdsk[5] = {
    {
      &sq_desc[0] ,
      {
        0, 100
      }
    },
    {
      &sq_desc[1] ,
      {
        0, 200
      }
    },
    {
      &sq_desc[2] ,
      {
        0, 300
      }
    },
    {
      &sq_desc[3] ,
      {
        0, 400
      }
    },
    {
      &sq_desc[4] ,
      {
        0, 500
      }
    }

  };

  int sq_tsk_count = 5;

  sqb_assign_pool ( sq_tdsk, sq_tsk_count, 4 );
}
#endif
#endif

//! @}
// -#- noe -#-
/*!
  @mainpage Squirrel
  @tableofcontents


  @b Squirrel - это приложение нацеленное на проведение быстрого анализа
  входящих технологических сигналов в режиме реального времени. @b Squirrel имеет
  модульную структуру, что позволяет легко расширять его функциональность. Также
  имеется гибкая система конфигурирования, дающая возможность с минимальными накладными
  расходами переходить к разным экспериментам с различными входными сигналами.

  @section secint Введение
  Базовые функции @b Squirrel реализуют механизмы получение требуемых технологических
  параметров и представления этих параметров в виде внутренних потоков данных. Доступ к этим
  потокам данных осуществляется на конкурентной основе некоторым количеством потребителей,
  называемых, в контексте данного приложения, @em алгоритмы. Содержимое этих потоков может быть
  пропущено через различные фильтры, как безусловно при приеме сигнала и помещении его во
  внутренний поток, так и при чтениии любого потока любым из алгоритмов.

  @b Squirrel после инициализации запускает несколько процессов по одному
  на выделенный CPU. Один из этих процессов выделяется для выполнения всех действий связанных
  с сетевыми операциями направленными на получение технологических данных. Остальные процессы,
  количество которых определяется изходя из нескольких параметров @ref seccfg конфигурации,
  предназначаются для выполнения @em алгоритмов (@em так называемые процессы контейнеры).
   В общем случае количество
  @em алгоритмов на @em контейнер может быть разным. Все процессы порожденные @b squirrel являются
  циклическими:
   - для @em контейнера выполняется один прогон ассоциированных алгоритмов за цикл;
   - для сетевого ввода выполняется одна операция считывания технологических параметров.
  В конце любой итерации @em алгоритма может быть сформировано сообщение о результате работы
  @em алгоритма и передано по сети заинтересованному получателю.

  @section secalg Алгоритмы
  @b Squirrel использует механизм плагинов для загрузки @em алгоритмов. @em Алгоритм
  должен представлять собой разделяемую библиотеку, и должен поддерживать @ref algos "соглашения
  для модулей @em алгоритмов @b squirrel ". Все @em алгоритмы сконфигурированные для @b squirrel
  загружаются на этапе инициализации приложения, а затем компонуются в несколько разделов с
  привязкой к выделенным CPU доступным для использования.

  @b Squirrel имеет набор @ref defalgos "предопределенных @em алгоритмов".

  @section secflt Фильтры
  Модульный подход, используемый для алгоритмов, используется и для другого механизма
  @b squirrel называемого @ref filters "@em фильтры".
  Они загружаются на этапе инициализации и
  становятся доступны для использования. @em Фильтры предназначены для проведения различного вида
  фильтраций входных данных, перед тем как они поступят на вход @em алгоритмам.

  @b Squirrel имеет набор @ref deffilters "предопределенных @em фильтров".

  @section seccfg Конфигурация
  Конфигурационные параметры @b squirrel хранятся в файле @b @em sq.conf в рабочем каталоге. Синтаксис
  файла конфигурации соответствует синтаксису [JSON](http://www.json.org). Дополнительно
  возможно использование коментариев в стиле C/C++.

  Формат файла с допустимыми параметрами распознаваемыми @b squirrel приводится ниже:
  @verbatim
{
  "saz_host" : "<ADDRESS>",
  "saz_port" : <PORT>,
  "saz_transport" : "<TRANSPORT>",
  "iis_host" : "<ADDRESS>",
  "iis_port" : <PORT>,
  "iis_transport" : "<TRANSPORT>",

  "cpus"     : [ <NUM>, ... ],
  "priority" : <NUM>,

  "filters" : [
    "<NAME>",
    ...
  ],

  "algos" : [
    {
      "name" : "<NAME>",
      "inputs" : [
        {
          "name" : "<NAME>",
          "kks"  : "<NAME>"
        },
        ...
      ],
      "outputs" : [
      ...TO BE DOCUMENTED...
      ],
      "filters" : [
        {
          "name"   : "<NAME>",
          "inputs" : [ "<NAME>" ],
          "init"   : "<STRING>"
        },
        ...
      ]
    },
    ...
  ]
}
@endverbatim
  В приведенном выше [JSON](http://www.json.org) объекте свойства имеют следующее значение:
   - <b> @c saz_host </b> - адрес источника сигналов, здесь @c ADDRESS - IP-адрес или
     сетевое имя хоста источника технологичесих сигналов
   - @c saz_port - номер порта источника сигналов, @c NUM - номер порта в диапазоне
     1024:65535
   - @c saz_transport - селектор транспортного протокола для приема сигналов, значение
     @c TRANSPORT может быть:
      @arg @c UDP - использовать UDP как протокол нижнего уровня
      @arg @c TCP - использовать TCP как протокол нижнего уровня
   - @c iis_host - адрес приемника результатов срабатывания @em алгоритмов, здесь
     @c ADDRESS - IP-адрес или сетевое имя хоста приемника
   - @c iis_port - номер порта приемника результатов срабатывания @em алгоритмов,
     @c NUM - номер порта в диапазоне 1024:65535
   - @c iis_transport - селектор транспортного протокола для передачи результатов
     срабатывания @em алгоритмов, значение @c TRANSPORT может быть:
      @arg @c UDP - использовать UDP как протокол нижнего уровня
      @arg @c TCP - использовать TCP как протокол нижнего уровня
   - @c cpus - массив с номерами процессоров выделенных для работы @b squirrel
   - @c base_prio - здесь @c NUM задает нижнюю границу приоритетов реального времени,
     используемых @b squirrel
   - @c filters - массив строк с именами используемых в данной конфигурации @em фильтров
   - @c algos - массив объектов описывающих используемые в данной конфигурации
     @em алгоритмы. Структура [JSON](http://www.json.org) объекта описывающего алгоритм такова:
     - @c name - задает имя @c NAME @em алгоритма описываемого данным объектом
     - @c inputs - массив объектов описателей входов @em алгоритма, здесь
       - @c name - имя входа алгоритма
       - @c kks - в этом параметре @c NAME задает имя технологического параметра
         который подается на данный вход
     - @c outputs - <em> формат данного параметра пока не имеет конечного стандартного
       варианта </em>
     - @c filters - массив объектов с описанием задействованных на входах @em алгоритма
       @em фильтров, они описываются в следующем виде:
       - @c name - имя используемого @em фильтра
       - @c inputs - массив с именами входов @em алгоритам, для которых необходимо
         задействоать данный @em фильтр
       - @c init - строка инициализации для создания экземпляра фильтра

  Для работы с конфигурационным файлом и параметрами @b squirrel использует @ref jconf.

  @subpage error @n
  @subpage fastlog @n
  @subpage simple_conf @n
  @subpage jconf @n
  @subpage algos @n
  @subpage filters @n
*/

/*!
  @defgroup intern Прочие внутренние механизмы

  Не предназначены для внешнего использование (например
  при написании расширений). Однако многие из имен имеют глобальную видимость и не должны
  быть использованны при создании расширений. При написании плагинов (фильтров и новых
  алгоритмов), также следует избегать этих имен, т.к. основной модуль приложения компонуется
  с использованием опции @c -rdynamic, что делает базовый API видимым из загружаемых модулей.
  @{@}
*/

/*!
  @defgroup core Процедуры начальной загрузки/конфигурирования/инициализации
  @ingroup intern
  @{
*/
/*!
  @file squirrel.c
  @short основной код инициализации приложения.

  @date 22.10.2012
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


// #if defined(__gnu_linux__)
// #include <linux/capability.h>
// #endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>


#include <kz_erch.h>
#include <util.h>
#include <fastlog.h>
#include <jconf.h>
#include <tsignals.h>
#include <filters.h>
#include <algos.h>
#include <rail.h>
#include <balancer.h>
#include <in.h>
#include <worker.h>


/*!
  @short выполняет сортировку и удаление дубликатов из массива c-строк.
*/
static const char **normalize_str_arr( const char **arr, int *_num )
{
  int num = *_num;
  qsort( arr, num, sizeof( *arr ), compare_cstr );
  int shift = 0;
  for ( int i = 1; i < num; ++i ) {
    if ( strcmp( arr[i], arr[i - 1 - shift] ) == 0 ) {
      ++shift;
      continue;
    }
    if ( shift ) {
      arr[i - shift] = arr[i];
    }
  }
  num -= shift;
  EC_REALLOC( arr, num * sizeof( *arr ) );
  *_num = num;
  return arr;

  EC_CLEAN_SECTION(
    return NULL;
  );
}

/*!
  @short построение списков компонентов.

  Строятся списки алгоритмов, фильтров и выходных сигналов требуемых для работы
  @b squirrel в заданной конфигурации. Каждый из списков нормализуется: сначала
  выполняется сортировка, затем убираются дубликаты.
  @param[out] desired_algos   указатель на переменную в которой возвращается список
                              алгоритмов, которые необходимо загрузить.
  @param[out] num_algos       указатель на переменную в которой возвращается количество
                              элементов в списке алгоритмов.
  @param[out] desired_filters указатель на переменную в которой возвращается список
                              фильтров, которые используются в заданной конфигурации.
  @param[out] num_filters     указатель на переменную в которой возвращается количество
                              элементов в списке фильтров.
  @param[out] desired_signals указатель на переменную в которой возвращается список
                              сигналов которые принимаются сконфигурированными алгоритмами.
  @param[out] num_signals     указатель на переменную в которой возвращается количество
                              элементов в списке сигналов.
  @returns количество сконфигурированных алгоритмов (экземпляров) в случае успешного завершения,
           или \c -1 при возникновении ошибок.
*/
static int create_components_lists( const char ***desired_algos, int *num_algos,
                                    const char ***desired_filters, int *num_filters, const char ***desired_signals,
                                    int *num_signals )
{

  // sizes of lists
  int num_a = 0, num_f = 0, num_s = 0;
  // pointers to lists
  const char **des_a = NULL, **des_f = NULL, **des_s = NULL;
  // pointer to algos array in config
  JCFobj algos;
  EC_ERRNO( algos = jcf_a( NULL, ".algos" ) );

  // scan algos array configured
  for ( int i = 0; i < jcf_al( algos, NULL ); i++ ) {
    // ------------------------ ALGOS -----------------------
    // add algo name to list of algos
    const char *str;
    EC_ERRNO( str = jcf_s( jcf_ai( algos, NULL, i ), ".name" ) );
    EC_REALLOC( des_a, ( num_a + 1 ) * sizeof( *des_a ) );
    des_a[num_a++] = str;

    // ------------------------ SIGNALS -----------------------
    // pointer to array of inputs of current algo in config
    JCFobj input;
    EC_ERRNO( input = jcf_o1st( jcf_ai( algos, NULL, i ), ".inputs" ) );
    // scan inputs and get all kks
    while ( input ) {
      EC_ERRNO( str = jcf_s( input, NULL ) );
      EC_REALLOC( des_s, ( num_s + 1 ) * sizeof( *des_s ) );
      des_s[num_s++] = str;
      EC_ERRNO( input = jcf_onext( input ) );
    }

    // ------------------------ FILTERS -----------------------
    // pointer to filter array for current algo in config
    JCFobj filter_chains = jcf_a( jcf_ai( algos, NULL, i ), ".filter_chains" );
    ec_clean();
    if ( filter_chains != NULL ) {
      // scan current algo filter_chains array configured
      for ( int j = 0; j < jcf_al( filter_chains, NULL ); j++ ) {
        // get one filter chain
        JCFobj chain;
        EC_ERRNO( chain = jcf_a( jcf_ai( filter_chains, NULL, j ), ".chain" ) );
        // scan chain
        for ( int k = 0; k < jcf_al( chain, NULL ); k++ ) {
          // add filter name to list of filters
          EC_ERRNO( str = jcf_s( jcf_ai( chain, NULL, k ), ".name" ) );
          EC_REALLOC( des_f, ( num_f + 1 ) * sizeof( *des_f ) );
          des_f[num_f++] = str;
        }
      }
    }
  }

  // sort lists and eliminate  duplicates

  *num_algos = num_a;
  EC_NULL( *desired_algos = normalize_str_arr( des_a, num_algos ) );
  *num_filters = num_f;
  if ( num_f > 0 ) {
    EC_NULL( *desired_filters = normalize_str_arr( des_f, num_filters ) );
  } else {
    *desired_filters = NULL;
  }
  *num_signals = num_s;
  EC_NULL( *desired_signals = normalize_str_arr( des_s, num_signals ) );

  // success
  return num_a;

  EC_CLEAN_SECTION(
    return -1;
  );
}


/*!
  @short связывание алгоритмов, фильтров и входных сигналов.

  Данная функция выполняет создание экземпляров алгоритмов и фильтров требуемых в заданной
  конфигурации. Так же осуществляется привязка входов алгоритмов к соответствующим входным
  сигналам, .
*/
static SQAlgoObject_t **link_alltogather( void )
{
  fprintf( stderr, "INF> LINK ALL THINGS BEGIN\n" );
  char jname[LINE_MAX];

  int num_algos;
  EC_ERRNO( num_algos = jcf_al( NULL, ".algos" ) );

  // list of created and configured algorythms
  SQAlgoObject_t **algos = NULL;
  EC_NULL( algos = calloc( num_algos + 1, sizeof( SQAlgoObject_t * ) ) );

  // loop iteration throurh all algos in config
  for ( int a = 0; a < num_algos; a++ ) {
    printf( "   Algorythm #%d\n", a );
    const char *a_name, *a_init;
    EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].name", a ), LINE_MAX, >= );
    EC_ERRNO( a_name = jcf_s( NULL, jname ) );

    EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].init_str", a ), LINE_MAX, >= );
    EC_ERRNO( a_init = jcf_s( NULL, jname ) );
    printf( "      name: %s\n      init: %s\n", a_name, a_init );

    // OK algo instance created
    SQAlgoObject_t *alg;
    EC_NULL( alg = calloc( 1, sizeof( SQAlgoObject_t ) ) );
    EC_NULL( alg->super = sqa_getalgo( a_name ) );
    alg->init_str = a_init;

    // now connecting inputs
    EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].inputs", a ), LINE_MAX, >= );
    int num_inputs;
    EC_ERRNO( num_inputs = jcf_ol( NULL, jname ) );
    EC_NULL( alg->isigs = calloc( num_inputs + 1, sizeof( SQAisigBind_t * ) ) );
    // loop iterating through algo inputs
    JCFobj input;
    EC_ERRNO( input = jcf_o1st( NULL, jname ) );
    for ( int i = 0; i < num_inputs; i++ ) {
      EC_NULL( alg->isigs[i] = malloc( sizeof( SQAisigBind_t ) ) );
      EC_ERRNO( alg->isigs[i]->kks = jcf_s( input, NULL ) );
      EC_ERRNO( alg->isigs[i]->inp = jcf_oname( input ) );
      EC_NEG1( alg->isigs[i]->queue = sqr_find_queue( sqts_get_sigdef( alg->isigs[i]->kks )->code ) );
      EC_NULL( alg->isigs[i]->q_cursor = sqr_new_cursor( alg->isigs[i]->queue ) );
      alg->isigs[i]->upd = 1;
      printf( "      Input #%d '%s' linked to signal '%s'\n", i, alg->isigs[i]->inp, alg->isigs[i]->kks );

      EC_ERRNO( input = jcf_onext( input ) );
    }

    // now processing filters
    EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].filter_chains", a ), LINE_MAX, >= );
    int num_fchains = jcf_al( NULL, jname );
    ec_clean();
    EC_NULL( alg->filter_chains = calloc( num_fchains + 1, sizeof( SQFilterChain_t * ) ) );
    if ( num_fchains > 0 ) {
      // loop iterating through filters chains configured
      for ( int fc = 0; fc < num_fchains; fc++ ) {
        printf( "      Processing filters chain #%d\n", fc );
        EC_NULL( alg->filter_chains[fc] = malloc( sizeof( SQFilterChain_t ) ) );
        EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].filter_chains[%d].bind", a, fc ), LINE_MAX, >= );
        int num_binds;
        EC_ERRNO( num_binds = jcf_al( NULL, jname ) );
        alg->filter_chains[fc]->inum = num_binds;
        EC_NULL( alg->filter_chains[fc]->input = calloc( num_binds + 1, sizeof( float * ) ) );
        EC_NULL( alg->filter_chains[fc]->target_ids = calloc( num_binds, sizeof( int ) ) );
        // iterate through inputs to which filter chain is binded
        for ( int b = 0; b < num_binds; b++ ) {
          const char *i_name;
          EC_ERRNO( i_name = jcf_s( jcf_ai( NULL, jname, b ), NULL ) );
          for ( int i = 0; i < num_inputs; i++ ) {
            if ( strcmp( i_name, alg->isigs[i]->inp ) == 0 ) {
              printf( "         filter input #%d link to algo input #%d ('%s')\n", b, i, i_name );
              alg->filter_chains[fc]->target_ids[b] = i;
              goto loo;
            }
          }
          EC_UMSG( "Algorythm haven't input named '%s'!", i_name );
loo:
          continue;
        }

        // process filter chain
        EC_CHCK0( snprintf( jname, LINE_MAX, ".algos[%d].filter_chains[%d].chain", a, fc ), LINE_MAX, >= );
        int num_filters;
        EC_ERRNO( num_filters = jcf_al( NULL, jname ) );
        EC_NULL( alg->filter_chains[fc]->chain = calloc( num_filters + 1, sizeof( SQFilterObject_t * ) ) );
        // loop iterating through filters configured in this chain
        for ( int f = 0; f < num_filters; f++ ) {
          EC_NULL( alg->filter_chains[fc]->chain[f] = malloc( sizeof( SQFilterObject_t ) ) );
          const char *f_name;
          EC_ERRNO( f_name = jcf_s( jcf_ai( NULL, jname, f ), ".name" ) );
          const char *f_init;
          EC_ERRNO( f_init = jcf_s( jcf_ai( NULL, jname, f ), ".init_str" ) );
          SQFilterDescriptor_t *filter_class;
          EC_NULL( filter_class = sqf_getfilter( f_name ) );
          alg->filter_chains[fc]->chain[f]->super = filter_class;
          alg->filter_chains[fc]->chain[f]->init_str = f_init;
          EC_NULL( alg->filter_chains[fc]->chain[f]->subj = filter_class->create( f_init ) );
          printf( "         Filter #%d in chain -> name: '%s', init: '%s'\n", f, f_name, f_init );
        }
      }
    }

    // creating instance of algorythm
    sqr_set_curr_algo( alg );
    if ( ( alg->subj = alg->super->create( a_init ) ) == NULL ) {
      EC_UMSG( "Failed to create instance of '%s' alg (initstr: '%s')", alg->super->name, a_init );
    }
    algos[a] = alg;
  }

  fprintf( stderr, "INF> LINK ALL THINGS FIN!\n" );
  return algos;

  EC_CLEAN_SECTION(
    fprintf( stderr, "INF> LINK ALL THINGS FAILED!\n" );
    if ( algos ) free( algos );

    return NULL;
  );
}

static inline
int setup_signals( void )
{
  //! Настройка сигналов
  struct sigaction siact;
  memset( &siact, 0, sizeof( siact ) );
  siact.sa_handler = SIG_IGN;
  EC_NEG1( sigfillset( &siact.sa_mask ) );
  EC_NEG1( sigaction( SIGINT, &siact, NULL ) );
  EC_NEG1( sigaction( SIGQUIT, &siact, NULL ) );
  EC_NEG1( sigaction( SIGTERM, &siact, NULL ) );
  EC_NEG1( sigaction( SIGHUP, &siact, NULL ) );
  // siact.sa_flags = SA_NOCLDSTOP;
  // EC_NEG1 ( sigaction (SIGCHLD, &siact, NULL) );


  sigset_t mask;
  EC_NEG1( sigemptyset( &mask ) );
  EC_NEG1( sigaddset( &mask, SIGINT ) );
  EC_NEG1( sigaddset( &mask, SIGQUIT ) );
  EC_NEG1( sigaddset( &mask, SIGTERM ) );
  EC_NEG1( sigaddset( &mask, SIGHUP ) );
  EC_NEG1( sigaddset( &mask, SIGCHLD ) );
  EC_NEG1( sigprocmask( SIG_BLOCK, &mask, NULL ) );

  return 0;

  EC_CLEAN_SECTION(
    return -1;
  );
}

/****************************************************************/
/****************************************************************/
/***********************      MAIN    ***************************/
/****************************************************************/
/****************************************************************/
/*!
  @short точка входа.

  Здесь располагается основной подготовительный код приложения. @b squirrel выполняет
  здесь инициализацию, конфигурацию, связывание и выделение ресурсов.
*/
int main( int ac, char **av )
{
  //! Настройка сигналов
  EC_NEG1( setup_signals() );

  /*!
    @todo
    При входе надо выполнить проверку возможностей предоставляемых ОС.
  */
  //EC_NEG1 ( check_capabilities () );


  //! Каталог содержащий исполняемый файл @b squirrel устанавливается как рабочий.
  DO(
    char *_wd = strdup( av[0] );
    EC_NZERO( chdir( dirname( _wd ) ) );
    free( _wd );
  );

  //! Запускается выделенный процесс для @ref fastlog "логирования сообщений".
  EC_NEG1( fl_init() );

  //! Загружается основной @ref jconf "конфигурационный файл"
  EC_NEG1( jcf_load( "sq.conf" ) );



  // get dedicated cpus ids and configured prioryties
  struct {
    long tic;
    int base_prio;
    int nworks;
    struct {
      pid_t pid;
      char *name;
      int   cpu;
    } *workers;
  } sq_proc = { .workers = NULL };

  EC_ERRNO( sq_proc.tic = jcf_l( NULL, ".tic" ) );

  EC_ERRNO( sq_proc.base_prio = jcf_l( NULL, ".priority" ) );
  EC_ERRNO( sq_proc.nworks  = jcf_al( NULL, ".cpus" ) );
  if ( sq_proc.nworks < 2 ) {
    fl_log( "EMG> Not enoght cpus configured" );
    return -1;
  }
  EC_NULL( sq_proc.workers = calloc( sq_proc.nworks, sizeof( *sq_proc.workers ) ) );
  // EC_ERRNO ( sq_proc.workers[0].cpu  = jcf_l (NULL, ".cpus[0]") );
  for ( int i = 0; i < sq_proc.nworks; i++ ) {
    sq_proc.workers[i].pid = -1;
    sq_proc.workers[i].name = calloc( 200, 1 );
    if ( i == 0 ) {
      snprintf( sq_proc.workers[i].name, 200, "input task" );
    } else {
      snprintf( sq_proc.workers[i].name, 200, "worker #%d", i - 1 );
    }
    EC_ERRNO( sq_proc.workers[i].cpu = jcf_l( jcf_ai( NULL, ".cpus", i ), NULL ) );
  }



  //! Строятся списки алгоритмов, фильтров и входных сигналов заданных
  //! заданных текущей конфигурацией.
  const char **desired_algos, **desired_filters, **desired_signals;
  int num_algos, num_algos_instance, num_filters, num_signals;
  EC_NEG1( num_algos_instance = create_components_lists( &desired_algos, &num_algos, &desired_filters,
                                &num_filters, &desired_signals, &num_signals ) );



  //! Загружаются таблицы входных сигналов заданные конфигурацией.
  EC_NEG1( sqts_load_signals( desired_signals, num_signals ) );

  //! Инициализируется шина передачи сигналов между компонентами.
  EC_NEG1( sqr_init( num_signals, 1000 ) );

  //------------------------- bus is clean & ready ------------------



  /************************************************************************************/
  /**************************                              ****************************/
  /***********************           FORK POINT I             *************************/
  /**************************                              ****************************/
  /************************************************************************************/
  //! Запуск задачи приема сигналов и привязка очередей.
  EC_NEG1( sq_proc.workers[0].pid = fork() );
  if ( sq_proc.workers[0].pid == 0 ) {
    EC_NEG1( sq_set_cpu( sq_proc.workers[0].cpu, sq_proc.base_prio ) );
    return sqi_run( desired_signals, num_signals, sq_proc.tic );
  }

  // wait for input task initializes signal queues
  sqr_wait();



  //! Подключаются плагины с фильтрами необходимыми для работы в заданной конфигурации.
  EC_NEG1( sqf_load_filters( NULL, desired_filters, num_filters ) );

  //! Загрузка плагинов с алгоритмами.
  EC_NEG1( sqa_load_algos( NULL, desired_algos, num_algos ) );

  free( desired_algos );
  free( desired_filters );
  free( desired_signals );

  //! Соединение входов алгоритмов с соответствующими очередями сигналов.
  //! Подключение фильтров на входах алгоритмов.
  SQAlgoObject_t **algos;
  EC_NULL( algos = link_alltogather() );

  //! Распределение алгоритмов по доступным процессорам.
  EC_NEG1( sqb_balance( algos, sq_proc.nworks - 1 ) );
  for ( int w = 1; w < sq_proc.nworks; w++ ) {
    int idx = 0;
    for ( SQAlgoObject_t **alg = algos; *alg != NULL; alg++ )
      if ( alg[0]->pool_id == w - 1 ) {
        ++idx;
      }
    if ( idx == 0 ) {
      continue;
    }

    /************************************************************************************/
    /**************************                              ****************************/
    /***********************           FORK POINT               *************************/
    /**************************                              ****************************/
    /************************************************************************************/
    EC_NEG1( sq_proc.workers[w].pid = fork() );
    if ( sq_proc.workers[w].pid != 0 ) {
      continue;
    }
    EC_NEG1( sq_set_cpu( sq_proc.workers[w].cpu, sq_proc.base_prio - 2 ) );

    idx = 0;
    for ( SQAlgoObject_t **alg = algos; *alg != NULL; alg++ )
      if ( alg[0]->pool_id == w - 1 ) {
        algos[idx++] = *alg;
      }
    algos[idx] = NULL;
    if ( idx == 0 ) {
      return 0;
    }
    return sqw_start_worker( w - 1, algos, sq_proc.tic );
    //return 0;
  }


  // *******************************************************
  // Ok all stuff is launched, now we hang and wait for
  // signal events meaning termination is comming
  // *******************************************************
  siginfo_t info;
  sigset_t mask;
  EC_NEG1( sigemptyset( &mask ) );
  EC_NEG1( sigaddset( &mask, SIGINT ) );
  EC_NEG1( sigaddset( &mask, SIGQUIT ) );
  EC_NEG1( sigaddset( &mask, SIGTERM ) );
  EC_NEG1( sigaddset( &mask, SIGHUP ) );
  EC_NEG1( sigaddset( &mask, SIGCHLD ) );

  sigwaitinfo( &mask, &info );
  switch ( info.si_signo ) {
    case SIGCHLD:
      fprintf( stderr, "ERR> " );
      int w = 0;
      char *who = NULL;
      for ( ; w < sq_proc.nworks; w++ ) {
        if ( sq_proc.workers[w].pid == info.si_pid ) {
          // sq_proc.workers[w].pid = 0;
          who = sq_proc.workers[w].name;
          break;
        }
      }
      if ( who == NULL ) {
        /* code */
        who =  "unknown";
      }
      switch ( info.si_code ) {
        case CLD_EXITED:
          fprintf( stderr, "%s has exited with status = %d.", who, info.si_status );
          break;
        case CLD_KILLED:
          fprintf( stderr, "%s was killed.", who );
          break;
        case CLD_DUMPED:
          fprintf( stderr, "%s terminated abnormally.", who );
          break;
        case CLD_TRAPPED:
          fprintf( stderr, "%s traced child has trapped.", who );
          break;
        case CLD_STOPPED:
          fprintf( stderr, "%s child has stopped.", who );
          sigqueue( info.si_pid, SIGCONT, ( union sigval ) {
            .sival_int = 0
          } );
          break;
        case CLD_CONTINUED:
          fprintf( stderr, "%s stopped child has continued.", who );
          break;
        default:
          fprintf( stderr, "%s undefined action has happened.", who );
          break;
      }
      fprintf( stderr, " Unexpected subprocess status change, starting finalization.\n" );
      if ( info.si_code == CLD_EXITED || info.si_code == CLD_KILLED || info.si_code == CLD_DUMPED ) {
        if ( waitid( P_PID, info.si_pid, &info, WEXITED ) == -1 ) {
          fprintf( stderr, "ERR> wait of previosly terminated process failed: %s\n", strerror( errno ) );
        }
        if ( info.si_code == CLD_EXITED ) {
          fprintf( stderr, "INF> %s[%d] terminated (exit code = %d)\n", sq_proc.workers[w].name, sq_proc.workers[w].pid, info.si_status );
        } else {
          fprintf( stderr, "INF> %s[%d] terminated (by signal: %s)\n", sq_proc.workers[w].name, sq_proc.workers[w].pid, strsignal( info.si_status ) );
        }

        if ( w < sq_proc.nworks ) {
          sq_proc.workers[w].pid = -1;
        }
      }
      break;

    default:
      fprintf( stderr, "INF> got %s signal, starting finalization\n", strsignal( info.si_signo ) );
      break;
  }

  for ( int w = 0; w < sq_proc.nworks; w++ ) {
    if ( sq_proc.workers[w].pid > 0 ) {
      fprintf( stderr, "INF> sending TERM to %s[%d]...\n", sq_proc.workers[w].name, sq_proc.workers[w].pid );
      sigqueue( sq_proc.workers[w].pid, SIGTERM, ( union sigval ) {
        .sival_int = 13
      } );
      if ( waitid( P_PID, sq_proc.workers[w].pid, &info, WEXITED ) == -1 ) {
        fprintf( stderr, "ERR> wait for %s[%d] failed: %s\n", sq_proc.workers[w].name, sq_proc.workers[w].pid, strerror( errno ) );
      } else {
        if ( info.si_code == CLD_EXITED ) {
          fprintf( stderr, "INF> %s[%d] terminated (exit code = %d)\n", sq_proc.workers[w].name, sq_proc.workers[w].pid, info.si_status );
        } else {
          fprintf( stderr, "INF> %s[%d] terminated (by signal: %s)\n", sq_proc.workers[w].name, sq_proc.workers[w].pid, strsignal( info.si_status ) );
        }
      }
    }
  }

  sqr_release();
  sqr_mb_release();
  fl_fin();

  return 0;

  EC_CLEAN_SECTION(
    sqr_release();
    sqr_mb_release();
    fl_fin();
    kill( 0, SIGTERM );
    return -1;
  );
}


//! @}

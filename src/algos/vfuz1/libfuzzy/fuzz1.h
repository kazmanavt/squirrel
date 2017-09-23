#ifndef _FUZZ_H_VP
#define _FUZZ_H_VP
#define MAX_OF_VARS 1024 /* max of fuzzy input variables */
#ifdef __cplusplus
extern "C" {

#endif

typedef struct engineDesc
 {
void * engine;
void *  fuzVars[MAX_OF_VARS];
 } EngineDesc;

int  initSquirrelFuzzy(char *file_name,char *(*sigNames[]),EngineDesc *peng);


  void  setAllFuzzyInput(double *,int,EngineDesc *peng);
  double   processFuzzy(EngineDesc *peng);

#ifdef __cplusplus
          }
#endif

#endif


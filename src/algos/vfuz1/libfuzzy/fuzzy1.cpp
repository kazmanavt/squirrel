#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <fl/Headers.h>
#include <fl/imex/FisImporter.h>
#include <fl/variable/InputVariable.h>
#include "fuzz1.h"





extern "C" int  initSquirrelFuzzy(char *file_name,char *(*_sigNames[]),EngineDesc *peng)
     {
char **sigNames;
fl::Importer *importer= NULL;
fl::Engine* engine = NULL;
fl::InputVariable* tmpVal;
std::ostringstream reader;

   // read model  from file
   //
   //
       
  std::ifstream ifs(file_name);

 if (!ifs.is_open())
       {
                    std::cout <<  "Error open FIS:"  << file_name <<  std::endl;
                    return -2;
       }

  std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

      reader << content << "\n";

       importer=new fl::FisImporter();
 try{
peng->engine=engine = importer->fromString(reader.str());
  } catch (fl::Exception& ex) {
                    std::cout <<  "Error importing from FIS:"  << ex.what() <<  std::endl;
                    delete importer;
                    return -1;
                }
 
// discover the input and output signals 
//
// scan the input signals
 std::vector<fl::InputVariable*> inputVariables=engine->inputVariables();
         sigNames=NULL;

        sigNames=(char **)malloc((inputVariables.size()+1)*sizeof(char*));
        sigNames[inputVariables.size()]=0; // NULL terminate 
        for (std::size_t i = 0; i < inputVariables.size(); ++i) {
         peng->fuzVars[i]=inputVariables.at(i);
        sigNames[i]=(char*)malloc(sizeof(inputVariables.at(i)->getName().c_str()));
        strcpy(sigNames[i],inputVariables.at(i)->getName().c_str()); 
                    }
          sigNames[inputVariables.size()]=NULL;
   #if 0 
    // scan vars name and get a refernces
     while(sigNames[mumOfSigsFound] != NULL) { 
     tmpVal=engine->getInputVariable(sigNames[mumOfSigsFound]);
      peng->fuzVars[mumOfSigsFound]=tmpVal;
       mumOfSigsFound++;
             }
#endif

    delete importer;
    *_sigNames=sigNames;
   return inputVariables.size();

}

 extern "C"  void setAllFuzzyInput(double *arrayOfVal,int  numOfVals,EngineDesc *peng)
   {
     int n;

   for(n=0; n< numOfVals; n++)     
    {
  ((fl::InputVariable*)(peng->fuzVars[n]))->setValue(arrayOfVal[n]);
    }
   }
/* *************************************************************************************
 */
  extern "C" double   processFuzzy(EngineDesc *peng)
   {
   fl::Engine* engine=(fl::Engine*)peng->engine;

  engine->process(); 
  
  fl::OutputVariable*  tip= engine->getOutputVariable("output1"); // magic name for output variabvle:)
  tip->defuzzify();
  fl::scalar obtained = tip->getValue(); 
  // std::cout << obtained  << std::endl;
     return double(obtained);
   }

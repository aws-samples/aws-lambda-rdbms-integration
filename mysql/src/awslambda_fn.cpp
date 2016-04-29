/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <my_global.h>
#include <my_sys.h>
#include <string.h>
#include <mysql.h>
#include <ctype.h>

#ifdef HAVE_DLOPEN

#define MAX_JSON_RESULT 8192
char *lambda_invoke_sync(char *fn_name, char *fn_argsJson) ;
char *lambda_invoke_async(char *fn_name, char *fn_argsJson) ;

// enable C symbols for linking from mysqld 
extern "C" {

//my_bool awslambda_fn_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
//my_bool awslambda_fn_async_init(UDF_INIT *initid, UDF_ARGS *args, char *message);

//void awslambda_fn_deinit(UDF_INIT *initid);
//void awslambda_fn_async_deinit(UDF_INIT *initid);

//char *awslambda_fn(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error); 
//char *awslambda_fn_async(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error); 


// init function                                                          
my_bool awslambda_fn_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if(!(args->arg_count == 2) ||
       args->arg_type[0] != STRING_RESULT ||
       args->arg_type[1] != STRING_RESULT ) {
      strcpy(message, "Expected two string arguments (fn_name, fn_argsJson)");
      return 1;
    }
    //allocate memory for function result - Json string
    initid->ptr = (char *)malloc(MAX_JSON_RESULT);
    return 0;
}
my_bool awslambda_fn_async_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return awslambda_fn_init(initid,args,message) ;
}

// deinit function                                                        
void awslambda_fn_deinit(UDF_INIT *initid)
{
    // deallocate result buffer memory
    free(initid->ptr);
}
void awslambda_fn_async_deinit(UDF_INIT *initid)
{
    awslambda_fn_deinit(initid) ;
}


// Actual function                                                        
char *call_lambda_invoke(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error, int syncMode) 
{
    char *fn_name = (char *)(args->args[0]);
    char *fn_argsJson = (char *)(args->args[1]);
    char *fn_result ;
    if (syncMode) {
       fn_result = lambda_invoke_sync(fn_name,fn_argsJson) ;
    } else {
       fn_result = lambda_invoke_async(fn_name,fn_argsJson) ;
    }
    if (!fn_result) {
       *error = 1 ;
       return NULL ;
    }
    strcpy(initid->ptr,fn_result); 
    *length = strlen(fn_result) ;
    free(fn_result); 
    return(initid->ptr);
}

char *awslambda_fn(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error) 
{
    return call_lambda_invoke(initid,args,result,length,is_null,error,1) ;
}

char *awslambda_fn_async(UDF_INIT *initid, UDF_ARGS *args, char *result, unsigned long *length, char *is_null, char *error) 
{
    return call_lambda_invoke(initid,args,result,length,is_null,error,0) ;
}

} //extern "C"
#endif /* HAVE_DLOPEN */

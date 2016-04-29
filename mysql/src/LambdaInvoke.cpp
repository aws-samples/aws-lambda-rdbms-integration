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

#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSSet.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/ratelimiter/DefaultRateLimiter.h>
#include <aws/core/utils/HashingUtils.h>

#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/InvokeRequest.h>

#include <aws/core/utils/memory/stl/AWSStringStream.h> 

using namespace Aws::Lambda;
using namespace Aws::Lambda::Model;

char *lambda_invoke(char *fn_name, char *fn_argsJson, InvocationType fn_invocationType)
{
    LambdaClient client;
    InvokeRequest invokeRequest;
    invokeRequest.SetFunctionName(fn_name);
    invokeRequest.SetInvocationType(fn_invocationType);
    auto payload = Aws::MakeShared<Aws::StringStream>("invokeLambda");
    *payload << fn_argsJson;
    invokeRequest.SetBody(payload);
    InvokeOutcome invokeOutcome = client.Invoke(invokeRequest);
    if (!invokeOutcome.IsSuccess()) {
        std::cerr << "Function invocation failed!\n";
        return NULL;
    }
    auto &result = invokeOutcome.GetResult();
    auto jsonResponse = Aws::Utils::Json::JsonValue(result.GetPayload());
    Aws::String strResponse = jsonResponse.WriteReadable();
    Aws::String errType = result.GetFunctionError() ;    
    if (!errType.empty()) {
       std::cerr << errType << ":  error in Lambda invokeResult: " << strResponse << "\n" ;
       return NULL;
    }
    char *strResponsePtr = (char *)malloc(strlen(strResponse.c_str())+1) ;
    strcpy(strResponsePtr, strResponse.c_str()) ;
    return(strResponsePtr);
}

char *lambda_invoke_sync(char *fn_name, char *fn_argsJson)
{
    return lambda_invoke(fn_name, fn_argsJson, InvocationType::RequestResponse);
}

char *lambda_invoke_async(char *fn_name, char *fn_argsJson)
{
    return lambda_invoke(fn_name, fn_argsJson, InvocationType::Event);
}

//int main() {
//   char fn_name[] = "lambdaTest" ;
//   char fn_argsJson[] = "{\"name\":\"bob\"}" ;
//   std::cout << lambda_invoke_sync(fn_name,fn_argsJson);
//}

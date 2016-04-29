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

package com.amazonaws.rdbmsinteg;

import java.io.*;
import java.nio.*;
import java.lang.*;
import com.amazonaws.AmazonClientException;
import com.amazonaws.AmazonServiceException;
import com.amazonaws.regions.*;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.InstanceProfileCredentialsProvider;
import com.amazonaws.services.lambda.AWSLambdaClient;
import com.amazonaws.services.lambda.model.InvocationType;
import com.amazonaws.services.lambda.model.InvokeRequest;
import com.amazonaws.services.lambda.model.InvokeResult;

public class LambdaInvoke {

    public static String invoke_sync(String fn_name, String fn_argsJson) throws Exception {
       return invoke(fn_name, fn_argsJson, "RequestResponse");
    }

    public static String invoke_async(String fn_name, String fn_argsJson) throws Exception {
       return invoke(fn_name, fn_argsJson, "Event");
    }

    private static String invoke(String fn_name, String fn_argsJson, String fn_invocationType) throws Exception {
	String result;
	   AWSLambdaClient client = new AWSLambdaClient();  // default credentials chain will find creds from instance metadata
	   InvokeRequest invokeRequest = new InvokeRequest();
	   invokeRequest.setFunctionName(fn_name);
       invokeRequest.setPayload(fn_argsJson);
	   invokeRequest.setInvocationType(InvocationType.fromValue(fn_invocationType));
	   InvokeResult invokeResult = client.invoke(invokeRequest);
	   ByteBuffer resultBytes = invokeResult.getPayload();
	   result = new String(resultBytes.array()) ;
	   String errType = (invokeResult.getFunctionError()); 
       if (errType != null && !errType.isEmpty()) {
		   System.err.println(errType + " error in Lambda invokeResult: " + result);
		   throw new Exception(result) ;
	   }
	return result;
    }

    public static void main (String[] args) throws Exception {
        String result = invoke_sync("lambdaTest","{\"name\":\"bob\"}") ;
        System.out.println(result + "\n");
    }

}

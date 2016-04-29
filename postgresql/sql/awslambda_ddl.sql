CREATE LANGUAGE plpythonu;

/* SYNCHRONOUSLY CALL NAMED LAMBDA FUNCTION */
CREATE OR REPLACE FUNCTION awslambda_fn(fn_name text, fn_args text)
    RETURNS text
    AS $$
     import boto3
     import boto.utils
     region=boto.utils.get_instance_identity()['document']['region']
     client=boto3.client('lambda',region)
     response=client.invoke(FunctionName=fn_name,
                            InvocationType='RequestResponse',
                            Payload=fn_args,
                           )
     r = response['Payload'].read()
     if ( 'FunctionError' in response ):
         raise Exception(r)
     return r
   $$ language plpythonu ;
GRANT EXECUTE ON FUNCTION awslambda_fn(text, text) TO PUBLIC;

/* ASYNCHRONOUSLY CALL NAMED LAMBDA FUNCTION */
CREATE OR REPLACE FUNCTION awslambda_fn_async(fn_name text, fn_args text)
    RETURNS text
    AS $$
     import boto3
     import boto.utils
     region=boto.utils.get_instance_identity()['document']['region']
     client=boto3.client('lambda',region)
     response=client.invoke(FunctionName=fn_name,
                            InvocationType='Event',
                            Payload=fn_args,
                           )
     r = response['Payload'].read()
     if ( 'FunctionError' in response ):
         raise Exception(r)
     return r
   $$ language plpythonu ;
GRANT EXECUTE ON FUNCTION awslambda_fn_async(text, text) TO PUBLIC;

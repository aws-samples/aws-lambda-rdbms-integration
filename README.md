# From SQL to Microservices: Integrating AWS Lambda with Relational Databases 

This is the code repository associated with the AWS Big Data Blog post, ["From SQL to Microservices: Integrating AWS Lambda with Relational Databases"](http://blogs.aws.amazon.com/bigdata/).

----------

[AWS Lambda](https://aws.amazon.com/lambda/) has emerged as excellent compute platform for modern [microservices](http://martinfowler.com/articles/microservices.html) architecture, driving dramatic advancements in flexibility, resilience, scale and cost effectiveness. Many customers can take advantage of this transformational technology from within their existing relational database application. In this post, we explore how to integrate your Amazon EC2 hosted Oracle or PostgreSQL database with AWS Lambda, allowing your database application to participate in a microservices architecture.
  
![](images/LambdaSQLAPI.png)

Here are a few of the reasons why you might find this capability useful:

- **Instrumentation:** Use database triggers to call a Lambda function when important data is changed in the database. Your Lambda function can easily integrate with Amazon CloudWatch, allowing you to create custom metrics, dashboards and alarms based on changes to your data.

- **Outbound streaming:** Again, use triggers to call Lambda when key data is modified. Your Lambda function can post messages to other AWS services such as Amazon SQS, Amazon SNS, Amazon SES, or Amazon Kinesis Firehose, to send notifications, trigger external workflows, or to push events and data to downstream systems, such as an Amazon Redshift data warehouse.

- **Access external data sources:** Call Lambda functions from within your SQL code to retrieve data from external web services, read messages from Amazon Kinesis streams, query data from other databases, and more. 

- **Incremental modernization:** Improve agility, scalability, and reliability, and eliminate database vendor lock-in by evolving in steps from an existing monolithic database design to a [well-architected](https://aws.amazon.com/blogs/aws/are-you-well-architected/), modern microservices approach. Incrementally migrate business logic embodied in database procedures into database-agnostic Lambda functions, while preserving compatibility with remaining SQL packages.

I’ll revisit these scenarios in [Part 2](#part-2---example-use-cases), but first you need to establish the interface that enables SQL code to invoke Lambda functions.

## Part 1 - Set up SQL-to-Lambda interface

You will create user-defined functions in the database in a programming language supported by both the RDBMS and the AWS SDK. These functions use the AWS SDK to invoke Lambda functions, passing data in the form of JSON strings.
  
This post shows you the steps for Oracle and PostgreSQL. 

### Create a test function in Lambda

1.  Sign in to the [AWS Management Console](http://console.aws.amazon.com/) and open the AWS Lambda console.
2.  Choose **Create a Lambda function**.
3.  On the **Select blueprint** page, choose **Skip**.
4.  On the **Configure Function** page, enter `lambdaTest` for **Name**, and choose **Python 2.7** for **Runtime**.
5.  Paste the following code for **Lambda Function Code**:

    ```
	def lambda_handler(event, context):
	    output = event
	    output["Status"] = "OK"
	    return output
    ```
6.  Assign a value for **Role** to the function. If you have used Lambda before, you can select an existing role; otherwise, select the option to create a new `Basic execution role`.
7.  The value of **Timeout** defaults to 3 seconds, which is fine for this function. Other functions may need more time to execute.
8.  Accept all other defaults and choose **Next**, **Create Function**.
9.  Test your function from the console by choosing **Test**, and verifying that it runs with no errors and that it returns a JSON object reflecting the input data with an added “Status” key.

    ![](images/lambdaTestResult.png)
  
### Create IAM EC2 Instance Role

The EC2 instance running your database server needs an associated role with an attached policy granting permission to invoke Lambda functions.

1.  Open the [AWS Management Console](http://console.aws.amazon.com/) and launch the IAM console.
2.  Choose **Roles** and **Create New Role**. For **Role Name**, enter `rdbms-lambda-access` and choose **Next Step**.
3.  On the **Select** **Role type** page, choose **Amazon EC2**.
4.  On the **Attach** **Policy** page, choose **AWSLambdaFullAccess** and **Next Step**.
5.  Choose **Create Role**.

### Create database on EC2 instance using the IAM role

Create a new database by launching a pre-configured Oracle or PostgreSQL AMI from the AWS Marketplace. Or, if you already have an Oracle or PostgreSQL server running on EC2, migrate it to a new instance so you can apply the newly created IAM role.

As you launch your new instance, be sure to specify the new IAM role, `rdbms-lambda-access`, in the **Configure Instance Details** page.

Connect to your new instance using SSH, and complete any necessary steps to ensure that your database is running and that you can connect with administrative privileges using the native database client.

The following two sections are database vendor specific. If you are using PostgreSQL, skip to the [PostgreSQL Setup](#postgresql-setup) section below.

### Oracle Setup

#### Step 1: LambdaInvoke java class
  
Oracle supports the use of Java methods for UDFs. The Java class below uses the AWS SDK to invoke a named Lambda function (fn\_name) in either synchronous (RequestResponse) or asynchronous (Event) mode, passing parameters in the form of a JSON string (fn\_argsJson):

```
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
		   throw new Exception(result) ;
	   }
	return result;
    }
}
```

Follow the instructions below to build and install the `LambdaInvoke` class:
  
1. Install **git** and **maven** on the database server instance, if they're not already installed.  
1. As the **oracle** user, download and build the `aws-lambda-rdbms-integration` project from github. Steps:  
   ```
   sudo su - oracle
   git clone https://github.com/awslabs/aws-lambda-rdbms-integration.git
   cd aws-lambda-rdbms-integration/oracle
   mvn clean package
   ```
     
   This builds a self contained .jar file containing the `LambdaInvoke` java class and all its dependencies, including the AWS SDK class files.
  
1. Verify that the EC2 instance role is correct and that you can connect to the AWS Lambda service and successfully call our function, using the main method included in the class:
   ```
   java -cp target/aws-rdbmsinteg-1.0.jar com.amazonaws.rdbmsinteg.LambdaInvoke
   ```
   If all is well, the following output will be displayed:
   ``` 
   {"Status": "OK", "name": "bob"}
   ```
  
1. Load the .jar file into the Oracle database:
   ```
   loadjava -user system/<password> target/aws-rdbmsinteg-1.0.jar
   ``` 

#### Step 2: User-defined functions

Here is an example Oracle function designed to use the invoke_sync() method of the LambdaInvoke java class to launch a named Lambda function in synchronous mode:
```
CREATE OR REPLACE FUNCTION awslambda_fn(fn_name VARCHAR2, fn_argsJson VARCHAR2) 
    RETURN VARCHAR2
    AS LANGUAGE JAVA
    NAME 'com.amazonaws.rdbmsinteg.LambdaInvoke.invoke_sync(java.lang.String, java.lang.String) return java.lang.String';
```
   

1. Create the `awslambda_fn()` and `awslambda_fn_async()` functions using the script provided:       
   ```
   cat sql/awslambda_ddl.sql | sqlplus system/<password>
   ```

1. Grant required permissions to the SYSTEM user using the script provided:
   ```
   cat sql/permissions.sql | sqlplus system/<password>
   ```

1. Oracle's Java keystore must trust the certificate authority (CA) used by the AWS service; by default it is empty. An easy way to fix this problem is to replace the default Oracle Java keystore with a populated keystore from a standard Java installation:
   ```
   cp $ORACLE_HOME/javavm/lib/security/{cacerts,cacerts.orig} 
   cat /usr/lib/jvm/jre-1.7.0-openjdk.x86_64/lib/security/cacerts > $ORACLE_HOME/javavm/lib/security/cacerts
   ```

1. Log into the database, and test the awslambda_fn function, passing the name of your Lambda function and a JSON input parameter string:
   ```
   sqlplus system/<password>
   SQL> SELECT awslambda_fn('lambdaTest','{"name":"bob"}') AS lambdatest FROM DUAL;
   
   LAMBDATEST
   --------------------------------------------------------------------------------
   {"Status": "OK", "name": "bob"}
   ```
   
Success! Using an Oracle SQL select statement, you have successfully invoked your test function on Lambda, and retrieved the results.


### PostgreSQL Setup

For PostgreSQL, use the PL/Python language to create your UDFs, leveraging the AWS Python SDK to launch Lambda functions and retrieve the results.

#### Step 1: Prerequisites

1. Make sure your database EC2 instance has Python and the two AWS SDK modules, **boto** and **boto3**, installed:
   ```
   sudo pip install boto boto3
   python -c "import boto; import boto3; print 'AWS SDK verified OK'"
   ```

1. Download the `aws-lambda-rdbms-integration` project as the postgres user:
   ```
   sudo su - postgres
   git clone https://github.com/awslabs/aws-lambda-rdbms-integration.git
   cd aws-lambda-rdbms-integration/postgresql
   ```
  
#### Step 2: User-defined functions

The PostgreSQL function below uses the AWS SDK to invoke a named Lambda function in synchronous mode:
```
CREATE LANGUAGE plpythonu;
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
```
1. Create the awslambda_fn() and awslambda_fn_async() functions using the script provided:    
   ``` 
   psql -U postgres -f sql/awslambda_ddl.sql
   ```

1. Log into the database, and call the awslambda_fn function, passing the name of the test Lambda function and a JSON input parameter string:
   ```
   psql -U postgres  
   postgres=# SELECT awslambda_fn('lambdaTest','{"name":"bob"}') AS lambdatest ;  
   
                          lambdatest
   ----------------------------------------------------------
    {"Status": "OK", "name": "bob"}
   (1 row)
   ```  
   
Success! Using a PostgreSQL SQL select statement, you have successfully invoked your test function on Lambda, and retrieved the results.

Now, let's explore some interesting use cases for your new Lambda interface.

## Part 2 - Example Use Cases

Now that the mechanics are in place, you can create new Lambda functions to do useful things! Let's revisit some of the use cases mentioned earlier.

  
### Instrumentation: Monitor and Alert with Amazon CloudWatch

Assume that you have an existing application which uses an Oracle database to track temperature sensor readings. Assume that the readings are stored in a two-column table:
```
CREATE TABLE temp_reading (reading_time TIMESTAMP, reading_value NUMERIC);
```

You can forward new records via Lambda to CloudWatch, allowing you to plot graphs of the temperature readings, and to configure alerts when a reading exceeds a threshold.

1. Create a new Python function in AWS Lambda by following the process used [earlier](#create-test-function-in-lambda).  
   Name the function `lambdaTempReadingTrigger`, and use the function code below:
   
	   ```
		import boto3
		client = boto3.client('cloudwatch')
		def lambda_handler(event, context):
		   t = event["reading_time"]
		   v = float(event["reading_value"])
		   print "New temperature reading: Time: %s, Temp: %.2f" % (t, v)
		   client.put_metric_data(
		       Namespace = 'Temperature Monitoring Database App',
		       MetricData = [{
				                'MetricName':'Temperature Reading',
				                'Timestamp': t,
				                'Value': v,
				            }]
		       )
		   return {"Status":"OK"}
	   ```
   
   Assign the policy **CloudWatchFullAccess** to the IAM role used by the Lambda function.
   Set Timeout to 10 seconds.   
   Save and test the function using this sample JSON input:
   
   ```
    {
	  "reading_time": "2016-02-15 12:00:00",
	  "reading_value": "60"
	}
   ```
   
   After running the test, the **Execution Result** field should display the message: `{"Status":"OK"}`.
     

1. Using your Oracle SQL client, create a database trigger:
   
	```
	CREATE OR REPLACE TRIGGER temp_reading_trigger 
		AFTER INSERT 
		ON temp_reading 
		FOR EACH ROW
		DECLARE
			fn_name varchar2(32) := 'lambdaTempReadingTrigger';
			fn_args varchar2(255);
			t varchar2(32);
			v number;
			res varchar2(32);
		BEGIN
			t := TO_CHAR(SYS_EXTRACT_UTC(:new.reading_time),'YYYY-MM-DD HH24:MI:SS');
			v := :new.reading_value;
			fn_args := '{"reading_time":"' || t || '", "reading_value":"' || v || '"}' ;
			DBMS_OUTPUT.PUT_LINE('Calling: ' || fn_name || ', input: ' || fn_args);
			SELECT awslambda_fn_async(fn_name, fn_args) INTO res FROM DUAL;
		END;
		/
	```

Now, when your application inserts a new reading into the `temp_reading` table, the value will be posted to the new `Temperature Reading` metric in CloudWatch.

Simulate adding a series of temperature readings to test our CloudWatch integration:  
```
SET SERVEROUTPUT ON;  -- display the debug log printed by the trigger
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '80' MINUTE, 65);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '70' MINUTE, 65);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '60' MINUTE, 63);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '50' MINUTE, 67);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '40' MINUTE, 74);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '30' MINUTE, 82);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '20' MINUTE, 85);
INSERT INTO temp_reading VALUES (CURRENT_TIMESTAMP - INTERVAL '10' MINUTE, 86);
```

In the *CloudWatch* console, for *Custom Metrics*, choose *Temperature Monitoring Database App*, *Temperature Reading*. Choose *Add to Dashboard* to display this chart with other charts and metrics. Choose *Create Alarm* to define your desired alarm threshold and notification actions.

   ![](images/cloudWatchMetric.png)
  

### Outbound Streaming: Synchronize Updates to Amazon Redshift

You can adapt this approach to publish records to other downstream systems. In this example, you use Amazon Kinesis Firehose to stream your temperature readings to an Amazon Redshift data warehouse, where the data can be integrated with other business data and used for reporting and analytics.

1.  Create a target table on your Amazon Redshift cluster:  
	   ```
	   CREATE TABLE rs_temp_reading (reading_time TIMESTAMP, reading_value NUMERIC);
	   ```
	   
1. Configure the [Amazon Redshift security group](http://docs.aws.amazon.com/firehose/latest/dev/controlling-access.html#using-iam-rs)  

1. Create a [Firehose stream](http://docs.aws.amazon.com/firehose/latest/dev/basic-create.html#console-to-redshift)
   - For **Delivery stream name**, enter `FirehoseTempReading.`
   - For **Redshift table**, enter `rs_temp_reading.`
   - For **Redshift COPY Parameters**, enter `FORMAT AS JSON 'auto'.`
   - Follow the guide for all other settings to create the Delivery Stream.  

1. Update the Lambda function created in the previous example, `lambdaTempReadingTrigger`, using the sample code below:
    ```
	import boto3
	import json
	client = boto3.client('firehose')
	def lambda_handler(event, context):
		record = json.dumps(event) + "\n"
		print "New temperature reading record: %s" % record
		client.put_record(
		   DeliveryStreamName = 'FirehoseTempReading',
		   Record = { 
		       		'Data':bytes(record) 
		       		}
		   )
		return {"Status":"OK"}      
      ```
       
        Assign the policy AmazonKinesisFirehoseFullAccess to the IAM role used by the Lambda function.

	Save and test the function, using the same test input data as before.  
	After running the test, the **Execution Result** field should display the message: `{"Status":"OK"}`.
   
Now, when your application inserts a new reading into the temp_reading table, the value is posted to the new Firehose delivery stream, and from there (after up to 5 minutes) to the rs_temp_reading table in your Amazon Redshift data warehouse. Try it for yourself!
   
   ![](images/redshift.png)


### Access External Data Example - Read From a Kinesis Stream

Synchronous calls to Lambda allow your database to tap into virtually any data from any source that a Lambda function can access. In this example, I show how to use a SQL function to read streaming messages from Amazon Kinesis.

1. In the Amazon Kinesis console, choose Create Stream. For Name, enter MySensorReadings, and for Number of Shards, enter 1.
   
1. Put some test records into the stream, using the [AWS CLI](https://aws.amazon.com/cli/): 
   ```
   aws kinesis put-record \
               --stream-name "MySensorReadings" \
               --partition-key "Sensor1" --data '{"time":"2016-02-16 12:00:00","temp":"99"}'
   aws kinesis put-record \
               --stream-name "MySensorReadings" \
               --partition-key "Sensor1" --data '{"time":"2016-02-16 12:30:00","temp":"100"}'
   ```

1. Create a new AWS Lambda Python2.7 function named 'lambdaReadKinesisStream', using the code below:
	```
	import boto3
	import json
	import time
	client = boto3.client('kinesis')
	def lambda_handler(event, context):
	    print "Reading Kinesis stream - %s" % json.dumps(event)
	    timeout = 3
	    streamName = event["StreamName"]
	    if 'Timeout' in event.keys():
	        timeout = float(event["Timeout"])
	    if 'ShardId' in event.keys():
	        shardId = event["ShardId"]
	    else:
	        shardId = client.describe_stream(StreamName = event["StreamName"])["StreamDescription"]["Shards"][0]["ShardId"]
	    if 'NextShardIterator' in event.keys() and event["NextShardIterator"]:
	        shardIterator = event["NextShardIterator"]
	    else:
	        if 'SequenceNumber' in event.keys() and event["SequenceNumber"]:
	            shardIterator = client.get_shard_iterator(
	                StreamName = streamName,
	                ShardId = shardId,
	                ShardIteratorType = "AFTER_SEQUENCE_NUMBER",
	                StartingSequenceNumber = event["SequenceNumber"]
	            )["ShardIterator"]
	        else:
	            shardIterator = client.get_shard_iterator(
	                StreamName = streamName,
	                ShardId = shardId,
	                ShardIteratorType = "TRIM_HORIZON"
	            )["ShardIterator"]
	    # read record
	    timeout_time = time.time() + timeout
	    while 1==1:
	        response = client.get_records(ShardIterator=shardIterator, Limit=1)
	        shardIterator = response['NextShardIterator']
	        output = {"Data":"", "SequenceNumber":""}
	        if (len(response["Records"]) > 0):
	            output["Data"] = json.loads(response["Records"][0]["Data"])
	            output["SequenceNumber"] = response["Records"][0]["SequenceNumber"]
	            output["NextShardIterator"] = shardIterator
	            break
	        if (time.time() > timeout_time):
	            output["NextShardIterator"] = shardIterator
	            break
	        time.sleep(0.2)
	    return output
	```
	 
   Assign the policy AmazonKinesisFullAccess to the IAM role used by the Lambda function.
    	   
   Save and test the function using this sample JSON input: 
   
   ```
	{
	  "StreamName": "MySensorReadings"
	}
   ```
   
   The JSON output should contain a Data object with the temp and time fields from your first Amazon Kinesis test record, and a record SequenceNumber and NextShardIterator:
   ```
   {
	  "Data": {
	    "temp": "99",
	    "time": "2016-02-16 12:00:00"
	  },
	  "NextShardIterator": "AAAAAAAAAAEOsl...truncated",
	  "SequenceNumber": "49559422897078301099101618386632370618817611442279153666"
	}
   ```
   
   
1. Log into the database and select the first Kinesis record from the stream:
	```
	SELECT awslambda_fn('lambdaReadKinesisStream','{"StreamName": "MySensorReadings"}') AS kinesistest ;
	
	kinesistest
	--------------------------------------------------------------------------------
	 {"Data": {"temp": "99", "time": "2016-02-16 12:00:00"}, "NextShardIterator": "AAAAAAAAAAEIK4p...truncated", "SequenceNumber": "49559422897078301099101618386632370618817611442279153666"
	```
	
	You can cast the return string to the database's JSON data type, and use the JSON functions to extract fields. For example, in PostgreSQL:  
	```
	postgres=# CREATE TEMP TABLE message AS SELECT CAST(awslambda_fn('lambdaReadKinesisStream', '{"StreamName": "MySensorReadings"}') as JSON) AS msg ;

	postgres=# SELECT msg->'Data'->'time' AS time, msg->'Data'->'temp' AS temp FROM message ;
	         time          | temp
	-----------------------+------
	 "2016-02-16 12:00:00" | "99"
	(1 row)
	```

1. To read the next record from the Kinesis shard, pass the `SequenceNumber` of the previous record as an input parameter:  
   ```
   SELECT awslambda_fn('lambdaReadKinesisStream','{"StreamName": "MySensorReadings", "SequenceNumber": "49559422897078301099101618386632370618817611442279153666"}') AS kinesistest ;
   
	kinesistest
	--------------------------------------------------------------------------------
	{"Data": {"temp": "100", "time": "2016-02-16 12:30:00"}, "NextShardIterator": "AAAAAAAAAAHAx...truncated", "SequenceNumber": "49559422897078301099101618386633579544637226208892813314"}
   ```

1. To iterate through all the messages in a Kinesis shard in a loop, capture the value of 'NextShardIterator' from the previous response, and pass it to the next request. 
   Here is an example function in PostgreSQL which will iterate through all the messages:
   ```
   CREATE OR REPLACE FUNCTION processSensorReadings(sequenceNumber text)
   RETURNS text
   AS $$
   DECLARE
      parm text;
      msg json;
      shardIter text ;
      data text ;
      time text;
      temp text ;
   BEGIN
      SELECT '{"StreamName": "MySensorReadings","SequenceNumber": "' || sequenceNumber || '"}' INTO parm ;
      LOOP
	      SELECT CAST(awslambda_fn('lambdaReadKinesisStream', parm) as JSON) INTO msg ;
	      SELECT msg->'NextShardIterator' INTO shardIter;
	      SELECT TRIM(BOTH '"' FROM shardIter) INTO shardIter; --remove quotes
	      SELECT msg->'Data' INTO data ;
	      EXIT WHEN data = '""' ;  -- message is empty - exit the loop
	      SELECT msg->'SequenceNumber' INTO sequenceNumber;
	      SELECT msg->'Data'->'time' INTO time;
	      SELECT msg->'Data'->'temp' INTO temp;
	      RAISE NOTICE 'Time %, Temperature %', time, temp ;
	      SELECT '{"StreamName": "MySensorReadings","NextShardIterator": "' || shardIter || '"}' INTO parm ;
	      -- Do something with the data here - maybe insert into a table?
      END LOOP;
      RETURN sequenceNumber;
   END ;
   $$ LANGUAGE plpgsql;
   ```
   
   Try reading all the sensor readings in your Kinesis stream. 
   ```
   postgres=# select processSensorReadings('') as lastsequencenumber;
	NOTICE:  Time "2016-02-16 12:00:00", Temp "99"
	NOTICE:  Time "2016-02-16 12:30:00", Temp "100"
	NOTICE:  Time "2016-02-16 13:00:00", Temp "110"
	
	                    lastsequencenumber
	----------------------------------------------------------
	 49559422897078301099101618386644459877013817107654115330

	(1 row)
   
   ```

	Try putting some additional messages on the stream, and call the function again, this time passing in the `lastsequencenumber` value returned by the previous call. You will see that it will pick up where it left off, and read only the new messages. 
	
In this example we showed you how to iterate through messages on an Amazon Kinesis stream from within your database. Using AWS Lambda functions, you can integrate any number of potential external data sources, including Amazon DynamoDB, other databases, and external web services, to name just a few.


### Incremental modernization

The ability to access Lambda microservices from a relational database allows you to refactor business logic incrementally, systematically eliminating database packages, procedures, functions, and triggers, replacing them with database-agnostic services implemented as Lambda functions.
  
This approach enables incremental modernization roadmaps which avoid high-risk ‘boil the ocean’ scenarios often necessitated by highly interdependent legacy SQL code. Following the implementation of a business logic feature on Lambda, SQL code remaining in the database can continue to access the feature by calling the new Lambda function via SQL.
  
While the Lambda SQL integration is an important enabler for incremental migration, it is not (of course) sufficient to ensure success. Embrace agile software delivery best practices, systematically moving in the direction of a sound architectural vision with each iteration. Your strategic architecture should incorporate data access layer services to achieve database independence and introduce flexibility to mix and match different persistence layer technologies as appropriate (e.g., caching, noSQL, alternative RDBMS engines, etc.).
  
In the end, when all the business logic is successfully migrated out of the database, there may be no more need for the Lambda SQL API. At this point all your business logic is embodied in the Lambda services layer, and the database has become a pure persistence layer. The API is ultimately a means to an end, making the modernization roadmap possible.


![](images/microservicesArch.png)


## Frequently asked questions

#### What happens if the Lambda function fails?

In synchronous mode, fatal errors encountered while launching or running the Lambda function throw an exception back to the database SQL engine. You can write an exception handler in your SQL code, or simply allow the failure to roll back your transaction.

In asynchronous mode, failures to launch the function result in SQL exceptions, but runtime failures encountered during execution are not detected by the SQL engine.

While your SQL transactions can roll back on failure, there is no inherent rollback capability built into Lambda. The effects of any code executed by your Lambda function prior to a failure may be persistent unless you have coded your own rollback or deduplication logic.

Use CloudWatch for troubleshooting, and to monitor the health metrics and log files generated by your Lambda functions.

#### How does it perform? 

Typically, the invocation overhead observed is in the order or 10-20ms per call, but your mileage may vary. Latency is higher immediately after a Lambda function is created, updated, or if it has not been used recently. For synchronous mode calls, the overall time your function takes depends on what your Lambda function does.

Lambda automatically scales to handle your concurrency needs. For more information, see the [AWS Lambda FAQ](https://aws.amazon.com/lambda/faqs/).

Throughput and latency requirements should guide your decisions on how to apply Lambda functions in your application. Experiment and test! You may find that calling Lambda in row level triggers throttles your insert rate beyond the tolerance of your application, in which case you need to come up with a different approach, possibly using micro-batches to amortize the invocation overhead across many rows.


#### Can I restrict user access?

Yes, you manage permissions on the Lambda UDFs as with any other database object.

For simplicity, the examples above allow any database user to invoke any Lambda function by:  
-  Assigning the permissive `AWSLambdaFullAccess` policy to the database EC2 instance role
-  Granting public execute privileges to your UDFs

In production, you should probably implement less permissive policies, for example:  
-  Modify the EC2 instance role policy to allow access to specified Lambda functions only.  
-  Create additional user-defined SQL functions as wrappers to enforce specific parameters.  
   Grant execute privileges on the wrapper functions only, and deny access to the general purpose functions.  
  
#### Can my Lambda functions read or update the database directly?  

Yes, of course. Your Lambda function can execute SQL on the database you're calling it from, or on any other database / data source for which it has connection credentials. Create a [deployment package](http://docs.aws.amazon.com/lambda/latest/dg/deployment-package-v2.html) which includes your function code along with all database client libraries that your code uses to interact with the database.  

Amazon VPC support was recently introduced for AWS Lambda; this feature allows your code to access database resources running in private subnets inside your VPC.

#### How do I handle JSON formatted input and output?

The JSON features offered by your database may help with JSON handling, as illustrated in the "Read from an Amazon Kinesis stream" example above.

#### Can Lambda functions handle batches, multi-row input or output?

Here are a few options you can consider for working with record batches:  
- Flatten multiple records into a JSON array; this method is limited by input/output JSON string length restrictions.    
- Buffer multiple records in Kinesis, using the input and output streaming techniques illustrated above.   
- Have your Lambda function connect to the database, and use named tables for input and output.   

#### Can’t I call AWS services, such as CloudWatch or Firehose, directly from the database function, without needing a Lambda function in the middle? 
  
Indeed. Using a similar technique, the AWS SDK could be used to directly invoke any other AWS service from a database UDF. 
  
However, by using Lambda consistently, you avoid the need to create separate database specific UDFs for each service, and you start to move toward a microservices architecture, with flexibility to deploy logic enhancements and changes to functions that are now abstracted from the database. 
  
#### Will the same technique work on other databases, such as MySQL or MS SQL Server?
  
Yes, as long as the database supports user defined functions defined in a programming language supported by an AWS SDK. 
  
The project GitHub repository includes a prototype implementation for MySQL, using C++ UDF extensions which leverage the new AWS C++ SDK. 
  
It should also be possible to apply the same techniques to MS SQL Server by defining UDFs in .NET, leveraging the AWS .NET SDK. 


#### Does this approach work on Amazon RDS databases?

No, not currently.

## Conclusion

In this post, you’ve seen how you can integrate your existing EC2 PostgreSQL or Oracle database with AWS Lambda. You’ve also reviewed some examples of how you might use this capability to benefit from some of the many advantages of a serverless, microservices architecture, either to enhance the capabilities of your existing application, or to start down a path of incremental modernization.
  
Are these techniques useful? Please feel free to ask questions, and to share your thoughts. We'd love to hear what you think!



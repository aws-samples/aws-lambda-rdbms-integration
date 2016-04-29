import boto3
import json
client = boto3.client('firehose')
def lambda_handler(event, context):
    record = json.dumps(event) + "\n"
    print "New temperature reading record: %s" % record
    client.put_record(
       DeliveryStreamName = 'FirehoseTempReading',
       Record = { 'Data':bytes(record) }
       )
    return {"Status":"OK"}  

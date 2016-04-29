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
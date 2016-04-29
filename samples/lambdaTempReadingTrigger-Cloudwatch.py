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
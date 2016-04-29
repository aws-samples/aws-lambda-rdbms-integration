def lambda_handler(event, context):
    output = event
    output["Status"] = "OK"
    return output

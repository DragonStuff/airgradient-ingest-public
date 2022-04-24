import json
import boto3
import time

def lambda_handler(event, context):
    """Airgradient Data Proxy which sends data to AWS Timestream

    Parameters
    ----------
    event: dict, required
        API Gateway Lambda Proxy Input Format

        Event doc: https://docs.aws.amazon.com/apigateway/latest/developerguide/set-up-lambda-proxy-integrations.html#api-gateway-simple-proxy-for-lambda-input-format

    context: object, required
        Lambda Context runtime methods and attributes

        Context doc: https://docs.aws.amazon.com/lambda/latest/dg/python-context-object.html

    Returns
    ------
    API Gateway Lambda Proxy Output Format: dict

        Return doc: https://docs.aws.amazon.com/apigateway/latest/developerguide/set-up-lambda-proxy-integrations.html
    """

    def current_milli_time():
        return round(time.time() * 1000)

    client_timestream = boto3.client('timestream-write', region_name='us-east-1')

    def write_records(client):
        print("Writing records")

        sensor_data = json.loads(event["body"])

        dimensions = [
            {'Name': 'Sensor', 'Value': str(event["rawPath"])[22:-9]}
        ]

        sensor_wifi = {
            'Dimensions': dimensions,
            'MeasureName': 'SensorWifiStrength',
            'MeasureValue': str(sensor_data["wifi"]),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_milli_time())
        }

        sensor_pm02 = {
            'Dimensions': dimensions,
            'MeasureName': 'SensorPM02',
            'MeasureValue': str(sensor_data["pm02"]),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_milli_time())
        }

        sensor_rco2 = {
            'Dimensions': dimensions,
            'MeasureName': 'SensorCO2',
            'MeasureValue': str(sensor_data["rco2"]),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_milli_time())
        }

        sensor_atmp = {
            'Dimensions': dimensions,
            'MeasureName': 'SensorTemp',
            'MeasureValue': str(sensor_data["atmp"]),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_milli_time())
        }

        sensor_rhum = {
            'Dimensions': dimensions,
            'MeasureName': 'SensorHumidity',
            'MeasureValue': str(sensor_data["rhum"]),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_milli_time())
        }

        records = [sensor_wifi, sensor_pm02, sensor_rco2, sensor_atmp, sensor_rhum]

        try:
            result = client.write_records(
                DatabaseName="airgradient",
                TableName="sensors",
                Records=records,
                CommonAttributes={}
            )
            print("WriteRecords Status: [%s]" % result['ResponseMetadata']['HTTPStatusCode'])
        except client.exceptions.RejectedRecordsException as err:
            print("RejectedRecords: ", err)
            for rr in err.response["RejectedRecords"]:
                print("Rejected Index " + str(rr["RecordIndex"]) + ": " + rr["Reason"])
            print("Other records were written successfully. ")
        except Exception as err:
            print("Error:", err)

    write_records(client_timestream)

    sensor_data = json.loads(event["body"])

    return {
        "statusCode": 200,
        "body": json.dumps(
            {
                "SensorName": str(event["rawPath"])[21:-9],
                "SensorRawData": str(event["body"]),
                'SensorWifi': sensor_data["wifi"],
                'SensorPM02': sensor_data["pm02"],
                'SensorCO2': sensor_data["rco2"],
                'SensorTemp': sensor_data["atmp"],
                'SensorHumidity': sensor_data["rhum"],
            }
        ),
    }
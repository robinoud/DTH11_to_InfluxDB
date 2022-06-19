import influxdb_client
from liveplot import live_plotter
import numpy as np


import warnings
warnings.simplefilter('ignore')

def shiftArray(arr, num, fill_value=0):
    result = np.empty_like(arr)
    if num > 0:
        result[:num] = fill_value
        result[num:] = arr[:-num]
    elif num < 0:
        result[num:] = fill_value
        result[:num] = arr[-num:]
    else:
        result[:] = arr
    return result

bucket = "sensors"
org = "org"
token = "AcuAKA9cKDBFAm_Cu529ghGDpRL__fOnAb9rChWkRHhY0i3VCXnDPZUGjH25yIZp5mpZyOo7O1k31ikuT8jvxQ=="
# Store the URL of your InfluxDB instance
url = "http://localhost:8086"

client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)

query_api = client.query_api()
query = ' from(bucket:"sensors")\
|> range(start: -1000000m, stop: now())\
|> filter(fn:(r) => r._field == "temperature" )'

last_time = np.datetime64('2022-06-04 03:58:16.982449+00:00')
# initail frame plot
i = 0
size = 200
x_vec = np.arange(1, size+1, 1)
y_vec = np.empty(len(x_vec))
y_vec.fill(30)


line1 = []

result = client.query_api().query_data_frame(org=org, query=query)
for temp in result['_value'].tail(200):
    y_vec[i] = temp
    print(i,temp)
    i += 1
while True:
    result = client.query_api().query_data_frame(org=org, query=query)
    #print(len(result))
    current_time = np.datetime64(result['_time'].tail(1).to_list()[0])
    if last_time != current_time:
        y_vec = shiftArray(y_vec,-1)
        y_vec[-1] = np.array(result['_value'].tail(1))
        last_time = current_time


    line1 = live_plotter(x_vec, y_vec, line1)
    #time.sleep(1)

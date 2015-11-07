#!/usr/bin/python

import serial
import datetime
import csv
import urllib

WHEEL_DIST = 0.94248  # circumference of a typical bucket wheel
TEMP_MIN = 21
TEMP_MAX = 27
BAUD_RATE = 9600
PORT_NAME = '/dev/ttyACM0'
LOG_FILE = 'hogswatch.csv'

input_port = None
dist_sum = 0
ts_general = None
ts0 = datetime.datetime.now()
ts1 = None
flag_tweeted = 0
flag_tweeted_dist = 0


def send_text(input_data):
    url = "http://www.bulksms.co.uk:5567/eapi/submission/send_sms/2/2.0"

    warning_msg = ' '.join(['Temperature out of bounds in ',
                            input_data[0].upper(),
                            '\'s viv! Temperature is:',
                            str(input_data[2])])

    param = urllib.urlencode({'username': 'toolverine',
                              'password': 'hedgehog',
                              'message': warning_msg,
                              'msisdn': 447961388940})

    f = urllib.urlopen(url, param)
    s = f.read()
    result = s.split('|')
    statusCode = result[0]
    statusString = result[1]
    f.close()

    return statusCode, statusString


def is_unsuitable(temp):
    result = temp < TEMP_MIN or temp > TEMP_MAX
    return result


try:
    input_port = serial.Serial(PORT_NAME, BAUD_RATE)
    input_port.open()

    log_file = open(LOG_FILE, 'a')
    lf_writer = csv.writer(log_file)

except:
    print 'Error during initialisation'
    raise SystemExit

while True:
    try:
        input_data = input_port.readline()
        input_data = input_data.split(':')

        ts_general = datetime.datetime.now()

        if input_data[1] == 'Temp' and is_unsuitable(float(input_data[2])):
            sc, ss = send_text(input_data)
            if sc != '0':
                log_msg = str(ts_general), ss
            else:
                log_msg = str(ts_general), 'Temp warning msg sent'
            lf_writer.writerow(log_msg)

        if input_data[1] == 'Dist' and input_data[2] != 0:
            dist_sum += input_data[2]
            if flag_tweeted == 1:
                pass
            else:
                ts1 = datetime.datetime.now()

        if input_data[1] == 'Dist' and input_data[2] == 0:
            if flag_tweeted == 1:
                ts0 = datetime.datetime.now()
                flag_tweeted = 0

        time_hr = ts_general.hour

        if time_hr == 9 and flag_tweeted_dist == 0:
            dist_sum = 0
            flag_tweeted_dist = 1
        elif time_hr != 9 and flag_tweeted_dist == 1:
            flag_tweeted_dist = 0

        lf_writer.writerow([ts_general, input_data])

    except:
        pass

input_port.close()
log_file.close()

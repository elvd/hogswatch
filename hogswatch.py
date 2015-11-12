#!/usr/bin/python
#
# Hogswatch - African Pygmy Hedgehog vivarium monitor
# ===================================================
#
# Python script, meant to run on a Raspberry Pi and complement the Arduino
# sketch collecting temperature, humidity, and wheel activity information.
#
# Logs all received data to a csv file, and sends text warnings to mobile phone
# in case of a low or high temperature in either vivarium. Also calculates
# distance ran from number of revolutions received.
#
# Communication with the Arduino is over serial link, using a SparkFun level
# converter to go between 5 V (Arduino) and 3.3 V (RPi) logic.
#
# TODO: Add function to push data to Xively.
# TODO: Move to `requests' instead of `urllib'
# TODO: Move usernames/passwords/API to JSON file
# TODO: Use `logging' module for service messages
#
# Author: Viktor Doychinov (v.o.doychinov@gmail.com)
#


import serial
import datetime
import csv
import urllib
from collections import namedtuple

wheel_dist = 0.94248  # circumference of a typical bucket wheel

# as recommended by African Pygmy Hedgehog breeders and keepers
temp_min = 21
temp_max = 27

baud_rate = 9600
arduino_port = '/dev/ttyAMA0'
logfile_name = 'hogswatch.csv'
url = 'http://www.bulksms.co.uk:5567/eapi/submission/send_sms/2/2.0'
input_port = None
timestamp = None
data = namedtuple('data', 'hedgehog param value')  # for better readability

print('Variables defined successfully')


def send_text(input_data):
    warning_msg = ' '.join(['Temperature out of bounds in ',
                            input_data.hedgehog.upper(),
                            '\'s viv! Temperature is:',
                            str(input_data.value)])

    param = urllib.urlencode({'username': 'toolverine',
                              'password': 'hedgehog',
                              'message': warning_msg,
                              'msisdn': 447961388940})

    f = urllib.urlopen(url, param)
    s = f.read()
    result = s.split('|')
    status_code = result[0]
    status_string = result[1]
    f.close()

    return status_code, status_string


def is_unsuitable(temp):
    return temp < temp_min or temp > temp_max


try:
    input_port = serial.Serial(arduino_port, baud_rate)
    input_port.open()

    logfile = open(logfile_name, 'a')
    lf_writer = csv.writer(logfile)

except:
    print('Error during initialisation')
    raise SystemExit

print('Serial port opened successfully')

# in case of RPi reboot, Arduino might have sent data previously - discard it
input_port.flushInput()

while True:
    try:
        input_data = input_port.readline()
        input_data = input_data.strip()
        input_data = input_data.split(':')
        input_data = data(*input_data)

        timestamp = datetime.datetime.now()

        if input_data.param == 'temp' and \
           is_unsuitable(float(input_data.value)):  # TODO: add a timer
            # sc, ss = send_text(input_data)
            # if sc != '0':
            #     log_msg = str(timestamp), ss
            # else:
            #     log_msg = str(timestamp), 'Temperature warning message sent'
            log_msg = [timestamp, 'Temperature warning']
            lf_writer.writerow(log_msg)

        if input_data.param == 'dist':
            actual_dist = float(input_data.value) * wheel_dist
            input_data = input_data._replace(value=actual_dist)

        lf_writer.writerow([timestamp, input_data.hedgehog,
                            input_data.param, input_data.value])
        logfile.flush()

    except:
        pass

input_port.close()
logfile.close()

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
# TODO: Save data to database instead of csv
#
# Author: Viktor Doychinov (v.o.doychinov@gmail.com)
#


import serial
import signal
import datetime
import csv
import json
import requests
import logging
import logging.handlers
import sys
from collections import namedtuple


wheel_dist = 0.94248  # circumference of a typical bucket wheel
temp_min = 21  # as recommended by African Pygmy Hedgehog breeders and keepers
temp_max = 27

# Arduino serial port configuration
baud_rate = 9600
arduino_port = '/dev/ttyAMA0'

data = namedtuple('data', 'hedgehog param value')  # for better readability
logfile_name = 'hogswatch.log'  # used by logging module
config_name = 'hogswatch.json'  # holds usernames, passwords, API keys
datafile_name = 'hogswatch.csv'  # until we move to sqlite

input_port = None
timestamp = None
timestamp_sms = None
feed_data = dict()

#  set up logger, following recipe in Logging Cookbook
hogswatch_logger = logging.getLogger(name='hogswatch')
hogswatch_logger.setLevel(logging.DEBUG)
handler = logging.handlers.RotatingFileHandler(logfile_name, maxBytes=1048576,
                                               backupCount=5)
formatter = logging.Formatter('%(asctime)s %(name)s %(levelname)s %(message)s')
handler.setFormatter(formatter)
hogswatch_logger.addHandler(handler)


def sigterm_handler(signum, frame):
    """Responds to kill commands from OS"""
    hogswatch_logger.info('Received signal %s from OS', str(signum))
    hogswatch_logger.info('Commencing shutdown')

    if input_port.isOpen():
        input_port.close()
        hogswatch_logger.info('Serial port closed')

    if not datafile.closed:
        datafile.close()
        hogswatch_logger.info('CSV file closed')

    logging.shutdown()
    sys.exit()


def send_text(input_data):
    """
    Sends a warning text message to a mobile number

    Forms the message to be sent from data in a namedtuple. Message text
    contains which hedgehog is affected, and what is the current temperature in
    their viv.

    Uses bulksms.co.uk to send the text. Parameters for the HTTP POST request
    are read from the configuration JSON file.
    """
    warning_msg = ' '.join(['Temperature out of bounds in ',
                            input_data.hedgehog.upper(),
                            '\'s viv! Temperature is:',
                            str(input_data.value)])

    hogswatch_config['sms_params']['message'] = warning_msg

    sms_response = requests.post(hogswatch_config['sms_base_url'],
                                 data=hogswatch_config['sms_params'])

    status_code, status_string, _ = sms_response.content.strip().split('|')

    return status_code, status_string


def is_unsuitable(temp):
    """Checks whether a measured temeperature is out of safe bounds"""
    return temp < temp_min or temp > temp_max


def publish_to_thingspeak(hedgehog_data):
    """
    Publishes sensor data to ThingSpeak.com

    In addition to saving data on distance ran, temperature, and humidity in
    hedgehogs' vivariums to a local fila; the data is published to two channels
    on ThingSpeak.com, one for each hedgehog.

    API keys and other configuration data are also stored in JSON file.
    """
    feed_data['api_key'] = hogswatch_config[hedgehog_data.hedgehog]

    if hedgehog_data.param == 'dist':
        feed_data['field1'] = hedgehog_data.value
    elif hedgehog_data.param == 'temp':
        feed_data['field2'] = hedgehog_data.value
    else:
        feed_data['field3'] = hedgehog_data.value

    publish_response = requests.post(hogswatch_config['data_log_url'],
                                     data=feed_data)
    feed_data.clear()

    return publish_response

signal.signal(signal.SIGTERM, sigterm_handler)  # register our handler

try:
    # initialise serial port communication with Arduino
    input_port = serial.Serial(arduino_port, baud_rate)
    input_port.open()
    input_port.flushInput()  # discards any previous data

    # load configuration variables from JSON
    with open(config_name, 'r') as config_file:
        hogswatch_config = json.load(config_file)
except serial.SerialException:
    hogswatch_logger.error('Unable to initiate serial communication',
                           exc_info=True)
    logging.shutdown()
    sys.exit()
except EnvironmentError:
    hogswatch_logger.error('Unable to load configuration file', exc_info=True)
    logging.shutdown()
    sys.exit()
else:
    hogswatch_logger.info('Configuration file loaded successfully')
    hogswatch_logger.info('System setup completed successfully')

try:
    # open csv file to store sensor data
    datafile = open(datafile_name, 'a')
    lf_writer = csv.writer(datafile)
except EnvironmentError:
    hogswatch_logger.error('Unable to open data file', exc_info=True)
    logging.shutdown()
    sys.exit()
else:
    hogswatch_logger.info('Data file opened successfully')

hogswatch_running = True

while hogswatch_running:
    try:
        input_data = input_port.readline()
        input_data = input_data.strip().split(':')
        input_data = data(*input_data)

        timestamp = datetime.datetime.now()

        # check if temperature is ok
        if input_data.param == 'temp' and \
           is_unsuitable(float(input_data.value)):

            hogswatch_logger.warning('Temperature outside safe limits')

            if (timestamp_sms is None) or \
               ((timestamp - timestamp_sms).total_seconds() >=
               hogswatch_config["temp_warning_period"]):
                sc, ss = send_text(input_data)
                timestamp_sms = datetime.datetime.now()
                hogswatch_logger.info('Attempted SMS send, response: %s %s',
                                      sc, ss)

        # convert revolutions to distance
        if input_data.param == 'dist':
            actual_dist = float(input_data.value) * wheel_dist
            input_data = input_data._replace(value=actual_dist)

        # save datapoint to file
        lf_writer.writerow([timestamp, input_data.hedgehog,
                            input_data.param, input_data.value])
        datafile.flush()  # write each received datapoint

        # publish to the internet for the whole world to see
        publish_response = publish_to_thingspeak(input_data)
        if int(publish_response.content) == 0:
            hogswatch_logger.error('ThingSpeak update error')

    except KeyboardInterrupt:
        hogswatch_running = False
        hogswatch_logger.info('Received Ctrl-C, shutting down')

else:
    # clean up after us
    input_port.close()
    hogswatch_logger.info('Serial port closed')

    datafile.close()
    hogswatch_logger.info('CSV file closed')

    hogswatch_logger.info('Files and ports closed, terminating')
    logging.shutdown()

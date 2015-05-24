#!/usr/bin/python

import serial
import datetime
import csv
import urllib
import xively
import twitter

WHEEL_DIST = 0.94248  # circumference of a typical bucket wheel
TWEET_TO = 900  # tweet every 15 minutes
TEMP_MIN = 21
TEMP_MAX = 27
BAUD_RATE = 9600
XIVELY_FEED_ID = 890448892
XIVELY_API_KEY = 'JV3s8zXnTR2s9bUeN6BPvhX8iF7W8Ew4hjvxH0ofjVsji7Y5'
TWITTER_API_KEY = 'nRpyRDH5kNOrdnr5aKXaf89C0'
TWITTER_API_SECRET = 'fl2PO0NhPuwWo8S1Jetd0JOscuedefs7NbVXraFcJCOXXWKi2A'
TWITTER_OAUTH_TOKEN = '2350375916-7S5Tr2GL832iPCYptOlCXUVS2hATeis8oGRuEmz'
TWITTER_OAUTH_SECRET = 'hyNUq8lm7VnrOdZbANe55u6TrRjsk5VPNbGRYCOqhE9mI'
PORT_NAME = '/dev/ttyACM0'
LOG_FILE = 'hogwatch.log'

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


def send_tweet(feed, mode_code, revs=0):
    if mode_code == 'start':
        msg = 'It\'s wheeling time for me! :)'
    elif mode_code == 'dist':
        dist_ran = revs * WHEEL_DIST
        msg = ' '.join(['Oh my, I ran ', str(dist_ran), ' metres last night!'])

    feed.statuses.update(status=msg)


def publish_to_xively(feed, data, log_time):
    stream_id = data[0] + data[1]
    if data[1] == 'Dist':
        datapoint = data[2] * WHEEL_DIST
    else:
        datapoint = data[2]
    feed.datastreams = xively.Datastream(id=stream_id,
                                         current_value=datapoint,
                                         at=log_time)

    feed.update()

try:
    input_port = serial.Serial(PORT_NAME, BAUD_RATE)
    input_port.open()

    log_file = open(LOG_FILE, 'a')
    lf_writer = csv.writer(log_file)

    xively_api = xively.XivelyAPIClient(XIVELY_API_KEY)
    xively_feed = xively_api.feeds.get(XIVELY_FEED_ID)

    twitter_feed = twitter.Twitter(auth=twitter.OAuth(TWITTER_OAUTH_TOKEN,
                                                      TWITTER_OAUTH_SECRET,
                                                      TWITTER_API_KEY,
                                                      TWITTER_API_SECRET))

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
                if (ts1 - ts0).total_seconds() >= TWEET_TO:
                    send_tweet(twitter_feed, 'start')
                    flag_tweeted = 1

        if input_data[1] == 'Dist' and input_data[2] == 0:
            if flag_tweeted == 1:
                ts0 = datetime.datetime.now()
                flag_tweeted = 0

        time_hr = ts_general.hour

        if time_hr == 9 and flag_tweeted_dist == 0:
            send_tweet(twitter_feed, 'dist', dist_sum)
            dist_sum = 0
            flag_tweeted_dist = 1
        elif time_hr != 9 and flag_tweeted_dist == 1:
            flag_tweeted_dist = 0

        publish_to_xively(xively_feed, input_data, ts_general)
        lf_writer.writerow([ts_general, input_data])

    except:
        pass

input_port.close()
log_file.close()

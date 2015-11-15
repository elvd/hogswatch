/*
** Hogswatch - African Pygmy Hedgehog vivarium monitor
** ===================================================
**
** Continuously monitors vivarium temperature, humidity, and wheel activity for
** two African Pygmy Hedgehogs, Caesar and Tzaritza (Ritz).
**
** A DHT 22 sensor provides temperature and humidity data. Adafruit's library
** is used for communication with the sensors.
**
** A combination of magnets, attached to the hedgehogs' wheels and Hall sensors
** are used to detect wheel rotation and count number of revolutions.
**
** Data is transmitted through a serial link to a Raspberry Pi for further
** processing, logging, and uploading to the Internet.
**
** Author: Viktor Doychinov (v.o.doychinov@gmail.com)
**
*/

#include <LiquidCrystal.h> // for debugging and testing purposes
#include <SoftwareSerial.h> // RPi communication, using dumb serial mode
#include "DHT.h" // Adafruit library for DHT22 sensor

// LCD pins, with corresponding pins on LCD board
#define D7 13 // 14
#define D6 12 // 13
#define D5 11 // 12
#define D4 10 // 11
#define E 9   // 6
#define RS 8  // 4
#define COLS 16
#define ROWS 2

// Hall sensor input pins
#define CAESARHALL1 6
#define CAESARHALL2 7
#define RITZHALL1 2
#define RITZHALL2 3

// Raspberry Pi communication serial pins
#define PIRX A1
#define PITX A0

// DHT sensor configuration
#define DHTTYPE DHT22
#define CDHTPIN A3
#define RDHTPIN A5

// Raspberry Pi TX time intervals, in ms
#define TXREVS 60000
#define TXTEMP 60000
#define TXHUMD 60000
#define TXOFFSET 20000 // for ThingSpeak updates

// Update LCD display every second.
#define UPDATEPERIOD 1000

// for detecting direction and number of revolutions
int c_flag1 = 0, c_flag2 = 0, r_flag1 = 0, r_flag2 = 0;
int hall_value1, hall_value2;
unsigned long c_revolutions = 0, r_revolutions = 0;
unsigned long timer_revs = 0, timer_temp = 0, timer_update = 0, timer_humidity = 0;
float temp = 0.0, humidity = 0.0;

// initialise the separate components/libraries
SoftwareSerial pi_port(PIRX, PITX) ;
LiquidCrystal stat_disp(RS, E, D4, D5, D6, D7);
DHT c_temp_sensor(CDHTPIN, DHTTYPE);
DHT r_temp_sensor(RDHTPIN, DHTTYPE);

void setup() {
  pinMode(CAESARHALL1, INPUT);
  pinMode(CAESARHALL2, INPUT);
  pinMode(RITZHALL1, INPUT);
  pinMode(RITZHALL2, INPUT);

  pi_port.begin(9600);
//  Serial.begin(9600);
  c_temp_sensor.begin();
  r_temp_sensor.begin();

  stat_disp.begin(COLS, ROWS);
  stat_disp.print("Hello Kate!"); // My lovely girlfriend and best hog mummy
  stat_disp.setCursor(0, 1);
  stat_disp.print("Huff Huff");

  delay(5000);
  stat_disp.clear();

// Get current time
  timer_temp = millis();
  timer_humidity = millis() + TXOFFSET;
  timer_revs = millis() + 2*TXOFFSET;
  timer_update = millis();
}

void loop() {

  if ((millis() - timer_revs) >= TXREVS) { // send number of revolutions to RPi
    pi_port.print("caesar:dist:");
    pi_port.println(c_revolutions);
    //    Serial.print("caesar:dist:");
    //    Serial.println(c_revolutions);

    pi_port.print("ritz:dist:");
    pi_port.println(r_revolutions);
    //    Serial.print("ritz:dist:");
    //    Serial.println(r_revolutions);

    c_revolutions = 0;
    r_revolutions = 0;
    timer_revs = millis();
  }

  if ((millis() - timer_temp) >= TXTEMP) { // send temperature to RPi
    temp = c_temp_sensor.readTemperature();
    pi_port.print("caesar:temp:");
    pi_port.println(temp);
//    Serial.print("caesar:temp:");
//    Serial.println(temp);

    temp = r_temp_sensor.readTemperature();
    pi_port.print("ritz:temp:");
    pi_port.println(temp);
//    Serial.print("ritz:temp:");
//    Serial.println(temp);

    timer_temp = millis();
  }

  if ((millis() - timer_humidity) >= TXHUMD) { // send humidity to RPi
    humidity = c_temp_sensor.readHumidity();
    pi_port.print("caesar:humidity:");
    pi_port.println(humidity);
    //    Serial.print("caesar:humidity:");
    //    Serial.println(humidity);

    humidity = r_temp_sensor.readHumidity();
    pi_port.print("ritz:humidity:");
    pi_port.println(humidity);
    //    Serial.print("ritz:humidity:");
    //    Serial.println(humidity);

    timer_humidity = millis();
  }

  if ((millis() - timer_update) >= UPDATEPERIOD) {
    update_display();
    timer_update = millis();
  }

  check_caesar();
  check_ritz();
}

void update_display() {
  stat_disp.clear();

  stat_disp.setCursor(0, 0);
  stat_disp.print("CSR REV:");
  stat_disp.print(c_revolutions);

  stat_disp.setCursor(0, 1);
  stat_disp.print("TZA REV:");
  stat_disp.print(r_revolutions);
}

/*
** Both check_ritz() and check_caesar() use the same algorithm, just different
** sensors. If any Hall sensor detectes LOW, i.e. magnetic field is present, a
** flag is set. If then the next Hall sensor also registers a magnetic field,
** the number of wheel rotations for the specific hedgehog is increased by one.
** Assumption is that said magnetic field is caused by the magnets attached to
** their wheels.
**
** For more information, look at accompanying sketch.
**
**
*/

void check_caesar() {
  hall_value1 = digitalRead(CAESARHALL1);
  hall_value2 = digitalRead(CAESARHALL2);

  if (hall_value1 == LOW) {
    if (c_flag1 == 0 && c_flag2 == 0) c_flag1 = 1;
    else if (c_flag2 == 1) {
      c_revolutions = c_revolutions + 1;
      c_flag2 = 0; }
    else if (c_flag1 == 1) c_flag1 = 0;
    delay(250); // TODO: use a different debounce method
  }

  if (hall_value2 == LOW) {
    if (c_flag1 == 0 && c_flag2 == 0) c_flag2 = 1;
    else if (c_flag1 == 1) {
      c_revolutions = c_revolutions + 1;
      c_flag1 = 0; }
    else if (c_flag2 == 1) c_flag2 = 0;
    delay(250);
  }
}

void check_ritz() {
  hall_value1 = digitalRead(RITZHALL1);
  hall_value2 = digitalRead(RITZHALL2);

  if (hall_value1 == LOW) {
    if (r_flag1 == 0 && r_flag2 == 0) r_flag1 = 1;
    else if (r_flag2 == 1) {
      r_revolutions = r_revolutions + 1;
      r_flag2 = 0; }
    else if (r_flag1 == 1) r_flag1 = 0;
    delay(250); // TODO: use a different debounce method
  }

  if (hall_value2 == LOW) {
    if (r_flag1 == 0 && r_flag2 == 0) r_flag2 = 1;
    else if (r_flag1 == 1) {
      r_revolutions = r_revolutions + 1;
      r_flag1 = 0; }
    else if (r_flag2 == 1) r_flag2 = 0;
    delay(250);
  }
}

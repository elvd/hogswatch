/*
** Hog's watch - Hedgehog vivarium monitor
** 
** Uses a rare-earth magnet and a hall effect sensor to detect
** and count wheel spins, and estimate total distance run.
** 
** Reads temperature data from a DHT22 sensor.
** 
** Sends data to a Raspberry Pi over serial port connection.
** 
**
*/

#include <LiquidCrystal.h> // for debugging and testing purposes
// #include <SoftwareSerial.h> // Xbee communication, using dumb serial mode
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
// #define PIRX 4
// #define PITX 5

// DHT sensor pins
#define DHTTYPE DHT22
#define CDHTPIN A3
#define RDHTPIN A5

// Raspberry Pi TX periods
#define TXREVS 60000
#define TXTEMP 60000
#define RESETPERIOD 86400000
#define UPDATEPERIOD 60000

int hall_value1, hall_value2;
int c_flag1 = 0, c_flag2 = 0, r_flag1 = 0, r_flag2 = 0; // for detecting direction and number of revolutions
unsigned long c_revolutions = 0, r_revolutions = 0;
unsigned long timer_revs = 0, timer_temp = 0, timer_start = 0, timer_update = 0;
float temp = 0.0, humidity = 0.0;

// initialise the separate components/libraries
// SoftwareSerial pi_port(PIRX, PITX) ;
LiquidCrystal stat_disp(RS, E, D4, D5, D6, D7);
DHT c_temp_sensor(CDHTPIN, DHTTYPE);
DHT r_temp_sensor(RDHTPIN, DHTTYPE);

void setup() {
  pinMode(CAESARHALL1, INPUT);
  pinMode(CAESARHALL2, INPUT);
  pinMode(RITZHALL1, INPUT);
  pinMode(RITZHALL2, INPUT);
  
//  Pi_port.begin(9600);
  Serial.begin(9600);
  c_temp_sensor.begin();
  r_temp_sensor.begin();
   
  stat_disp.begin(COLS, ROWS);
  stat_disp.print("Hello Kate!");
  stat_disp.setCursor(0, 1);
  stat_disp.print("Huff Huff");
  
  delay(5000);
  stat_disp.clear();
  
// Get current time  
  timer_temp = millis();
  timer_revs = millis();
  timer_start = millis();
  timer_update = millis();
}

void loop() {

  if ((millis() - timer_revs) >= TXREVS) {
//    Pi_port.print("caesar:dist:");
//    Pi_port.print(c_revolutions);
//    Pi_port.println();

//    Pi_port.print("ritz:dist:");
//    Pi_port.print(r_revolutions);
//    Pi_port.println();

    Serial.print("caesar:dist:");
    Serial.println(c_revolutions);

    Serial.print("ritz:dist:");
    Serial.print(r_revolutions);

    timer_revs = millis();
  }
  
  if ((millis() - timer_temp) >= TXTEMP) {
    temp = c_temp_sensor.readTemperature();
    humidity = c_temp_sensor.readHumidity();
//    Pi_port.print("caesar:temp:");
//    Pi_port.print(temp);
//    Pi_port.println();
    Serial.print("caesar:temp:");
    Serial.println(temp);
    Serial.print("caesar:humidity:");
    Serial.println(humidity);

    temp = r_temp_sensor.readTemperature();
    humidity = r_temp_sensor.readHumidity();
//    Pi_port.print("ritz:temp:");
//    Pi_port.print(temp);
//    Pi_port.println();
    Serial.print("ritz:temp:");
    Serial.println(temp);
    Serial.print("ritz:humidity:");
    Serial.println(humidity);

    timer_temp = millis();
  }
  
  if ((millis() - timer_start) >= RESETPERIOD) {
    c_revolutions = 0;
    r_revolutions = 0;
    timer_start = millis();
  }
  
  if ((millis() - timer_update) >= UPDATEPERIOD) {
    update_display();
    timer_update = millis();
  }
  
  check_caesar();
  Serial.print("c_flag1:");
  Serial.println(c_flag1);
  Serial.print("c_flag2:");
  Serial.println(c_flag2);
  check_ritz();
  Serial.print("r_flag1:");
  Serial.println(r_flag1);
  Serial.print("r_flag2:");
  Serial.println(r_flag2);
}

void update_display() {
  stat_disp.clear();
  
  stat_disp.setCursor(0, 0);
  stat_disp.print("CSR REV:");
  stat_disp.print(c_revolutions);

  stat_disp.setCursor(0, 1);
  stat_disp.print("RTZ REV:");
  stat_disp.print(r_revolutions);
}

void check_caesar() { 
  hall_value1 = digitalRead(CAESARHALL1);
  hall_value2 = digitalRead(CAESARHALL2);

  if (hall_value1 == LOW) {
    if (c_flag1 == 0 && c_flag2 == 0) c_flag1 = 1;
    else if (c_flag2 == 1) {
      c_revolutions = c_revolutions + 1;
      c_flag2 = 0; }
    else if (c_flag1 == 1) c_flag1 = 0;
    delay(250);
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
    delay(250);
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


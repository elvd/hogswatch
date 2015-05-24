/*
** Hog's watch - Hedgehog vivarium monitor
** 
** Uses a rare-earth magnet and a hall effect sensor to detect
** and count wheel spins, and estimate total distance run.
** 
** Reads temperature data from a DHT22 sensor.
** 
** Sends data to a Raspberry Pi over Xbee link.
** 
**
*/

#include <LiquidCrystal.h> // for debugging and testing purposes
#include <SoftwareSerial.h> // Xbee communication, using dumb serial mode
#include "DHT.h" // Adafruit library for DHT22 sensor

// LCD pins, with corresponding pins on LCD board
#define pinD7 5 // 14
#define pinD6 4 // 13
#define pinD5 3 // 12
#define pinD4 2 // 11
#define pinE 9  // 6
#define pinRS 8 // 4
#define COLS 16
#define ROWS 2

// Hall sensor input pins
#define pinHall1 A1
#define pinHall2 A4

// Raspberry Pi communication serial ports
#define PiRX 12
#define PiTX 13

// DHT sensor ports
#define DHTTYPE DHT22
#define C_DHTPIN 7
#define R_DHTPIN 10

// Raspberry Pi TX periods
#define TX_revs 600000
#define TX_temp 300000

int hallValue1, hallValue2 ;
int flag1 = 0, flag2 = 0 ; // for detecting direction and number of revolutions
unsigned long revolutions = 0 ;
unsigned long timer_revs = 0, timer_temp = 0 ;
float temp = 0.0 ;

// initialise the separate components/libraries
LiquidCrystal stat_disp(pinRS, pinE, pinD4, pinD5, pinD6, pinD7) ;
SoftwareSerial Pi_port(PiRX, PiTX) ;
DHT C_temp_sensor(C_DHTPIN, DHTTYPE) ;
DHT R_temp_sensor(R_DHTPIN, DHTTYPE) ;

void setup() {
  pinMode(pinHall1, INPUT) ;
  pinMode(pinHall2, INPUT) ;
  
  Pi_port.begin(9600) ;
  C_temp_sensor.begin() ;
  R_temp_sensor.begin() ;
   
  stat_disp.begin(COLS, ROWS) ;
  stat_disp.print("Hello Kate!") ;
  stat_disp.setCursor(0, 1) ;
  stat_disp.print("Huff Huff") ;
  
  timer_temp = millis() ;
  timer_revs = millis() ;
}

void loop() {

  if ((millis() - timer_revs) >= TX_revs) {
    Pi_port.print("caesar:Dist:") ;
    Pi_port.print(revolutions) ;
    Pi_port.println() ;
    revolutions = 0 ;
    timer_revs = millis() ;
  }
  
  if ((millis() - timer_temp) >= TX_temp) {
    temp = C_temp_sensor.readTemperature() ;
    Pi_port.print("caesar:Temp:") ;
    Pi_port.print(temp) ;
    Pi_port.println() ;
    temp = R_temp_sensor.readTemperature() ;
    Pi_port.print("ritz:Temp:") ;
    Pi_port.print(temp) ;
    Pi_port.println() ;
    timer_temp = millis() ;
  }
  
  stat_disp.setCursor(0, 1);
  
  hallValue1 = digitalRead(pinHall1) ;
  hallValue2 = digitalRead(pinHall2) ;

// old code not taking into account direction
//  if (hallValue1 == LOW || hallValue2 == LOW) {
//	revolutions = revolutions + 1 ;
//     	stat_disp.print(revolutions) ;
//        delay(100) ;	
//  }

  if (hallValue1 == LOW) {
    if (flag1 == 0 && flag2 == 0) flag1 = 1 ;
    else if (flag2 == 1) {
      revolutions = revolutions + 1 ;
      flag2 = 0 ; }
    else if (flag1 == 1) flag1 = 0 ;
    delay(500) ;
    Pi_port.print("Hall sensor 1: ") ;
    Pi_port.print(flag1) ;
    Pi_port.print(flag2) ;
    Pi_port.println() ;
  }
  
  if (hallValue2 == LOW) {
    if (flag1 == 0 && flag2 == 0) flag2 = 1 ;
    else if (flag1 == 1) {
      revolutions = revolutions + 1 ;
      flag1 = 0 ; }
    else if (flag2 == 1) flag2 = 0 ;
    delay(500) ;
    Pi_port.print("Hall sensor 2: ") ;
    Pi_port.print(flag1) ;
    Pi_port.print(flag2) ;
    Pi_port.println() ;
  }
  
}

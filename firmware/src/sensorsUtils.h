#ifndef SENSORSUTILS_H
#define SENSORSUTILS_H

#include <Arduino.h>
#include <QMC5883LCompass.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Ultrasonic.h>
#include <Preferences.h>
#include "motorsUtils.h"

const int SDA_PIN = 21;  
const int SCL_PIN = 22;

extern Preferences prefs;


extern QMC5883LCompass compass;

extern const uint8_t gpsLed;
extern const int echoPin;
extern const int triggerPin;
extern float speed;
extern double lat;
extern double lon;



extern TinyGPSPlus gps;
extern HardwareSerial SerialGPS;
extern Ultrasonic ultrasonic;

void startCompass();
float readCompass();
void initSensors();
void readGPSInfo();
void updateGpsLed();
float getUltrasonic(); 
void compassCalibrate();
#endif
#include "sensorsUtils.h"

float azimuth;
QMC5883LCompass compass;

const uint8_t gpsLed = 26;
const int echoPin = 15;
const int triggerPin=4;

float speed = 0.0f;
double lat = 0;
double lon = 0;

TinyGPSPlus gps;
HardwareSerial SerialGPS(2); 
Ultrasonic ultrasonic(triggerPin,echoPin);

const float declinacion = -9.87;
const float orientationOffset = -65;

const uint8_t RX_GPS = 16;
const uint8_t TX_GPS = 17;

void startCompass(){
    Wire.begin(SDA_PIN,SCL_PIN);
    compass.init();
    compass.setCalibrationOffsets(-364.00, 1134.00, -715.00);
    compass.setCalibrationScales(0.93, 0.85, 1.34);
}

float readCompass(){
    compass.read();
    float orientation = fmod(compass.getAzimuth() + orientationOffset + declinacion + 360.0, 360.0);
    return orientation;
}

void initSensors(){
    startCompass();
    pinMode(gpsLed, OUTPUT);

    SerialGPS.begin(9600, SERIAL_8N1, RX_GPS, TX_GPS);
}

void readGPSInfo() {
    while (SerialGPS.available() > 0) {
        gps.encode(SerialGPS.read());
        if (gps.location.isUpdated()) {
            lat = gps.location.lat();
            lon = gps.location.lng();
            speed = gps.speed.mps();
        }
    }
}

void updateGpsLed() {
    if (gps.satellites.value() > 0 && gps.location.isValid()) {
        digitalWrite(gpsLed, HIGH);
    } else {
        digitalWrite(gpsLed, LOW);
    }
}

float getUltrasonic(){
    const int N = 5;
    float sum = 0;
    delay(50);
    for (int i = 0; i < N; i++) {
      sum += ultrasonic.read();  
      delay(10);               
    }
    return sum / N;
}
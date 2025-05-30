#include "sensorsUtils.h"

float azimuth;
QMC5883LCompass compass;
Preferences prefs;

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

double offsetx=0;
double offsety=0;
double offsetz=0;

double scalex=1;
double scaley=1;
double scalez=1;

const uint8_t RX_GPS = 16;
const uint8_t TX_GPS = 17;

void readCalibrationValues();

void startCompass(){
    Wire.begin(SDA_PIN,SCL_PIN);
    compass.init();
    readCalibrationValues();
    compass.setCalibrationOffsets(offsetx, offsety, offsetz);
    compass.setCalibrationScales(scalex, scaley, scalez);

    
}

void saveCalibrationValues(){
  prefs.begin("calibracion", false);  
  prefs.putDouble("offsetx", offsetx);
  prefs.putDouble("offsety", offsety);
  prefs.putDouble("offsetz", offsetz);
  prefs.putDouble("scalex", scalex);
  prefs.putDouble("scaley", scaley);
  prefs.putDouble("scalez", scalez);
  prefs.end();

}

void readCalibrationValues(){
    prefs.begin("calibracion", true);
    offsetx = prefs.getDouble("offsetx", 0.0);
    offsety = prefs.getDouble("offsety", 0.0);
    offsetz = prefs.getDouble("offsetz", 0.0);

    scalex = prefs.getDouble("scalex", 1.0);
    scaley = prefs.getDouble("scaley", 1.0);
    scalez = prefs.getDouble("scalez", 1.0);
    Serial.printf("Calibration offsets: (%f,%f,%f) scales: (%f,%f,%f) \n",offsetx,offsety,offsetz,scalex,scaley,scalez);
    prefs.end();

}
float readCompass(){
    compass.read();
    float orientation = fmod(compass.getAzimuth()+270,360.0f);
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
void compassCalibrate() {   
  calibrationTurn();
  compass.calibrate(); 
  stopCar();
  offsetx=compass.getCalibrationOffset(0);
  offsety=compass.getCalibrationOffset(1);
  offsetz=compass.getCalibrationOffset(2);
  scalex=compass.getCalibrationScale(0);
  scaley=compass.getCalibrationScale(1);
  scalez=compass.getCalibrationScale(2);
  saveCalibrationValues();
  ESP.restart();
}
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

double offsetx=0;
double offsety=0;
double offsetz=0;

double scalex=1;
double scaley=1;
double scalez=1;

const uint8_t RX_GPS = 16;
const uint8_t TX_GPS = 17;

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
    prefs.end();

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
void compassCalibrate() {
  Serial.println("Comenzando calibración...");
  Serial.println("Mueve el sensor en todas las direcciones durante 10 segundos");

  long t0 = millis();
  int16_t x, y, z;
  int16_t minX = 32767, minY = 32767, minZ = 32767;
  int16_t maxX = -32768, maxY = -32768, maxZ = -32768;

  while (millis() - t0 < 10000) {  // 10 segundos
    compass.read();
    x = compass.getX();
    y = compass.getY();
    z = compass.getZ();

    if (x < minX) minX = x;
    if (y < minY) minY = y;
    if (z < minZ) minZ = z;

    if (x > maxX) maxX = x;
    if (y > maxY) maxY = y;
    if (z > maxZ) maxZ = z;

    delay(100);
  }

  offsetx= (maxX + minX) / 2.0;
  offsety = (maxY + minY) / 2.0;
  offsetz = (maxZ + minZ) / 2.0;

  double avgDelta = ((maxX - minX) + (maxY - minY) + (maxZ - minZ)) / 3.0;
  scalex = avgDelta / (maxX - minX);
  scaley = avgDelta / (maxY - minY);
  scalez = avgDelta / (maxZ - minZ);

  saveCalibrationValues();

  Serial.println("Calibración completa.");
  Serial.print("Offset X: "); Serial.println(offsetx);
  Serial.print("Offset Y: "); Serial.println(offsety);
  Serial.print("Offset Z: "); Serial.println(offsetz);
}
#include <Arduino.h>
#include <driver/ledc.h>
#include <QMC5883LCompass.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <math.h>

#define WGS84_A 6378137.0 // Semi-eje mayor
#define WGS84_E2 0.00669438 // Excentricidad al cuadrado
#define K0 0.9996

struct UTMCoordinates {
  double x;
  double y;
};


QMC5883LCompass compass;
TinyGPSPlus gps;

HardwareSerial SerialGPS(2); 


#define RX_GPS 16
#define TX_GPS 17
#define gpsLed 26

float speed = 0.0f;
double lat = 0;
double lon = 0;

float azimuth;
const float declinacion = -9.87;
const float orientationOffset=-65;

#define ena 13  
#define enb 18 

#define motorOut1 27    
#define motorOut2 14    
#define motorOut3 23    
#define motorOut4 19   

const int motorFreq=20000;
const int motorRes=8;
const int enaChannel = 2;  // Canal 2 para ENA
const int enbChannel = 3;  // Canal 3 para ENB

int motorOutputs[4]={motorOut1,motorOut2,motorOut3,motorOut4};
bool executed = false;

void turnRight(float errorMagnitude);
void turnLeft(float errorMagnitude);
void moveForward(float maxSpeed);

void startCompass(){
  compass.init();
  compass.setCalibrationOffsets(-364.00, 1134.00, -715.00);
  compass.setCalibrationScales(0.93, 0.85, 1.34);
}
float readCompass();
bool alignDirection(int direction);

double calculateBearing(double lat1, double lon1, double lat2, double lon2);
void translateDestination(UTMCoordinates destination);
UTMCoordinates LatLonToUTM(double lat, double lon);

void setupMotors(){
  ledcDetachPin(enb);
  pinMode(ena,OUTPUT);
  ledcSetup(enaChannel,motorFreq,motorRes);
  ledcAttachPin(ena,enaChannel);  
  ledcSetup(enbChannel,motorFreq,motorRes);
  ledcAttachPin(enb,enbChannel);
  for(int output : motorOutputs){
    pinMode(output,OUTPUT);
  }
}

WiFiClient espClient;
const char* ssid = "Moto G60";
const char* password = "123456789";

void connectWifi() {
  Serial.print("Conectando al WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("\n[WiFi] Conectado, IP: %s \n", WiFi.localIP().toString().c_str());
}

void readGPSInfo() {
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
    if (gps.location.isUpdated()) {
      lat = gps.location.lat();
      lon = gps.location.lng();
      speed = gps.speed.mps();
    }
  }
  if(gps.satellites.value()>0 && gps.location.isValid()){
    digitalWrite(gpsLed,HIGH);
  }
  else{
    digitalWrite(gpsLed,LOW);
  }
}

PubSubClient mqtt(espClient);
const char* mosquitto_host = "test.mosquitto.org";
const int port = 1883;
const char* sendTopic = "ufpsesp32/coords";
const char* receiveTopic = "ufpsclient/sentCoords";
const char* logsTopic = "ufpsesp32/logs";

unsigned long lastTimeSent = 0;
int sendInterval = 5000;  

void reconnectMQTT() {
  Serial.print("\nIntentando conectar a servidor MQTT...");
  while (!mqtt.connected()) {
    if (mqtt.connect("Esp32")) {
      Serial.println("\n[MQTT] Conectado");
      mqtt.subscribe(receiveTopic); // Mejor aquí una sola vez
      break;
    } else {
      Serial.printf("\n Falló [Err no: %d]", mqtt.state());
      delay(100);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  StaticJsonDocument<256> coords;
  deserializeJson(coords, message);
  double latDestination = coords["lat"];
  double lonDestination = coords["lon"];

  Serial.printf("[Android] %s\n", message.c_str());

  
  float bearing=calculateBearing(lat, lon, latDestination, lonDestination);
  if(alignDirection(bearing)){
    UTMCoordinates projDestination=LatLonToUTM(latDestination,lonDestination);
    translateDestination(projDestination);
  }
}

bool pubMQTT(const char* topic,String msg) {
  return mqtt.publish(topic, msg.c_str());
}

String serializeData(){
  static DynamicJsonDocument data(256);
  data.clear();
  readGPSInfo();
  azimuth=readCompass();
  data["lat"] = lat;
  data["long"] = lon;
  data["speed"] = speed;
  data["orientation"]=azimuth;
  String jsonStr;
  serializeJson(data, jsonStr);
  return jsonStr;
}

void setup() {
  pinMode(13, OUTPUT); 
  digitalWrite(13, LOW);
  delay(100);
  
  Serial.begin(115200);
  connectWifi();
  mqtt.setServer(mosquitto_host, port);
  mqtt.setCallback(callback);

  startCompass();
  setupMotors();

  pinMode(gpsLed, OUTPUT);
  Serial1.begin(9600, SERIAL_8N1, RX_GPS, TX_GPS);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
    return;

  if (!mqtt.connected()) {
      reconnectMQTT();
  }
 
  mqtt.loop();

  readCompass();  
  String data = serializeData();

  if (millis() - lastTimeSent >= sendInterval) {
    if (pubMQTT(sendTopic,data))
      Serial.printf("[Published] %s \n", data.c_str());
    lastTimeSent = millis();
  }
}
float readCompass(){
  compass.read();
  float orientation = fmod(compass.getAzimuth() + orientationOffset + declinacion + 360.0, 360.0);
  return orientation;
}

double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  lat1 = radians(lat1);
  lon1 = radians(lon1);
  lat2 = radians(lat2);
  lon2 = radians(lon2);

  double dLon = lon2 - lon1;

  double y = sin(dLon) * cos(lat2);
  double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);

  double bearing = atan2(y, x);
  bearing = degrees(bearing);
  bearing = fmod((bearing + 360), 360);

  return bearing;
}

bool alignDirection(int targetDirection) {
  const float tolerance = 5.0;  
  while (true) { 
    
    float orientation=readCompass();
    float error = targetDirection - orientation;

    // Normaliza error a rango [-180, 180]
    if (error > 180) error -= 360;
    if (error < -180) error += 360;

    char buffer[100];
    snprintf(buffer, sizeof(buffer), "[Ajustando orientación] %.2f°, [Error] %.2f°\n", orientation, error);
    String orientationLog = String(buffer);    
    pubMQTT(logsTopic,orientationLog);

    if (fabs(error) <= tolerance) {
      Serial.println("[Direction] Dirección alineada");
      break;  
    }

    if (error > 0) {
      turnRight(fabs(error));
    } else {
      turnLeft(fabs(error));
    }

    delay(50); // Pequeña pausa para no saturar
  }
  return true;
}

void turnRight(float errorMagnitude) {
  for (int output = 0; output < 4; output++) {
    int state = (output == 0 || output == 2) ? HIGH : LOW;
    digitalWrite(motorOutputs[output], state);
  }

  int steps = map(constrain(errorMagnitude, 0, 90), 0, 90, 3, 10); // Menos pasos si error pequeño
  int minPower = 120;
  int maxPower = 200;

  for (int i = 0; i < steps; i++) {
    int power = map(i, 0, steps - 1, minPower, maxPower);
    ledcWrite(enaChannel, power);
    ledcWrite(enbChannel, power);
    delay(2);
  }

  delay(5); // Pequeño delay ajustable

  for (int i = steps - 1; i >= 0; i--) {
    int power = map(i, 0, steps - 1, minPower, maxPower);
    ledcWrite(enaChannel, power);
    ledcWrite(enbChannel, power);
    delay(2);
  }

  ledcWrite(enaChannel, 0);
  ledcWrite(enbChannel, 0);
}

void turnLeft(float errorMagnitude) {
  for (int output = 0; output < 4; output++) {
    int state = (output == 1 || output == 3) ? HIGH : LOW;
    digitalWrite(motorOutputs[output], state);
  }

  int steps = map(constrain(errorMagnitude, 0, 90), 0, 90, 3, 10); // Igual que turnRight
  int minPower = 120;
  int maxPower = 200;

  for (int i = 0; i < steps; i++) {
    int power = map(i, 0, steps - 1, minPower, maxPower);
    ledcWrite(enaChannel, power);
    ledcWrite(enbChannel, power);
    delay(2);
  }

  delay(5);

  for (int i = steps - 1; i >= 0; i--) {
    int power = map(i, 0, steps - 1, minPower, maxPower);
    ledcWrite(enaChannel, power);
    ledcWrite(enbChannel, power);
    delay(2);
  }

  ledcWrite(enaChannel, 0);
  ledcWrite(enbChannel, 0);
}

void moveForward(float maxSpeed){
  for (int output = 0; output < 4; output++) {
    int state = (output == 1 || output == 2) ? HIGH : LOW;
    digitalWrite(motorOutputs[output], state);
  }
  ledcWrite(enaChannel,maxSpeed);
  ledcWrite(enbChannel,maxSpeed*0.98); //Compensacion por diferencia de empuje de motores
}

void stopCar(){
  for(int output : motorOutputs){
    digitalWrite(output,LOW);
  }
  ledcWrite(enaChannel,0);
  ledcWrite(enbChannel,0);
}

double calculateDistance(UTMCoordinates location, UTMCoordinates destination) {
  double x1=location.x;
  double y1=location.y;
  double x2=destination.x;
  double y2=destination.y;
  return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

void translateDestination(UTMCoordinates destination){

  while(true){
    readGPSInfo();
    moveForward(200); 
    UTMCoordinates location=LatLonToUTM(lat,lon);
    double distance=calculateDistance(location,destination);
    Serial.printf("[Translation] Distance: %.2f \n",distance);
    moveForward(200); 
    float tolerance=5.0;
    if(distance<=tolerance)
      break;
  }
  stopCar();
}


UTMCoordinates LatLonToUTM(double lat, double lon) {
  int zoneNumber = 18;
  double lonOrigin = (zoneNumber - 1) * 6 - 180 + 3;

  double latRad = lat * M_PI / 180.0;
  double lonRad = lon * M_PI / 180.0;
  double lonOriginRad = lonOrigin * M_PI / 180.0;

  double N = WGS84_A / sqrt(1 - WGS84_E2 * sin(latRad) * sin(latRad));
  double T = tan(latRad) * tan(latRad);
  double C = WGS84_E2 / (1 - WGS84_E2) * cos(latRad) * cos(latRad);
  double A = cos(latRad) * (lonRad - lonOriginRad);

  double M = WGS84_A * (
      (1 - WGS84_E2/4 - 3*WGS84_E2*WGS84_E2/64 - 5*WGS84_E2*WGS84_E2*WGS84_E2/256) * latRad
      - (3*WGS84_E2/8 + 3*WGS84_E2*WGS84_E2/32 + 45*WGS84_E2*WGS84_E2*WGS84_E2/1024) * sin(2*latRad)
      + (15*WGS84_E2*WGS84_E2/256 + 45*WGS84_E2*WGS84_E2*WGS84_E2/1024) * sin(4*latRad)
      - (35*WGS84_E2*WGS84_E2*WGS84_E2/3072) * sin(6*latRad)
  );

  double utmX = K0 * N * (A + (1-T+C)*pow(A,3)/6 + (5-18*T+T*T+72*C-58*0.006739496742)*pow(A,5)/120) + 500000.0;
  double utmY = K0 * (M + N * tan(latRad) * (A*A/2 + (5-T+9*C+4*C*C)*pow(A,4)/24 + (61-58*T+T*T+600*C-330*0.006739496742)*pow(A,6)/720));

  if (lat < 0) {
    utmY += 10000000.0;
  }

  UTMCoordinates result;
  result.x = utmX;
  result.y = utmY;
  return result;
}


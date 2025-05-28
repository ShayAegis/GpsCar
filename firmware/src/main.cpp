#include <Arduino.h>
#include <driver/ledc.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "sensorsUtils.h"
#include "motorsUtils.h"
#include "geoutils.h"

bool executed = false;
void alignDirection();
void translateDestination();
WiFiClient espClient;
const char* ssid = "HUAWEI Y9 2019";
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

PubSubClient mqtt(espClient);
const char* mosquitto_host = "test.mosquitto.org";
const int port = 1883;
const char* sendTopic = "ufpsesp32/coords";
const char* receiveTopic = "ufpsclient/sentCoords";
const char* logsTopic = "ufpsesp32/logs";

hw_timer_t *timer=NULL;

bool publishFlag=false;
bool alignStarted=false;
bool translationStarted=false;
short targetOrientation = 0;
const float orientationTolerance=5.0f;
float toleranceBreak=0.0f;
byte motorSpeed=0;
bool onReverseAlign=false;
float ultrasonicRead=0.0f;
bool collisionDetected=false;

UTMCoordinates targetDestination;
void detectCollision();
void scanSurroundings();

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
  DeserializationError error = deserializeJson(coords, message);
  if (error) {
      Serial.print("Failed to deserialize JSON: ");
      Serial.println(error.f_str());
      return;
  }
  
  if (coords.containsKey("command")) {
      const char* command = coords["command"];
      
      if (strcmp(command, "translation") == 0 && coords.containsKey("value")) {
          double latDestination = coords["value"]["lat"];
          double lonDestination = coords["value"]["lon"];
          Serial.printf("[Android] %s\n", message.c_str());
          targetOrientation = calculateBearing(lat, lon, latDestination, lonDestination);
          targetDestination=LatLonToUTM(latDestination,lonDestination);
          translationStarted=true;
          alignStarted=true;
      } 
      else if (strcmp(command, "orientation") == 0 && coords.containsKey("value")) {
          targetOrientation = coords["value"];
          alignStarted=true;
      }
      else if (strcmp(command, "motor") == 0 && coords.containsKey("value")) {
          int value= coords["value"];
          motorSpeed=map(value,0,100,140,255);
          if(!collisionDetected)
            moveForward(motorSpeed);
    }
      else if(strcmp(command, "calibration") == 0 && coords.containsKey("value")){
          int calibrationStarted= coords["value"];
          if(calibrationStarted)
            compassCalibrate();
          Serial.println("Starting calibration");
      }
  }

  

}
bool pubMQTT(const char* topic,String msg) {
  return mqtt.publish(topic, msg.c_str());
}
String serializeData(bool includeAzimuth){
  static DynamicJsonDocument data(256);
  data.clear();
  data["lat"] = lat;
  data["long"] = lon;
  data["speed"] = speed;
  float azimuth=readCompass();
  if(includeAzimuth)
    data["orientation"]=azimuth;
  String jsonStr;
  serializeJson(data, jsonStr);
  return jsonStr;
}

void pubLocationInfo(){
  readGPSInfo();
  readCompass();
  String json=serializeData(true);
  if(pubMQTT(sendTopic,json)){
    Serial.printf("[Published] %s \n", json.c_str());
  }
}

void IRAM_ATTR onTimer(){
  publishFlag=true;
}

void setup() {
  pinMode(13, OUTPUT); 
  digitalWrite(13, LOW);
  delay(100);
  pinMode(2,OUTPUT);

  Serial.begin(115200);
  connectWifi();
  mqtt.setServer(mosquitto_host, port);
  mqtt.setCallback(callback);
  initSensors();
  setupMotors();

  timer=timerBegin(0,80,true);
  timerAlarmWrite(timer,3e6,true);
  timerAttachInterrupt(timer,&onTimer,true);
  timerAlarmEnable(timer);
  servo.write(90);

}

void loop() {
  if (WiFi.status() != WL_CONNECTED)
    return;

  if (!mqtt.connected()) {
      reconnectMQTT();
  }

  mqtt.loop();
  updateGpsLed();
  detectCollision();
  toleranceBreak=motorSpeed*0.2;
  if (publishFlag) {
    publishFlag = false;
    pubLocationInfo();
  }
  if(alignStarted)
    alignDirection();
  if(translationStarted)
    translateDestination();
  delay(10);
  if(collisionDetected){
      stopCar();
      scanSurroundings();
  }
}


void detectCollision(){
  static int counter=0;
  ultrasonicRead=getUltrasonic();
  if(ultrasonicRead<toleranceBreak){
    digitalWrite(2,HIGH);
    collisionDetected=true;
  }
  else{
    digitalWrite(2,LOW);
    collisionDetected=false;
  }
}
void translateDestination(){
    if(collisionDetected)
      return;
    float orientation=readCompass();
    UTMCoordinates location=LatLonToUTM(lat,lon);
    double distance=calculateDistance(location,targetDestination);
    motorSpeed=200;

    static bool translationInProgress=false;
    if(!translationInProgress){
      translationInProgress=true;
      Serial.printf("[Translation] Distance: %.2f \n",distance);
      moveForward(motorSpeed); 
    }
    else{
      if(fabs(orientation - targetOrientation) > orientationTolerance){
        stopCar();
        alignStarted=true;
      }
      else if(distance<=toleranceBreak){
          stopCar();
          translationStarted=false;
          pubMQTT(logsTopic, "[Translation] Objetivo alcanzado.");
        
      }
        
    }
}


void alignDirection() {
  float orientation;
  static float error;
  static bool alignInProgress = false;
  if (!alignInProgress) {
    alignInProgress = true;
    Serial.println("[Direction] Iniciando alineación...");
  }

  orientation = readCompass();
  error = targetOrientation - orientation;
  if (error > 180) error -= 360;
  if (error < -180) error += 360;

  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Ajustando orientación] %.2f°, [Error] %.2f°\n", orientation, error);
  String orientationLog = String(buffer);    
  pubMQTT(logsTopic, orientationLog);

  if (fabs(error) <= orientationTolerance) {
    Serial.println("[Direction] Dirección alineada");
    alignInProgress = false;  
    alignStarted=false;
  }

  if (error > 0) {
    turnRight(fabs(error));
  } else {
    turnLeft(fabs(error));
  }

  delay(10); 

}
float mapServoToAzimuth(int servoAngle) {
  if (servoAngle <= 90) {
    return 90 - servoAngle;
  }
  else {
    return 450 - servoAngle;
  }
}
void scanSurroundings(){
  if(alignStarted)
    return;
  float maxDistance = 0;   
  float directionAngle = 0;

  for (int i = 25; i <= 135; i+=5) {
    servo.write(i);
    delay(100);   
    float distance = getUltrasonic(); 
    if (distance > maxDistance) {
      maxDistance = distance;
      directionAngle = i;
    }
  }

  pubMQTT(logsTopic,"[Ultrasonic] Avoid obstacle at "+String(directionAngle)+" distance "+String(maxDistance));
  Serial.println("[Ultrasonic] Avoid obstacle at "+String(directionAngle)+" distance "+String(maxDistance));
  targetOrientation = readCompass()+mapServoToAzimuth(directionAngle);                                                                                                     
  alignStarted = true;           
  translationStarted = false;   
  servo.write(90);             
}

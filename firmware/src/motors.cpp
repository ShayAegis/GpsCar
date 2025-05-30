#include "motorsUtils.h"

Servo servo;

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
    servo.attach(servoPin);
  }

  void turnRight(float errorMagnitude) {
    for (int output = 0; output < 4; output++) {
      int state = (output == 0 || output == 2) ? HIGH : LOW;
      digitalWrite(motorOutputs[output], state);
    }
  
    int steps = map(constrain(errorMagnitude, 0, 90), 0, 90, 3, 10); 
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
  
  void turnLeft(float errorMagnitude) {
    for (int output = 0; output < 4; output++) {
      int state = (output == 1 || output == 3) ? HIGH : LOW;
      digitalWrite(motorOutputs[output], state);
    }
  
    int steps = map(constrain(errorMagnitude, 0, 90), 0, 90, 3, 10); 
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
      int state = (output == 1 || output == 2) ? LOW : HIGH; 
      digitalWrite(motorOutputs[output], state);
    }
  
    float currentSpeed = 0;
    float rampDuration = 500;  
    int rampSteps = 50;      
    float dt = rampDuration / rampSteps;
  
    for (int i = 0; i <= rampSteps; i++) {
      float progress = (float)i / rampSteps;
      currentSpeed = maxSpeed * (1 - exp(-5 * progress)); 
  
      ledcWrite(enaChannel, currentSpeed * 0.99);
      ledcWrite(enbChannel, currentSpeed);
  
      delay(dt);
    }
  }
  void moveReverse(float maxSpeed){
    for (int output = 0; output < 4; output++) {
      // Invertimos la lógica de avance
      int state = (output == 1 || output == 2) ? HIGH : LOW;
      digitalWrite(motorOutputs[output], state);
    }
    ledcWrite(enaChannel, maxSpeed);
    ledcWrite(enbChannel, maxSpeed);
  }
  void stopCar(){
    for(int output : motorOutputs){
      digitalWrite(output,LOW);
    }
    ledcWrite(enaChannel,0);
    ledcWrite(enbChannel,0);
  }
  void calibrationTurn(){
    const byte turnSpeed=150;
    for (int output = 0; output < 4; output++) {
      int state = (output == 1 || output == 3) ? HIGH : LOW;
      digitalWrite(motorOutputs[output], state);
    }    
    ledcWrite(enaChannel,turnSpeed);
    ledcWrite(enbChannel,turnSpeed*1.1);
  }
  void servoTest(){
    for (int i = 25; i <= 135; i++) {
        servo.write(i);
        delay(10);  
      }
      for (int i = 135; i >= 25; i--) {
        servo.write(i);
        delay(10);
      }
  }
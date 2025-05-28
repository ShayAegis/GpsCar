#ifndef MOTORSUTILS_H
#define MOTORSUTILS_H
#include <Arduino.h>
#include <ESP32Servo.h>
#define ena 13  
#define enb 18 

#define motorOut1 27    
#define motorOut2 14    
#define motorOut3 23    
#define motorOut4 19   

const short motorFreq=20000;
const byte motorRes=8;
const byte enaChannel = 2; 
const byte enbChannel = 3;  
const byte servoPin=5;

extern Servo servo;


const byte motorOutputs[4]={motorOut1,motorOut2,motorOut3,motorOut4};

void setupMotors();
void turnRight(float errorMagnitude);
void turnLeft(float errorMagnitude);
void moveForward(float maxSpeed);
void moveReverse(float maxSpeed);
void stopCar();
void servoTest();

#endif

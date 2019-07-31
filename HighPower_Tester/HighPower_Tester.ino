//MargayDemo.ino
#include "Resnik.h"
#include <BME.h>

BME RH; //Initialzie BME280

String Header = ""; //Information header
uint8_t I2CVals[1] = {0x77}; 
// int Count = 0;
unsigned long UpdateRate = 5; //Number of seconds between readings 

Resnik Logger;

void setup() {
  Header = Header + RH.GetHeader();
  Logger.begin(I2CVals, sizeof(I2CVals), Header); //Pass header info to logger
  Init();
}

void loop() {
  Logger.Run(Update, UpdateRate);
//  Logger.AddDataPoint(Logger.GetOnBoardVals());
//  Serial.println(Logger.GetOnBoardVals());
//  delay(100);
}

String Update() 
{
  Init();
  delay(3000);
  return RH.GetString();
//return "x";
}

void Init() 
{
  RH.begin(0x77);
}

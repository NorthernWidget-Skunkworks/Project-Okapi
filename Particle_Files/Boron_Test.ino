#include "dct.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
// SYSTEM_MODE(AUTOMATIC);

#define RST 9 //Reset connected to pin 9
#define WD_DONE 16 //WD_DONE connected to pin 16
#define GPIO D14 //GPIO to micro on pin 14

#define BACKHAUL_NUM 5 //Define the number of backhaul values at a given time //FIX make flexible, just read until stop value??
String GlobalRead; 
String Data[BACKHAUL_NUM];
// int Count = 0;

int ForceReset(String command);

void setup() {
    Cellular.setActiveSim(INTERNAL_SIM);
    Cellular.clearCredentials();
    // This set the setup done flag on brand new devices so it won't stay in listening mode
    const uint8_t val = 0x01;
    dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, 1);
    pinMode(9, INPUT); //Set RST as input to not interfere with rest of system
    pinMode(D7, OUTPUT);
    pinMode(GPIO, OUTPUT);
    Serial.begin(9600);
    Serial1.begin(38400); //Begin to comunicate with the micro
    Serial.println("START");
    Particle.function("Reset", ForceReset);
}

void loop() {
    while(Serial1.available()) {
        String Val = Serial1.readStringUntil('\n');
        if(Val.trim() == "START BACKHAUL") {
            digitalWrite(D7, HIGH);  //DEBUG!
            digitalWrite(GPIO, HIGH);
            for(int i = 0; i < BACKHAUL_NUM; i++) { //Read data into array
                // while(!Serial1.available()); //wait for data
                Data[i] = Serial1.readStringUntil('\n'); 
            }
            // GlobalRead = Serial1.readString();
            // Serial.println("Got Values");
            for(int i = 0; i < BACKHAUL_NUM; i++) { //Read data into array
                Serial.println(Data[i]); //Send the values back on USB serial //FIX! Replace with publish 
            }
            digitalWrite(D7, LOW); //DEBUG!
            Cellular.on(); //Turn on cell module 
            for (uint32_t ms = millis(); millis() - ms < 1000; Particle.process());
            Particle.connect(); //Connect to cloud
            for (uint32_t ms = millis(); millis() - ms < 10000; Particle.process());  // wait 10 seconds to Connect.  Boron connects in <10seconds on my desk in AutoMode.
            waitUntil(Particle.connected); //Wait until connection is complete
            Serial.println("Connected");
            for(int i = 0; i < BACKHAUL_NUM; i++) { //Read data into array
                Particle.publish("Data", (Data[i]), WITH_ACK); //Send the values back on USB serial //FIX! Replace with publish
                delay(1000); 
            }
            Serial.println("Sent");

            digitalWrite(GPIO, LOW);
            // Serial.println(GlobalRead); //Print to serial for test
            // Particle.publish("Data", GlobalRead);
        }
        // Serial.println(Val);
    }
    // if(Particle.connected()) {
        // Particle.publish("Data", GlobalRead, WITH_ACK); //Will not return until cloud has acked message
        // digitalWrite(GPIO, HIGH); //Trigger micro to let it know message has been sent off
    // }
    // else digitalWrite(GPIO, LOW); 
    // if(Count > 1000) {
    //     Serial.println("Rollover");
    //     Count = 0;
    // }
    // Count++; 
}

// this function automagically gets called upon a matching POST request
int ForceReset(String command)
{
  // look for the matching argument "coffee" <-- max of 64 characters long
  if(command == "RST")
  {
    //Set FEATHER_EN pin high to connect power to feather
    Serial.println("RESET");
    pinMode(RST, OUTPUT);
    digitalWrite(RST, LOW);
    delay(1);
    digitalWrite(RST, HIGH);
    return 1;
  }
  else return -1;
}


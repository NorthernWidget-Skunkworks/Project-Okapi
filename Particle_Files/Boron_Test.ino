#include "dct.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
// SYSTEM_MODE(AUTOMATIC);

#define RST D4 //Reset connected to pin 9
#define WD_DONE A3 //WD_DONE connected to pin 16
#define GPIO D14 //GPIO to micro on pin 14

#define BACKHAUL_NUM 5 //Define the number of backhaul values at a given time //FIX make flexible, just read until stop value??
String GlobalRead; 
String Data[BACKHAUL_NUM];

unsigned long Timeout = 0;
// int Count = 0;

int ForceReset(String command);

ApplicationWatchdog wd(90000, Mayday);

void setup() {
    pinMode(GPIO, OUTPUT);
    digitalWrite(GPIO, HIGH);
    Cellular.setActiveSim(INTERNAL_SIM);
    Cellular.clearCredentials();
    // This set the setup done flag on brand new devices so it won't stay in listening mode
    const uint8_t val = 0x01;
    dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, 1);
    pinMode(9, INPUT); //Set RST as input to not interfere with rest of system
    pinMode(D7, OUTPUT);
    pinMode(WD_DONE, OUTPUT);
    digitalWrite(WD_DONE, HIGH);
    Serial.begin(9600);
    Serial1.begin(38400); //Begin to comunicate with the micro
    Serial.println("START");
    Particle.function("Reset", ForceReset);
    Timeout = millis();
}

void loop() {
    while(Serial1.available()) {
        wd.checkin(); //If any serial print made, service wd //FIX??
        digitalWrite(D7, HIGH);  //DEBUG!
        String Val = Serial1.readStringUntil('\n');
        Serial.println(Val.trim()); //DEBUG!
        if(Val.trim() == "START BACKHAUL") {
            // while(Val.trim() == "START BACKHAUL") { //Read until data is being sent
            //     Val = Serial1.readStringUntil('\n'); 
            // }
            digitalWrite(D7, HIGH);  //DEBUG!

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
    if(millis() - Timeout > 90000) {
        pinMode(RST, OUTPUT);
        digitalWrite(RST, HIGH);
        delay(1);
        digitalWrite(RST, LOW);
        delay(1);
    }
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

void Mayday() {
   // System.sleep(SLEEP_MODE_DEEP, 60);
   pinMode(RST, OUTPUT);
   digitalWrite(RST, HIGH);
   delay(5);
   digitalWrite(RST, LOW);
   delay(5);
   digitalWrite(WD_DONE, LOW);  //Cycle VBat if in use, FIX??
   delay(1);
   digitalWrite(WD_DONE, HIGH);
   delay(1); 
}


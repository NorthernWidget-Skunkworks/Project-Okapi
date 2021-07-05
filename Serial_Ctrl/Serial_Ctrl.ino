#include <MCP23018.h>  //Include IO expander interface 
#include <DS3231.h>  //Include RTC interface
#include <BME.h>  //Include BME280 interface
#include <Adafruit_ADS1015.h> //Include ADC interface
#include <SD.h>  //Include SD interface
#include <SPI.h> //Include base SPI library
#include <MCP4725.h> //Include custom DAC interface 
#include <EEPROM.h>
#include <Wire.h> //Include base Wire library

//If a pin is defined as "0", it is allowed to be made an output, if pin is defined as a "1" is if forbidden to be defined as an output
// const uint32_t PinMask = 0b11100000101000001100000000011000; //Pin configuration mask for uC
// const uint16_t ExpPinMask = 0b1001110101011110; //Pin configuration mask for IO Expander
const uint16_t ExpPinMask = 0x00; //DEBUG!
const uint32_t PinMask = 0x00; //DEBUG!

MCP23018 IO(0x20); //Instatiate IO Expander
BME RH; //Instatiate BME280
MCP4725 DAC; //Instatiate DAC
Adafruit_ADS1115 ADC_OB(0x48); //Initialize on board (power moitoring) ADC
Adafruit_ADS1115 ADC_Ext(0x49);  //Initialize external (sensor) ADC


float BMEData[3] = {0}; //Storage for BME data, Temp, Pressure, RH //DEBUG??

boolean Debug = false; //FIX!!
char ReadArray[25] = {0};

const uint8_t C1 = 19; //Control pins for V_HP
const uint8_t C0 = 18;

const uint8_t EN_BUS_PRIME = 23;  //Power enable lines for buses
const uint8_t EN_BUS_SEC = 22;

const uint8_t GPIO[4] = {12, 25, 3, 26}; //GPIO pins 0~3, 0,1 on primary bus, 2,3 on secondary bus. 0 and 2 PWM capable

const uint8_t Global_Int = 28; //Global interrupt from IO Expander
const uint8_t Log_Int = 27; //Interrupt for logging event

////////////////////////////////////SD////////////////////////////////////////
const uint8_t SD_CS = 4; //Define chip select for SD card
char FileNameC[11]; //Used for file handling
boolean SD_Init = false;

////////////////////////////////////CLOCK//////////////////////////////////////
DS3231 Clock;

////////////////////////////////////////LED//////////////////////////////////
const uint8_t BuiltInLED = 20;
const uint8_t RED = 13;
const uint8_t GREEN = 15;
const uint8_t BLUE = 14;
const uint8_t RGB[] = {RED, GREEN, BLUE};

void setup() {
	Serial.begin(57600); //Initialize serial for general comunication
	Serial.println("Welcome to Resnik...");
	Serial.println("Developed by Dennis Nedry");
	Serial.print("Loading...\n\n");
	SetHP(1); //Turn on power via VBeta
	pinMode(BuiltInLED, OUTPUT);
	digitalWrite(BuiltInLED, LOW); //DEBUG!
	Wire.begin(); //Initialize base I2C library
	Serial.print("\tIO Exp... ");
	IO.begin(); //Initialize IO Expander
	Serial.println("Done");

	Serial.print("\tBME280... ");
	RH.begin(); //Initialize BME280
	Serial.println("Done");

	Serial.print("\tDAC... ");
	DAC.begin(0x62); //Intialize DAC
	Serial.println("Done");

	//ADD ADCS!!
	digitalWrite(BuiltInLED, HIGH); //DEBUG!
	Serial.println("Done");
	Serial.println("Enter When Ready...");
}

void loop() {
  static int ReadLength = 0;
  String ReadString;
  
  if(Serial.available() > 0) {
    char Input = Serial.read();

    if(Input != '\r') { //Wait for carrage return
      ReadArray[ReadLength] = Input;
      ReadLength++;
    }

    if(Input == '\r') {
      ReadString = String(ReadArray);
      ReadString.trim();
      memset(ReadArray, 0, sizeof(ReadArray));
      ReadLength = 0;
      
      if(ReadString == "Help") {
      	Serial.println("Commands:");
      	Serial.print(F("\tSD\n\tClock\n\tI2C\n\tADC Disp\n\tLED\n"));
      	Serial.print(F("\tSet HP\n\tSet Mode uC\n\tSet Pin uC\n\tRead Pin uC\n\tSet DAC\n\tRead ADC OB\n\tRead ADC Ext\n"));
      }

      if(ReadString == "SD") {
        Serial.println("\nTesting SD");
       	Serial.println("Please Wait...");
        String TestResult = bool2pf(TestSD());
        Serial.println(TestResult);
      }

      if(ReadString == "Clock") {
        Serial.println("\nTesting Clock");
       Serial.println("Please Wait...");
        String TestResult = bool2pf(TestClock());
        Serial.println(TestResult);
      }

      if(ReadString == "I2C") {
        Serial.println("\nI2C Test");
       Serial.println("Please Wait...");
        I2CTest();
        Serial.println("End");
      }

      if(ReadString == "ADC Disp") {
        Serial.println("\nBegin ADC Test");
        ADCDisp();
        Serial.println("...End ADC Test");
      }

      // if(ReadString == "IO") {
      //   Serial.println("\nIO Test");
      //   IOTest();
      //   Serial.println("End");
      // }

      // if(ReadString == "PG") {
      //   Serial.println("\nBegin PG Test");
      //   String TestResult = bool2pf(PGTest());
      //   Serial.print("PG: ");
      //   Serial.println(TestResult);
      // }

      // if(ReadString == "Power") { 
      //   Serial.println("\nPower Test");
      //   PowerTest();
      //   Serial.println("End");
      // }

      // if(ReadString == "Button") {
      //   Serial.println("\nBegin Button Test");
      //   ButtonTest();
      // }
      // Serial.println(ReadString); //DEBUG!
      // Serial.println(ReadString.indexOf("TEST")); //DEBUG!
      if(ReadString.indexOf("Set HP") >= 0) {
      	SetHP(ReadString.substring(6).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Set Mode uC") >= 0) { //"Pin Mode uC xx y", xx is pin position (0 ~ 32), y is direction (0 ~ 1)
      	// Serial.println(ReadString.substring(13).toInt()); //DEBUG!
      	// Serial.println(ReadString.indexOf(",")); //DEBUG!
      	SetPinMode(ReadString.substring(11).toInt(), ReadString.substring(ReadString.indexOf(",") + 1).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Set Mode IO") >= 0) { //"Pin Mode uC xx y", xx is pin position (0 ~ 32), y is direction (0 ~ 1)
      	SetPinModeIO(ReadString.substring(11).toInt(), ReadString.substring(ReadString.indexOf(",") + 1).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Set Pin uC") >= 0) { //"Set uC Pin xx y", xx is pin position (0 ~ 32), y is value (0 ~ 1)
      	// Serial.println(ReadString.substring(ReadString.indexOf(",")).toInt()); //DEBUG!
      	// Serial.println(ReadString.substring(ReadString.indexOf(","))); //DEBUG!
      	// Serial.println(ReadString.substring(ReadString.indexOf(",") + 1).toInt()); //DEBUG!
      	DigitalWrite(ReadString.substring(10).toInt(), ReadString.substring(ReadString.indexOf(",") + 1).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Set Pin IO") >= 0) { //"Set IO Pin xx y", xx is pin position (0 ~ 32), y is value (0 ~ 1)
      	DigitalWriteIO(ReadString.substring(10).toInt(), ReadString.substring(ReadString.indexOf(",") + 1).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Read Pin uC") >= 0) { //"Set uC Pin xx", xx is pin position (0 ~ 32)
      	DigitalRead(ReadString.substring(11).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Read Pin IO") >= 0) { //"Set uC Pin xx", xx is pin position (0 ~ 32)
      	DigitalReadIO(ReadString.substring(11).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Read ADC OB") >= 0) { //"Read ADC OB xx", xx is pin position (0 ~ 3)
      	ReadADC(ReadString.substring(11).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Read ADC Ext") >= 0) { //"Read ADC OB xx", xx is pin position (0 ~ 3)
      	ReadADCExt(ReadString.substring(12).toInt()); //Pass int to function
      }

      if(ReadString.indexOf("Set DAC") >= 0) { //"Set DAC x.x...", x.x is voltage value 
      	SetDAC(ReadString.substring(7).toFloat()); //Pass float to function
      }

      if(ReadString == "LED") {
        Serial.println("\nLED Test");
        LEDTest();
        Serial.println("End");
      }

      if(ReadString == "SN Set") {
        char Data[5] = {0};
        while(Serial.available() > 0) {
          int x = Serial.read();
        }

        Serial.print("Enter Board Type... ");
        while(Serial.available() == 0);
        String DeviceIDStr = Serial.readString();
        DeviceIDStr.toCharArray(Data, 5);
        uint16_t DeviceID = (int)strtol(Data, NULL, 16);
        Serial.println(DeviceID, HEX);

        Serial.print("Enter Group ID... ");
        while(Serial.available() == 0);
        String GroupIDStr = Serial.readString();
        GroupIDStr.toCharArray(Data, 5);
        uint16_t GroupID = (int)strtol(Data, NULL, 16);
        Serial.println(GroupID, HEX);

        Serial.print("Enter Unique ID... ");
        while(Serial.available() == 0);
        String UniIDStr = Serial.readString();
        UniIDStr.toCharArray(Data, 5);
        uint16_t UniID = (int)strtol(Data, NULL, 16);
        Serial.println(UniID, HEX);

        int Location = EEPROM.length() - 8;
        
        boolean Fault = false;
        uint16_t FirmwareID = 0x0000;
         uint16_t SerialNumber[] = {DeviceID, GroupID, UniID, FirmwareID}; 

         for(int i = 0; i < 4; i++) { 
            EEPROM.write(Location, (SerialNumber[i] & 0xFF00) >> 8);
            EEPROM.write(Location + 1, SerialNumber[i] & 0xFF);
            Location += 2;
          }
       Location = EEPROM.length() - 8;
       
       for(int i = 0; i < 4; i++) {
          if( EEPROM.read(Location) != (SerialNumber[i] & 0xFF00) >> 8 || EEPROM.read(Location + 1) != (SerialNumber[i] & 0xFF)){
            Fault = true;
          }
          Location += 2;
       }
      
       if(Fault == true){
        Serial.println("CRITICAL ERROR: Readback Failed, try again");
       }
       
       else{
        Serial.println("\n...Serial Number Write Completed Without Errors.");
       }
      }

      if(ReadString == "SN Read") {
        int Location = EEPROM.length() - 8;
        for(int i = 0; i < 8; i++) {
          Serial.println(EEPROM.read(Location + i), HEX);
       }
      }

      if(ReadString == "ItsAUnixSystem") {
       Serial.println("\nDebuging Output Enabled!");
       Serial.println("Careful not to turn all the fences off...");
        Debug = true;
      }
    }
	}
}

void SetPinMode(uint8_t Pin, bool State)  //Set the pin mode of an onboard pin
{
	//Add check for pin range!
	if(State == INPUT || (State == OUTPUT && ((PinMask >> Pin) & 0x01) == 0)) {  //Proceed only if input, or output AND mask allows for output
		pinMode(Pin, !State);  //Arduino for some reason used inverse logic for OUTPUT and INPUT... *sigh*
 		Serial.print("uC Pin ");
		Serial.print(Pin);
		if(State == 0) Serial.println(" Set as Output");
		if(State == 1) Serial.println(" Set as Input");
	}
	else Serial.println("Pin Direction Error!");
}

bool SetPinModeIO(uint8_t Pin, bool State)  //Set the pin mode of a pin on the IO expander
{
	//Add check for pin range!
	if(State == INPUT || (State == OUTPUT && ((ExpPinMask >> Pin) & 0x01) == 0)) {  //Proceed only if input, or output AND mask allows for output
		IO.PinMode(Pin, State, 0); // Will only check Port 0 for now
		// IO.PinMode(Pin, State, 1); // ????
		Serial.print("IO Exp Pin ");
		Serial.print(Pin);
		if(State == 0) Serial.println(" Set as Output");
		if(State == 1) Serial.println(" Set as Input");
	}
	else Serial.println("Pin Direction Error!");
}

void DigitalWrite(uint8_t Pin, bool State) //Set the state of a pin on the uC
{
	if(((PinMask >> Pin) & 0x01) == 1) Serial.println("Pin Direction Error!"); //Pin is not configurable as output
	else {
		digitalWrite(Pin, State, 0); //Set pin state, Port 0
		Serial.print("uC Pin ");  //Print Status
		Serial.print(Pin);
		Serial.print(" Set to ");
		Serial.println(State);
	}
}

void DigitalWriteIO(uint8_t Pin, bool State)
{
	if(((ExpPinMask >> Pin) & 0x01) == 1) Serial.println("Pin Direction Error!"); //Pin is not configurable as output
	else {
		IO.DigitalWrite(Pin, State); //Set pin state
		Serial.print("IO Pin ");  //Print Status
		Serial.print(Pin);
		Serial.print(" Set to ");
		Serial.println(State);
	}
}

void DigitalRead(uint8_t Pin)
{
	Serial.print("uC Pin ");  //Print Status
	Serial.print(Pin);
	Serial.print(" Value is ");
	Serial.println(digitalRead(Pin));
}

void DigitalReadIO(uint8_t Pin)
{
	Serial.print("IO Pin ");  //Print Status
	Serial.print(Pin);
	Serial.print(" Value is ");
	Serial.println(IO.DigitalRead(Pin));
}

void ReadADC(uint8_t Pin) 
{
	//Check pin??
	Serial.print("ADC OB, ");
	Serial.print(Pin); 
	Serial.print(" = ");
	Serial.print(ADC_OB.readADC_SingleEnded(Pin)*0.1875, 5);
	Serial.println("mV");
}

void ReadADCExt(uint8_t Pin)
{
	Serial.print("ADC Ext, ");
	Serial.print(Pin); 
	Serial.print(" = ");
	Serial.print(ADC_Ext.readADC_SingleEnded(Pin)*0.1875, 5);
	Serial.println("mV");
}

void SetDAC(float Val)
{
	uint16_t BitVal = floor((Val/5.0)*4096); //Convert to bit respresentaiton
	DAC.setVoltage(BitVal, false); //set DAC, do not store result in EEPROM (false flag)
	Serial.print("DAC Output Set To ");
	Serial.println(Val, 4);
	if(Debug) Serial.println((BitVal/4096)*5.0, 4); //Print actual (nearest) value 
}

void SetHP(int State)  //-1 off, 0 off, 1 VBeta, 2 VPrime
{
	pinMode(C0, OUTPUT); //Set as outputs, if not already
	pinMode(C1, OUTPUT);
	Serial.print("High Power Status: ");
	switch(State){
		case -1:  //Turn off power
//      pinMode(16, OUTPUT);
//      pinMode(17, OUTPUT);
//      
//      pinMode(16, LOW);
//      pinMode(17, LOW);
			Serial.println(" OFF");
			digitalWrite(C0, HIGH);
			digitalWrite(C1, HIGH);
			break;

		case 0:  //Turn off power
//      pinMode(16, OUTPUT);
//      pinMode(17, OUTPUT);
//      
//      pinMode(16, LOW);
//      pinMode(17, LOW);
			Serial.println(" OFF");
			digitalWrite(C0, LOW);
			digitalWrite(C1, LOW);
			break;

		case 1:  //Turn on VBeta 
			Serial.println(" VBeta");
			digitalWrite(C0, LOW);
			digitalWrite(C1, HIGH);
			break;

		case 2:  //Turn on VPrime 
			Serial.println(" VPrime");
			digitalWrite(C0, HIGH);
			digitalWrite(C1, LOW);
			break;

		default:  //Turn off by default 
//      pinMode(16, OUTPUT);
//      pinMode(17, OUTPUT);
//      
//      pinMode(16, LOW);
//      pinMode(17, LOW);
			Serial.println(" OFF");
			digitalWrite(C0, LOW);
			digitalWrite(C1, LOW);
			break;
	}
}

void LEDTest() {
  //Initalize LEDs and set all off
  pinMode(BuiltInLED, OUTPUT);
  digitalWrite(BuiltInLED, HIGH);
  for(int p = 0; p < 3; p++){
    pinMode(RGB[p], OUTPUT);
    digitalWrite(RGB[p], HIGH);
  }
  
  while(Serial.read() != '\r'){
//    bool State = false;
    for(int i = 0; i < 3; i++){
      digitalWrite(RGB[i], LOW);
      delay(1000);
//      State = !State;
      digitalWrite(RGB[i], HIGH);
      delay(1000);
    }
    digitalWrite(BuiltInLED, LOW);
    delay(1000);
    digitalWrite(BuiltInLED, HIGH);
    delay(1000);
  }
}

void ADCDisp(){ 
	Serial.println("IN DEVELOPMENT!");
    // while(Serial.read() != '\r'){
    //   Serial.print("Ref = "); 
    //   Serial.print(analogRead(VRef_Pin)*(3.3/1024.0));
    //   Serial.println(" V");

    //   Serial.print("Therm = "); 
    //   Serial.print(analogRead(ThermSense_Pin)*(3.3/1024.0));
    //   Serial.println(" V");

    //   Serial.print("Bat = "); 
    //   Serial.print(analogRead(BatSense_Pin)*(3.3/1024.0)/10.0);
    //   Serial.println(" V");

    //   Serial.print("Ax = "); 
    //   Serial.print(adc.GetVoltage()*(3.3/1024.0));
    //   Serial.println(" V");

    //   Serial.print("\n\n");
    // }
}

void I2CTest() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

boolean TestSD()
{
  boolean SDError = false;
  pinMode(SD_CS, OUTPUT);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2); //Sts SPI clock to 4 MHz for an 8 MHz system clock

  if(!SD_Init) {
  if (!SD.begin(SD_CS)) {
    if(Debug) Serial.println("Failed Init");
    SDError = true; 
    return SDError;
  }
  if(Debug) Serial.println("Initialized");
  SD_Init = true;
  }
  String FileName = "HWTest";
  (FileName + ".txt").toCharArray(FileNameC, 11);
  SD.remove(FileNameC); //Remove any previous files
  
  randomSeed(millis()); //Seed with a random number to try to endsure randomness
  int RandVal = random(30557); //Generate a random number between 0 and 30557 (the number of words in Hamlet)
  char RandDigits[5] = {0};
  sprintf(RandDigits, "%d", RandVal); //Convert RandVal into a series of digits
  int RandLength = (int)((ceil(log10(RandVal))+1)*sizeof(char)); //Find the length of the values in the array
  // Serial.print("Random Digits:"); //DEBUG!
  // for(int p = 0; p < 5; p++) { //DEBUG!
  //   Serial.print(RandDigits[p]);
  // }
  // Serial.println(""); //DEBUG!

  File DataWrite = SD.open(FileNameC, FILE_WRITE);
  if(DataWrite) {
    DataWrite.println(RandVal);
    DataWrite.println("\nHe was a man. Take him for all in all.");
    DataWrite.println("I shall not look upon his like again.");
    DataWrite.println("-Hamlet, Act 1, Scene 2");
  }
  DataWrite.close();

  if(Debug){
    Serial.print("Random Value = "); Serial.println(RandVal);
  }
  
  char TestDigits[5] = {0};
  File DataRead = SD.open(FileNameC, FILE_READ);
  if(DataRead) {
    DataRead.read(TestDigits, RandLength);

    if(Debug){
      Serial.print("Recieved Value = ");
    }
    for(int i = 0; i < RandLength - 1; i++){ //Test random value string
      if(Debug) Serial.print(TestDigits[i]);
      if(TestDigits[i] != RandDigits[i]) {
        SDError = true;
        Serial.print("x"); //DEBUG!
        Serial.print(RandDigits[i]);
      }
    }
    if(Debug) Serial.println("");
  
    if(Debug) {
      Serial.println("SD Test Data:");
      while(DataRead.available()) {
        Serial.write(DataRead.read());
      }
    }
  }
  DataRead.close();

  

  if(Debug) Serial.println("");
  
  return SDError;
}

boolean TestClock()
{
  boolean ClockError = false;
  boolean DumbFalse = false;
  Clock.setClockMode(false);  // set to 24h
  byte DateSet[] = {63, 4, 5, 4, 20, 0, 0};
  byte DateRead[7] = {0};
  
  Clock.setYear(DateSet[6]);
  Clock.setMonth(DateSet[5]); 
  Clock.setDate(DateSet[4]);
  Clock.setDoW(DateSet[3]); //Thursday?
  Clock.setHour(DateSet[2]);
  Clock.setMinute(DateSet[1]);
  Clock.setSecond(DateSet[0]);

  delay(5000);

  DateRead[6] = Clock.getYear();
  DateRead[5] = Clock.getMonth(DumbFalse);
  DateRead[4] = Clock.getDate();
  DateRead[3] = Clock.getDoW();
  DateRead[2] = Clock.getHour(DumbFalse, DumbFalse);
  DateRead[1] = Clock.getMinute();
  DateRead[0] = Clock.getSecond();
  
  for(int i = 6; i > 0; i--){
    if(DateRead[i] != DateSet[i]) ClockError = true;
  }
    if(DateRead[0] != DateSet[0] + 5 && DateRead[0] != DateSet[0] + 4 && DateRead[0] != DateSet[0] + 6) ClockError = true;

  if(Debug) {
    Serial.println("Values Read:");
    Serial.print("Year = "); Serial.print("20"); Serial.println(DateSet[6]);
    Serial.print("Month = "); Serial.println(DateSet[5]);
    Serial.print("Day = "); Serial.println(DateSet[4]);
    Serial.print("DoW = "); Serial.println(DateSet[3]);
    Serial.print("Hour = "); Serial.println(DateSet[2]);
    Serial.print("Minute = "); Serial.println(DateSet[1]);
    Serial.print("Second = "); Serial.println(DateSet[0]);
    
    Serial.println("\nValues Read:");
    Serial.print("Year = "); Serial.print("20"); Serial.println(DateRead[6]);
    Serial.print("Month = "); Serial.println(DateRead[5]);
    Serial.print("Day = "); Serial.println(DateRead[4]);
    Serial.print("DoW = "); Serial.println(DateRead[3]);
    Serial.print("Hour = "); Serial.println(DateRead[2]);
    Serial.print("Minute = "); Serial.println(DateRead[1]);
    Serial.print("Second = "); Serial.println(DateRead[0]);
    Serial.println("");
  }
  return ClockError;

  //Test alarms??
}

String bool2pf(boolean TestResult){
  if(TestResult) return "FAIL";
  return "PASS";
}

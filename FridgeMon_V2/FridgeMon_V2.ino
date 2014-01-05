/****************************************************************
* Fridge tempreture monitor and logger.                         *
* Sensors in three frdige zones and Sensor outside              *
* LCD Display Ethernet access                                   *
* Logging with RTC                                              *
*                                                               *
* http://typefaster.tumblr.com/                                 *
*****************************************************************/
/*
* Use FridgeMon_Sensor_serial to obtain the ROM code for one wire sensors (R=xxx)
* Use DumpTempLogFile to dump the SD card log to the serial port
* Use SDdataLog_delete to clear the log file
* SD_listfile shows the files on the SD card
* http://typefaster.tumblr.com/post/17052725855/arduino-fridge-temperature-monitor
Version Changes:
V1: Working Version
V2: Corrected some comments

*/

// Debug Code. Uncomment these to make active
// These pump a fair amount of text to the serial port
// #define TempDebug
// #define TimeDebug
// #define TextDebug

// Libraries
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <Wire.h>         // necessary, or the application won't build properly
#include <stdio.h>
#include <PCF8583.h>      // Note that I've modified the standard library to cope with V1.0
#include <SD.h>

// Global variables
// LiquidCrystal lcd(7,8,9,10,11,12);  // Initialise LCD
// Note SD Card uses  pins  4, 11, 12, 13
// Note Ethernet uses pins 10, 11, 12, 13
LiquidCrystal lcd(3,5,6,7,8,9);        // Initialise LCD
OneWire TempPin(2);                    // Initialise OneWire on pin 2
PCF8583 p (0xA0);                      // RTC Address

// dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);
char s[32];                            // buffer for dtostrf function

// On the Ethernet Shield, CS is pin 10. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;            // Pin 4 on the Freetronics Etherten board

 // OneWire Sensor addresses. There are the three I made into modules
 // Use FridgeMon_Sensor_serial to obtain the ROM code for one wire sensors (R=xxx)
 byte Sense[3][8] = { {0x28, 0x57, 0x8B, 0x9E, 0x03, 0x00, 0x00, 0x1E},
                      {0x28, 0xAD, 0x76, 0x9E, 0x03, 0x00, 0x00, 0xE9},
                      {0x28, 0xB8, 0x76, 0x9E, 0x03, 0x00, 0x00, 0x59} };
                                                                                                  
 String SenseName[3][3] = { {"Top", "Top",    "T"}, // Sensor names
                            {"Out", "Outside","O"},
                            {"Bot", "Bottom", "B"} };

// LCD start locations for sensors (Col, Row)
 int SenseLCD[3][2] = { { 0, 0},     // Sensor 0
                        { 8, 0},     // Sensor 1
                        { 0, 1} };   // Sensor 2
                      
  int NextMinute = 0;                // Tracks time (minute only), so w know to write to SD card
 
/******************* Setup **********************************/ 
void setup() {
  
  lcd.begin(16, 2);                  // set up the LCD's number of columns and rows: 
  Serial.begin(9600);                // Serial port
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);              // SD Card select pin
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
  Serial.println("Card failed, or not present");  // don't do anything more:
    lcd.setCursor(0,0);
    lcd.print("SD Card failed,");
    lcd.setCursor(0,1);
    lcd.print("or not present");
    delay(15000);   
      } else {
#ifdef TextDebug     
  Serial.println("SD card initialized succesfully.");
#endif TextDebug
      }
      
  NextMinute = int(RealTimeClock().charAt(15)); // Get the current minute
  NextMinute = NextMinute - 47;            // Convert from ASCII plus 1, to ensure we don't miss
     if(NextMinute > 9){ NextMinute = 0;}  // target needs to be reset to 0 after 9 minutes

#ifdef TextDebug  
  Serial.print("Numeric NextMinute = ");
  Serial.println(NextMinute);
#endif TextDebug

DataWrite("T,19.99,O,19.99,B,19.99,Time=,2012/01/01 00:00:00"); // Write a flag that we reinitilised
      
}

/*********** Main Loop ***************************************/
void loop() {
 
  String SDcardTxt = "";             // Initilise Data stirng
  
  for(int sensor = 0; sensor <=2; sensor++){  // a loop to cycle through the sensors
  float Temp = GetTemp(sensor);      // Call the temperature routing
  LCDprint(sensor, Temp);            // print the result on the LCD
  SDcardTxt += SenseName[sensor][2]; // Build up data for SDcard
  SDcardTxt += ",";
  SDcardTxt += dtostrf(Temp, 4, 2, s ); // Convert the float to String
  SDcardTxt += ",";  
  };

  String rtc = RealTimeClock();      // get the time
  
  SDcardTxt += "Time=,";             // Add the time to the SDcard data
  SDcardTxt += rtc;
  
  rtc = rtc.substring(11);           // Strip the date off the front
  
  lcd.print(" ");                    // Move to next field
  lcd.print(rtc);                    // print time (only)

  int minute = int(rtc.charAt(4));   // Get the current minute
  minute = minute - 48;              // convert from ASCII to integer 
  
#ifdef TextDebug  
  Serial.print("rtc = ");
  Serial.println(minute);
#endif TextDebug
  
  if (minute == NextMinute){         // see if this is a new minute
     DataWrite(SDcardTxt);           // Write to SD card

#ifdef TextDebug       
     Serial.println(SDcardTxt); 
#endif TextDebug

     NextMinute = minute + 1;        // Increment the next minute target
     if(NextMinute > 9){ NextMinute = 0;}  // target needs to be reset to 0 after 9 minutes
  }
}

/*********** SD card datalogger *********************************************
 ** SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK  - pin 13
 ** CS   - pin 4   Based on code by Tom Igoe
 */
void DataWrite(String data)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("TempLog.csv", FILE_WRITE);

  if (dataFile) {                 // if the file is available, write to it:
    dataFile.println(data);
    dataFile.close();
    
//    Serial.println(data);         // print to the serial port too:
       } else {                   // if the file isn't open, pop up an error:
    Serial.println("error opening TempLog.csv");
                 } 
}

/*********** Real Time Clock Routine *******************************************
 *  read/write serial interface to PCF8583 RTC via I2C interface
 *
 *  Arduino analog input 5 - I2C SCL (PCF8583 pin 6)
 *  Arduino analog input 4 - I2C SDA (PCF8583 pin 5)
 *
 *  You can set the type by sending it YYMMddhhmmss;
 *  the semicolon on the end tells it you're done...
 *
 ******************************************************************************/
String RealTimeClock(){

 if(Serial.available() > 0){
       p.year   = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48)) + 2000;
       p.month  = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48));
       p.day    = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48));
       p.hour   = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48));
       p.minute = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48));
       p.second = (byte) ((Serial.read() - 48) *10 + (Serial.read() - 48)); // Use of (byte) type casting and ascii math to achieve result.  

       if(Serial.read() == ';'){                // Get new time from Serial port
         Serial.println("setting date");        // Inform of time reset
	 p.set_time();                          // Set the time
       }
  }

  p.get_time();
  char time[50];
  sprintf(time, "%02d/%02d/%02d %02d:%02d:%02d", p.year, p.month, p.day, p.hour, p.minute, p.second);

#ifdef TimeDebug
  Serial.print("The time is : ");
  Serial.println(time);
#endif TimeDebug

  return(time);  

}

/*********** LCD Print Routine ********************************/
void LCDprint(int sensor, float temp){
  lcd.setCursor(SenseLCD[sensor][0],SenseLCD[sensor][1]);  // set the cursor start location
  lcd.print(SenseName[sensor][2]);                         // Print the sensor name (Letter)
  lcd.print(" "); 
//  if(int(temp) < 10){ lcd.print(" "); }                    
  if(int(temp) < 10 && temp > 0.0 ){                       // Check if we have single digits
     lcd.print(" "); 
     }                                                     // Add a space if we have single digits 
  lcd.print(temp);                                         // Print the temp passed to us.  
}

/*********** Dallas One Wire Temp Read Routine ****************/
float GetTemp(int sensor){
#ifdef TempDebug  
  Serial.print("Started \n");
  Serial.print("Sensor = ");
  Serial.println(sensor);
#endif TempDebug
  
  // Variables
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;   // OneWire Variables
  int i = 0;
  byte mydata[12];
  byte present = 0;
  float OutPutTemp = 0;
  
  // Get the temperature of the pin
  TempPin.reset();                           // Reset the network
  TempPin.select(Sense[sensor]);             // Select the sensor we want
  TempPin.write(0x44);                       // start temp conversion
  delay(800);                                // maybe 750ms is enough, maybe not
  present = TempPin.reset();                 // Reset the network
  TempPin.select(Sense[sensor]);             // Select the sensor we want
  TempPin.write(0xBE);                       // Read Scratchpad
 
 #ifdef TempDebug 
  Serial.print("Presence = ");
  Serial.print(present,HEX);
  Serial.println(" ");
 #endif TempDebug
 
    for ( i = 0; i < 9; i++) {                // we need 9 bytes
    mydata[i] = TempPin.read();               // Read the Onewire serial data
    
#ifdef TempDebug
    Serial.print(mydata[i], HEX);
    Serial.print(":");
#endif TempDebug    
    }
   
  // Convert binary data to usable format.   
   LowByte = mydata[0];
  HighByte = mydata[1];
  TReading = (HighByte << 8) + LowByte;      // Get the digital value of the temp
  SignBit  = TReading & 0x8000;              // test most sig bit
  if (SignBit)                               // negative
  { TReading = (TReading ^ 0xffff) + 1;  }   // 2's comp
  
//  OutPutTemp = float(Tc_100) / 100.00;        // Convert to floating point temp
  OutPutTemp = float(TReading) *0.0625;         // Convert to floating point temp  
  if (SignBit){ OutPutTemp = OutPutTemp * -1;}  // make sure it's correctly a negative temp


#ifdef TempDebug
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25
  Whole = Tc_100 / 100;                      // separate off the whole and fractional portions
  Fract = Tc_100 % 100;
Serial.print("\nTReading = ");
Serial.println(TReading);
Serial.print("Tc_100 = ");
Serial.println(Tc_100);

Serial.print("Integer Temp = ");
  if (SignBit)                               // If its negative
  {  Serial.print("-"); }
  Serial.print(Whole);
  Serial.print(".");
  if (Fract < 10)
  {  Serial.print("0"); }
  Serial.print(Fract);
  
 Serial.print("\nOutPutTemp = ");
 Serial.println(OutPutTemp);   
 Serial.print("Stopped \n\n");     
    
    delay(1000);
    
#endif TempDebug   

  return(OutPutTemp);
  }
  

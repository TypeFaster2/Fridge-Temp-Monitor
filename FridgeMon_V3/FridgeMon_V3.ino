/****************************************************************
* Fridge tempreture monitor and logger.                         *
* Sensors in three frdige zones and Sensor outside              *
* LCD Display Ethernet access                                   *
* Logging with RTC                                              *
*                                                               *
* http://typefaster.tumblr.com/                                 *........................................................................
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
    Removed the space in front of the time field
V3: Trimmed memory down to work with IDE 1.0.5, mostly by using
       nnn.print(F("blah blah)).
    Dropped unsed strings from SenseName
    Added free memory checking routine 
    Changed pin assigments so will work on Uno or Leonardo with Ethernet shield    
*/

// Debug Code. Uncomment these to make active
// These pump a fair amount of text to the serial port
// #define TempDebug
// #define TimeDebug
// #define TextDebug
// #define Memory

// Libraries
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <Wire.h>         // necessary, or the application won't build properly
#include <stdio.h>
#include <PCF8583.h>      // Note that I've modified the standard library to cope with V1.0
#include <SD.h>           // SD Card library
#include <avr/pgmspace.h>

// Global variables
// LiquidCrystal lcd(7,8,9,10,11,12);  // Initialise LCD
// LiquidCrystal (rs, enable, d4, d5, d6, d7)
// Note SD Card uses  pins  4, 11, 12, 13
// Note Ethernet uses pins 10, 11, 12, 13
   LiquidCrystal lcd(A0,A1,5,6,7,8);        // Initialise LCD
OneWire TempPin(A3);                   // Initialise OneWire on pin 2
PCF8583 p (0xA0);                      // RTC Address

// dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);
char s[32];                            // buffer for dtostrf function

// On the Ethernet Shield, CS is pin 10. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;            // Pin 4 on the Freetronics Etherten board
                                                                                                  
/* String SenseName[3][3] = { {"Top", "Top",    "T"}, // Sensor names
                            {"Out", "Outside","O"},
                            {"Bot", "Bottom", "B"} };
*/
 String SenseName[3][1] = { {"T"}, // Sensor names
                            {"O"},
                            {"B"} };

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
  Serial.println(F("Card failed, or not present"));  // don't do anything more:
    lcd.setCursor(0,0);
    lcd.print(F("SD Card failed,"));
    lcd.setCursor(0,1);
    lcd.print(F("or not present"));
    delay(15000);   
      } else {
#ifdef TextDebug     
  Serial.println(F("SD card initialized succesfully."));
#endif TextDebug
      }
      
  NextMinute = int(RealTimeClock().charAt(15)); // Get the current minute
  NextMinute = NextMinute - 47;            // Convert from ASCII plus 1, to ensure we don't miss
     if(NextMinute > 9){ NextMinute = 0;}  // target needs to be reset to 0 after 9 minutes

#ifdef TextDebug  
  Serial.print(F("Numeric NextMinute = "));
  Serial.println(NextMinute);
#endif TextDebug

#ifdef Memory
  Serial.print(F("  SetUp Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(F(" "));
#endif Memory

 DataWrite("Init"); // Write a flag that we reinitilised
      
}

/*********** Main Loop ***************************************/
void loop() {
 
  String SDcardTxt = "";             // Initilise Data stirng
  
  for(int sensor = 0; sensor <=2; sensor++){  // a loop to cycle through the sensors
  float Temp = GetTemp(sensor);      // Call the temperature routing
  LCDprint(sensor, Temp);            // print the result on the LCD
  SDcardTxt += SenseName[sensor][0]; // Build up data for SDcard
  SDcardTxt += ",";
  SDcardTxt += dtostrf(Temp, 4, 2, s ); // Convert the float to String
  SDcardTxt += ",";  
  };

  String rtc = RealTimeClock();      // get the time
  
  SDcardTxt += "Time=,";             // Add the time to the SDcard data
  SDcardTxt += rtc;
  
  rtc = rtc.substring(11);           // Strip the date off the front
  
  lcd.print(F(" "));                    // Move to next field
  lcd.print(rtc);                    // print time (only)

  int minute = int(rtc.charAt(4));   // Get the current minute
  minute = minute - 48;              // convert from ASCII to integer 
  
#ifdef TextDebug  
  Serial.print(F("rtc = "));
  Serial.println(minute);
#endif TextDebug

#ifdef Memory
  Serial.print(F(" Main Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(F(" "));
#endif Memory
  
  if (minute == NextMinute){         // see if this is a new minute
     DataWrite(SDcardTxt);           // Write to SD card

#ifdef TextDebug   
     Serial.print(F("SDcardTxt = "));
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
    
#ifdef TextDebug   
     Serial.print(F("Entered DataWrite = "));
     Serial.println(data); 
#endif TextDebug  
  
  File dataFile = SD.open("TempLog.csv", FILE_WRITE);
//    dataFile = SD.open("TempLog.csv", FILE_WRITE);

#ifdef TextDebug   
     Serial.println(F("File opened. Now test to see if we can write."));
#endif TextDebug  

  if (dataFile) {                 // if the file is available, write to it:
    dataFile.println(data);
    dataFile.close();
    
#ifdef TextDebug   
     Serial.println(F("Looks like we wrote to it and have now closed it."));
#endif TextDebug 
    
    Serial.println(data);         // print to the serial port too:
       } else {                   // if the file isn't open, pop up an error:
    Serial.println(F("error opening TempLog.csv"));
                 } 

#ifdef Memory
  Serial.print(F(" SD Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(F(" "));
#endif Memory                               
                 
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
         Serial.println(F("setting date"));        // Inform of time reset
	 p.set_time();                          // Set the time
       }
  }

  p.get_time();
  char time[50];
  sprintf(time, "%02d/%02d/%02d %02d:%02d:%02d", p.year, p.month, p.day, p.hour, p.minute, p.second);

#ifdef TimeDebug
  Serial.print(F("The time is : "));
  Serial.println(time);
#endif TimeDebug

#ifdef Memory
  Serial.print(F("  RTC Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(" ");
#endif Memory

  return(time);  

}

/*********** LCD Print Routine ********************************/
void LCDprint(int sensor, float temp){
  lcd.setCursor(SenseLCD[sensor][0],SenseLCD[sensor][1]);  // set the cursor start location
  lcd.print(SenseName[sensor][0]);                         // Print the sensor name (Letter)
  lcd.print(F(" ")); 
                  
  if(int(temp) < 10 && temp > 0.0 ){                       // Check if we have single digits
     lcd.print(F(" ")); 
     }                                                     // Add a space if we have single digits 
  lcd.print(temp);                                         // Print the temp passed to us.  
  
  #ifdef Memory
  Serial.print(F("  LCD Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(F(" "));
#endif Memory
  
}

/*********** Free RAM calculation Routine *********************/
int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

/*********** Dallas One Wire Temp Read Routine ****************/
float GetTemp(int sensor){
#ifdef TempDebug  
  Serial.print(F("Started \n"));
  Serial.print(F("Sensor = "));
  Serial.println(sensor);
#endif TempDebug
  
  // Variables
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;   // OneWire Variables
  int i = 0;
  byte mydata[12];
  byte present = 0;
  float OutPutTemp = 0;
  
 // OneWire Sensor addresses. There are the three I made into modules
 // Use FridgeMon_Sensor_serial to obtain the ROM code for one wire sensors (R=xxx) 
 byte Sense[3][8] = { {0x28, 0x57, 0x8B, 0x9E, 0x03, 0x00, 0x00, 0x1E},
                      {0x28, 0xAD, 0x76, 0x9E, 0x03, 0x00, 0x00, 0xE9},
                      {0x28, 0xB8, 0x76, 0x9E, 0x03, 0x00, 0x00, 0x59} };
  
  // Get the temperature of the pin
  TempPin.reset();                           // Reset the network
  TempPin.select(Sense[sensor]);             // Select the sensor we want
  TempPin.write(0x44);                       // start temp conversion
  delay(800);                                // maybe 750ms is enough, maybe not
  present = TempPin.reset();                 // Reset the network
  TempPin.select(Sense[sensor]);             // Select the sensor we want
  TempPin.write(0xBE);                       // Read Scratchpad
 
 #ifdef TempDebug 
  Serial.print(F("Presence = "));
  Serial.print(present,HEX);
  Serial.println(F(" "));
 #endif TempDebug
 
    for ( i = 0; i < 9; i++) {                // we need 9 bytes
    mydata[i] = TempPin.read();               // Read the Onewire serial data
    
#ifdef TempDebug
    Serial.print(mydata[i], HEX);
    Serial.print(F(":"));
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
Serial.print(F("\nTReading = "));
Serial.println(TReading);
Serial.print(F("Tc_100 = "));
Serial.println(Tc_100);

Serial.print(F("Integer Temp = "));
  if (SignBit)                               // If its negative
  {  Serial.print(F("-")); }
  Serial.print(Whole);
  Serial.print(F("."));
  if (Fract < 10)
  {  Serial.print(F("0")); }
  Serial.print(Fract);
  
 Serial.print(F("\nOutPutTemp = "));
 Serial.println(OutPutTemp);   
 Serial.print(F("Stopped \n\n"));     
    
    delay(1000);
    
#endif TempDebug  

#ifdef Memory
  Serial.print(F(" Temp Free Memory = "));
  int mem = freeRam();
  Serial.println(mem);
  Serial.println(F(" "));
#endif Memory

  return(OutPutTemp);
  }
  

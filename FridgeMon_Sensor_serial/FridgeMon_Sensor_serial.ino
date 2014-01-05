/****************************************************************
* Fridge tempreture monitor and logger.
* Sensors in three frdige zones and Sensor outside
* LCD Display Ethernet access
* Logging with RTC
* 
* Use this to find the serial number of the sensors.
* Based on code from here:
* http://www.arduino.cc/playground/Learning/OneWire
*****************************************************************/

// Libraries
#include <LiquidCrystal.h>
#include <OneWire.h>

LiquidCrystal lcd(7,8,9,10,11,12);           // Initialise LCD
OneWire TempPin(2);                               // Initialise OneWire on pin 2

void setup() {
  
  lcd.begin(16, 2);                          // set up the LCD's number of columns and rows: 
  Serial.begin(9600);                        // Serial port

}

void loop() {
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;   // OneWire Variables
  
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  if ( !TempPin.search(addr)) {
      Serial.print("No more addresses.\n");
      Serial.print("\n");
      TempPin.reset_search();
      delay(10000);
      return;
      }

  Serial.print("R=");
  for( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      return;
  }

  TempPin.reset();
  TempPin.select(addr);
  TempPin.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a TempPin.depower() here, but the reset will take care of it.

  present = TempPin.reset();
  TempPin.select(addr);    
  TempPin.write(0xBE);         // Read Scratchpad
  Serial.println(" ");
  
  Serial.print("P=");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = TempPin.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print( OneWire::crc8( data, 8), HEX);
  Serial.println();
  
  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;


  if (SignBit) // If its negative
  {
     Serial.print("-");
  }
  Serial.print(Whole);
  Serial.print(".");
  if (Fract < 10)
  {
     Serial.print("0");
  }
  Serial.print(Fract);

  Serial.print("\n");
  Serial.print("\n");
  
  
  
  
}

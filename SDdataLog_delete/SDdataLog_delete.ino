#include <SD.h>

File dataFile;

// String targetFile = "TempLog.csv";

void setup()
{
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(10, OUTPUT);

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  if (SD.exists("TempLog.csv")) {
    Serial.println("TempLog.csv exists.");
  }
  else {
    Serial.println("TempLog.csv doesn't exist.");
  }

  // open a new file and immediately close it:
  Serial.println("Creating or Opening TempLog.csv...");
  dataFile = SD.open("TempLog.csv", FILE_WRITE);
  dataFile.close();

  // Check to see if the file exists: 
  if (SD.exists("TempLog.csv")) {
    Serial.println("TempLog.csv exists.");
  }
  else {
    Serial.println("TempLog.csv doesn't exist.");  
  }

  // delete the file:
  Serial.println("Removing TempLog.csv...");
  SD.remove("TempLog.csv");

  if (SD.exists("TempLog.csv")){ 
    Serial.println("TempLog.csv exists.");
  }
  else {
    Serial.println("TempLog.csv doesn't exist.");  
  }
}

void loop()
{
  // nothing happens after setup finishes.
}


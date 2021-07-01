/*
    Description

    By
    Mads Rosenhoej Jeppesen - Aarhus 2021
    mrj@mpe.au.dk

    Query data from Drill Logger SD card
*/


#include <SD.h>

const int chipSelect = 10;
bool systemActive;
bool newCommand = false;
String command;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) systemActive = false;
  else systemActive = true;

  Serial.println("initialization done.");

}

void loop() {
  recvWithEndMarker();

  delay(250);
}

void printDirectory(File dir, int numTabs) {

  while (true) {
    File entry =  dir.openNextFile();

    if (! entry) {
      // no more files
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }

    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// Receive Commands
void recvWithEndMarker() {
  while (Serial.available())
  {
    char c = Serial.read();
    //Serial.print(c);              // Uncomment to see command string flow
    if (c == '<')
    {
      newCommand = true;
    }
    else if (c == '>' && newCommand == true)
    {
      newCommand = false;
      parseCommand(command);
      command = "";
    }
    else if (newCommand == true)
    {
      command += c;
    }
  }
}

// Command handler
void parseCommand(String com)
{
  String part1;
  String part2;
  File file;

  part1 = com.substring(0, com.indexOf('.'));
  part2 = com.substring(com.indexOf('.') + 1);

  if (part1.equalsIgnoreCase("query")) {
    if (part2.equalsIgnoreCase("all")) {
      if(systemActive){
        file = SD.open("/");
        printDirectory(file, 0);
        Serial.println("done!");
        file.close();
      }
      else Serial.println("SD card connection Error!");
    }
    else {
      Serial.println("NACK");
    }
  }

  else if (part1.equalsIgnoreCase("size")) {
    // part 2 contains file name
    if(systemActive){
        Serial.println("Opening file: " + part2 + ".txt");
        file = SD.open(part2 + ".txt");
        if(file){
            Serial.println("File size: " + (String)file.size() + " bytes");
            file.close();
        }
        else Serial.println("File not found!");
    }
    else Serial.println("SD card connection Error!");
  }
  else if (part1.equalsIgnoreCase("download")) {
    if(systemActive){
        Serial.println("Opening file: " + part2 + ".txt");
        file = SD.open(part2 + ".txt");
        if(file) {
            while(file.available()){
                Serial.write(file.read());    
            }
            file.close();
        }
        else Serial.println("File not found!");
    }
    else Serial.println("SD card connection Error!");
  }
  else Serial.println("NACK");
}

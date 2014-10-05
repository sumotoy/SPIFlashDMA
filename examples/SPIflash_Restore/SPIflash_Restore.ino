// SPIFlashDMA_Restore (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will program the flash chip with the
// contents of a dump file from the SD card.
//

// Enable (1) or disable (0) VT100 terminal mode
// Enable this if your terminal program supports VT100 control sequences.
// The Serial Monitor in the Arduino IDE does not support VT100 control sequences. 
// If using the Serial Monitor the line ending should be set to 'Both NL & CR'.
#define VT100_MODE  0

#include <SPIFlashDMA.h>
#include <SPI.h>
#include <SD.h>

const int SD_chipSelect = 6;      // Arduino Mega: 53 - Arduino Due: 53
const int FLASH_chipSelect = 16;   // Arduino Mega: 45 - Arduino Due: 52

SPIFlashDMA myFlash(FLASH_chipSelect); 

String  filename = "";
String  line = "";
boolean filenameOK = false;
File    dataFile;
long    lines = 0;
char    b64buf[344];
char    incoming = 0;
char    filenameca[15];
char    hexbuf[5];  

void base64decode()
{
  int fc = 0, tc = 0;
  
  for (int i=0; i<344; i++)
  {
    if(b64buf[i] >='A' && b64buf[i] <='Z') b64buf[i] = b64buf[i] - 'A';
    else if(b64buf[i] >='a' && b64buf[i] <='z') b64buf[i] = b64buf[i] - 71;
    else if(b64buf[i] >='0' && b64buf[i] <='9') b64buf[i] = b64buf[i] + 4;
    else if(b64buf[i] == '+') b64buf[i] = 62;
    else if(b64buf[i] == '/') b64buf[i] = 63;
  }
    
  while (fc<344)
  {
    myFlash.buffer[tc]   = (b64buf[fc] << 2) + ((b64buf[fc+1] & 0x30) >> 4);
    myFlash.buffer[tc+1] = ((b64buf[fc+1] & 0xf) << 4) + ((b64buf[fc+2] & 0x3c) >> 2);
    myFlash.buffer[tc+2] = ((b64buf[fc+2] & 0x3) << 6) + b64buf[fc+3];
    fc += 4;
    tc += 3;
  }
}

String readlnSD()
{
  String tmp = "";
  char ch;
  
  ch = dataFile.read();
  while ((ch != 10) and (ch != 13))
  {
    tmp += ch;
    ch = dataFile.read();
  }
  ch = dataFile.peek();
  while ((ch == 10) or (ch == 13))
  {
    ch = dataFile.read();
    ch = dataFile.peek();
  }  
  return tmp;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {;}


  Serial.println("SPIFlashDMA Dumpfile Restore");
  Serial.println("-------------------------");
  Serial.println();
  
  Serial.print("Initializing SD card... ");        // Must be done before initializing the Flash chip
  
  if (!SD.begin(SD_chipSelect))
  {
    Serial.println("Card failed, or not present");
    while (true) {};
  }
  Serial.println("Card initialized.");
  
  Serial.print("Initializing Flash chip... ");
  myFlash.begin();
  if (myFlash.ID_device==0)
  {
    Serial.print("Unsupported chip, or not present (0x");
    Serial.print(myFlash.ID_device, HEX);
    Serial.println(")");
    while (true) {};
  }
  Serial.println("Chip initialized.");

  while (!filenameOK)
  {
    Serial.println();
    Serial.print("Enter a filename to restore (8 characters maximum, no extension): ");
    filename = "";
    while (incoming != 13)
    {
      while (!Serial.available()) {};
      incoming = Serial.read();
      if ((incoming != 10) and (incoming != 13))
        filename += incoming;
    }
    
    filename.toUpperCase();
    filename += ".SFD";
    filename.toCharArray(filenameca, 15);
    Serial.println(filename);
    if (filename.length()>12)
    {
      Serial.println("The filename you entered is too long. Please try again...");
      incoming = 0;
    }
    else
      if (!SD.exists(filenameca))
      {
        Serial.println("That file does not exist on the SD card. Please try again...");
        incoming = 0;
      }
      else
        filenameOK = true;
  }
}

void loop()
{
  int udfreq = 0x100;
  
  dataFile = SD.open(filenameca);

  if (dataFile)
  {
    line = readlnSD();
    while (line.startsWith("//"))
      line = readlnSD();
    lines = line.toInt();
    if (lines>myFlash.Pages)
    {
      Serial.println();
      Serial.println("Connected flash chip is too small to accept image. Aborting...");
      while (true) {};
    }

    Serial.print("Waiting for flash chip to be ready... ");
    myFlash.waitForReady();
    Serial.println("Chip is ready.");

    Serial.print("Erasing chip... ");
    myFlash.eraseChip();
    Serial.println("Done.");

    Serial.println();
    Serial.println("Starting restore...");
    Serial.println();

    Serial.print("Number of pages to be written: 0x");
    sprintf(hexbuf, "%04X", (lines+1));
    Serial.println (hexbuf);
    if (VT100_MODE) {Serial.println(); udfreq = 0x10;}

    for (long page = 0; page<=lines; page++)
    {
      line = readlnSD();
      if (((page % udfreq) == 0) or (page==lines))
      {
        if (VT100_MODE) {Serial.write(0x1B); Serial.print("[A");}
        Serial.print("Writing page: 0x");
        sprintf(hexbuf, "%04X", page);
        Serial.println(hexbuf);
      }
      line.toCharArray(b64buf, 344);
      base64decode();
      myFlash.writePage(page);
    }
    dataFile.close();
    Serial.println("Finished!");
  }  
  else {
    Serial.println("error opening file");
  } 
  
  while (true) {};
}


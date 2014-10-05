// SPIFlashDMA_Backup (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will dump the contents of the flash chip to
// the SD card.
//
// You can select if you want to dump the file system only
// or if you want to dump the contents of the entire chip.
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

String     filename = "";
char       incoming = 0;
boolean    filenameOK = false;
uint8_t    b64buf[344];
const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char       filenameca[15];
char       hexbuf[5];
char       backupType;
boolean    typeOK = false;
uint32_t   backupPages;
uint16_t   fileid = 0;
uint32_t   filesize = 0;
uint8_t    reservedPages = 0x20;
uint16_t   usedPages = 0;

void base64encode()
{
  int fc = 0, tc = 0;
  
  while (fc<255)
  {
    b64buf[tc]   = b64[(myFlash.buffer[fc] & 0xfc) >> 2];
    b64buf[tc+1] = b64[((myFlash.buffer[fc] & 0x03) << 4) + ((myFlash.buffer[fc+1] & 0xf0) >> 4)];
    b64buf[tc+2] = b64[((myFlash.buffer[fc+1] & 0x0f) << 2) + ((myFlash.buffer[fc+2] & 0xc0) >> 6)];
    b64buf[tc+3] = b64[(myFlash.buffer[fc+2] & 0x3f)];
    fc += 3;
    tc += 4;
  }
  b64buf[tc]   = b64[(myFlash.buffer[fc] & 0xfc) >> 2];
  b64buf[tc+1] = b64[((myFlash.buffer[fc] & 0x03) << 4) + ((myFlash.buffer[fc+1] & 0xf0) >> 4)];
  b64buf[tc+2] = '=';
  b64buf[tc+3] = '=';
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) {;}
  Serial.println("SPIFlashDMA Backup");
  Serial.println("---------------");
  Serial.println();
  
  Serial.print("Initializing SD card... ");
  pinMode(SS, OUTPUT);
  
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
    Serial.print("Enter a filename for the backup (8 characters maximum, no extension): ");
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
      if (SD.exists(filenameca))
      {
        Serial.println("A file with that name already exists. Overwriting is not supported. Please try again...");
        incoming = 0;
      }
      else
        filenameOK = true;
  }
  
  while (Serial.available())
    incoming = Serial.read();
  incoming = 0;

  while (!typeOK)
  {
    Serial.println();
    Serial.print("Type of backup ([E]ntire chip / [F]ilesystem): ");
    while (incoming != 13)
    {
      while (!Serial.available()) {};
      incoming = Serial.read();
      if ((incoming != 10) and (incoming != 13))
        backupType = incoming;
    }
    
    if(backupType >='a' && backupType <='z') backupType -= 32;
    Serial.println(backupType);
    if ((backupType != 'E') and (backupType != 'F'))
    {
      Serial.println("Invalid choice. Please try again...");
      incoming = 0;
    }
    else
      typeOK = true;
  }
  
  while (Serial.available())
    incoming = Serial.read();
  incoming = 0;
  
  if (backupType == 'E')
    backupPages = myFlash.Pages;
  else
  {
    while (filesize != ERROR_FILE_DOES_NOT_EXIST)
    {
      filesize = myFlash.getFileSize(fileid);
      if (filesize != ERROR_FILE_DOES_NOT_EXIST)
      {
        usedPages += filesize/256;
        if ((filesize % 256) != 0)
          usedPages++;
        fileid++;
      }
    }
    backupPages = usedPages + reservedPages;
  }  

  Serial.println();
  Serial.println("Starting backup...");
  Serial.println();
}

void loop()
{
  int udfreq = 0x100;
  
  File dataFile = SD.open(filenameca, FILE_WRITE);
  
  if (dataFile)
  {
    Serial.println("Writing file header");
    if (backupType == 'E')
      dataFile.println("// SPIFlashDMA raw image dump");
    else
      dataFile.println("// SPIFlashDMA filesystem image dump");
    dataFile.println("//");
    dataFile.print("// Originating chip: ");
    dataFile.print(myFlash.Text_manufacturer);
    dataFile.print(" -> ");
    dataFile.println(myFlash.Text_device);
    dataFile.println();
    dataFile.println(backupPages-1, DEC);
    
    Serial.print("Number of pages to be read: 0x");
    sprintf(hexbuf, "%04X", backupPages);
    Serial.println (hexbuf);
    if (VT100_MODE) {Serial.println(); udfreq = 0x10;}

    for (long page=0; page<backupPages; page++)
    {
      myFlash.readPage(page);
      base64encode();
      if (((page % udfreq) == 0) or (page==(backupPages-1)))
      {
        if (VT100_MODE) {Serial.write(0x1B); Serial.print("[A");}
        Serial.print("Dumping page: 0x");
        sprintf(hexbuf, "%04X", page);
        Serial.println(hexbuf);
      }
      dataFile.write(b64buf, sizeof(b64buf));
      dataFile.println();
    }
    dataFile.close();
    Serial.println("Finished!");
  }  
  else
  {
    Serial.print("error opening ");
    Serial.println(filename);
  }
  
  while (true) {};
}

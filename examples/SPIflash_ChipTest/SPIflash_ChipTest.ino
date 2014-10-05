// SPIFlashDMA_ChipTest (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will test the falsh chip.
//
// ****** WARNING ******
// Running this sketch will erase the entire chip.
//

#include <SPIFlashDMA.h>
#include <SPI.h>


SPIFlashDMA myFlash(16);

char strbuf[100];
int  ok = 0;
int  mainok = 0;
long maxpage;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {;}
  myFlash.begin();

}

void loop()
{
  if (myFlash.ID_device!=0)
  {
    Serial.print("The connected device is a ");
    Serial.print(myFlash.Text_device);
    Serial.print(", which is a ");
    Serial.print(myFlash.Capacity, DEC);
    Serial.print("Mbit ");
    Serial.print(myFlash.Text_type);
    Serial.print(" from ");
    Serial.print(myFlash.Text_manufacturer);
    Serial.println(".");
    Serial.println();
    Serial.println("****************************");
    Serial.print("Manufacturer ID : 0x");
    Serial.println(myFlash.ID_manufacturer, HEX);
    Serial.print("Memory type     : 0x");
    Serial.println(myFlash.ID_type, HEX);
    Serial.print("Device ID       : 0x");
    Serial.println(myFlash.ID_device, HEX);
    Serial.println("****************************");
    Serial.print("Number of pages : 0x");
    Serial.println(myFlash.Pages, HEX);
    Serial.println("****************************");
    Serial.println();
    maxpage = myFlash.Pages;
    Serial.print("Waiting for Flash chip to be ready... ");
    myFlash.waitForReady();
    Serial.println("Chip is ready.");

    Serial.println();
    Serial.println("Send any character to start the test...");
    while (!Serial.available()) {};
    while (Serial.available()) {Serial.read();};

    Serial.println();
    Serial.println("Erasing chip");
    myFlash.eraseChip();
    Serial.println();
    Serial.println("Writing pages");
    for (int page=0; page<maxpage; page++)
    {
      if ((page % 128) == 0)
      {
        if (page != 0) Serial.println();
        sprintf (strbuf, "0x%04X: ", page);
        Serial.print(strbuf);
      }
      for (int i=0; i<256; i++)
        myFlash.buffer[i] = (page % 256);
      myFlash.writePage(page);
      Serial.print(".");
    }
    Serial.println();
    Serial.println();
    Serial.println("Clearing buffer");
    for (int i=0; i<256; i++)
      myFlash.buffer[i]=0;
    Serial.println("Reading page (* = OK, - = Fault)");
    for (int page=0; page<maxpage; page++)
    {
      if ((page % 128) == 0)
      {
        if (page != 0) Serial.println();
        sprintf (strbuf, "0x%04X: ", page);
        Serial.print(strbuf);
      }
      ok = 0;
      myFlash.readPage(page);
      for (int i=0; i<256; i++)
        if (myFlash.buffer[i] != (page % 256))
        {
          ok = 1;
          mainok = 1;
        }
      if (ok==0)
        Serial.print("*");
      else
        Serial.print("-");
    }
    Serial.println();
    Serial.println();
    if (mainok==0)
      Serial.print("Chip is OK...");
    else
      Serial.print("Chip failed...");
    Serial.println();
    Serial.println();
    Serial.println("Erasing chip");
    myFlash.eraseChip();
    Serial.println("Done...");
  }
  else {
    Serial.println("The connected device is not supported!");
  }
  while(1) {};
}


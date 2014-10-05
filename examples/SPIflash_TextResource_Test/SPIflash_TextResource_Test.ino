// SPIFlashDMA_TextResource_Test (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will attempt to read the list of text
// resources from file #1 in the flash memory chip 
// file system.
//
// The chip must contain the SPIFlashDMA file system.
// This sketch was designed to work with the
// "File System Demo" (DEMO.SFD) dataset.
//

#include <SPIFlashDMA.h>



// Hardware SPI
// Remember to set the correct pin for your development board
// Parameter order: CE/SS
SPIFlashDMA myFlash(16);

char str[80];
uint16_t fileid = 1;
uint16_t resid = 0;
uint16_t result = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {;}
  Serial.println("Starting...");
  myFlash.begin();
}

void loop()
{
  if (myFlash.ID_device!=0)
  {
    result = myFlash.readTextResource(fileid, resid, str, sizeof(str));
    while (result != ERR_OUT_OF_RANGE)
    {
      Serial.print(resid, DEC);
      Serial.print(" -> ");
      Serial.println(str);
      resid++;
      result = myFlash.readTextResource(fileid, resid, str, sizeof(str));
    }
  }
  else
    Serial.println("Unknown flash device!");
  
  while(1){};
}



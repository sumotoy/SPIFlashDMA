// SPIFlashDMA_FileList (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will attempt to read the list of files
// from the flash memory chip.
//
// The chip must contain the SPIFlashDMA file system.
//
#include <SPI.h>
#include <SPIFlashDMA.h>



// Hardware SPI
// Remember to set the correct pin for your development board
// Parameter order: CE/SS
SPIFlashDMA myFlash(16);

uint16_t fileid = 0;
uint16_t filetype = 0;
uint32_t filesize = 0;
char buf[100];

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
    sprintf(buf, "The connected device is a %s, which is a %dMbit %s from %s.", myFlash.Text_device, myFlash.Capacity, myFlash.Text_type, myFlash.Text_manufacturer);
    Serial.println(buf);
    Serial.println();
    Serial.println("| ID\t| Filetype\t\t\t\t| Filesize\t| Note");
    Serial.println("|-------|---------------------------------------|---------------|---------------");

    while (filetype != ERR_FILE_DOES_NOT_EXIST)
    {
      filetype = myFlash.getFileType(fileid);
      if (filetype != ERR_FILE_DOES_NOT_EXIST)
      {
        Serial.print("| ");
        sprintf(buf, "%3d", fileid);
        Serial.print(buf);
        Serial.print("\t| ");
        switch (filetype)
        {
        case 1:
          Serial.print("Binary\t\t\t\t| ");
          break;
        case 2:
          Serial.print("Text\t\t\t\t\t| ");
          break;
        case 3:
          Serial.print("Text Resource\t\t\t\t| ");
          break;
        case 4:
          Serial.print("Color image\t\t\t\t| ");
          break;
        case 5:
          Serial.print("Monochrome image for color display\t| ");
          break;
        case 6:
          Serial.print("Monochrome image\t\t\t| ");
          break;
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
          Serial.print("Custom ");
          Serial.print(filetype-31, DEC);
          Serial.print("\t\t\t\t| ");
          break;
        default:
          Serial.print("<unknown>\t\t\t\t| ");
          break;
        }
        filesize = myFlash.getFileSize(fileid);
        sprintf(buf, "%13lu", filesize);
        Serial.print(buf);
        Serial.print("\t| ");
        myFlash.readFileNote(fileid, buf);
        if (buf[0] == 0)
          Serial.println("<no note>");
        else
          Serial.println(buf);
        fileid++;
      }
    }
    Serial.println("|-------|---------------------------------------|---------------|---------------");
  }
  else
    Serial.println("Unknown flash device!");
  
  while(1){};
}



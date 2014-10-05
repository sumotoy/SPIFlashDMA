// SPIFlashDMA_FlashUploader (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program must be running on your development
// board if you are trying to upload data directly
// from the "FlashUploader" PC application.
//
// Please note that uploading over a serial link 
// have been unstable during testing, and should 
// only be used if you cannot upload data via a
// SD card using the SPIFlashDMA_Restore sketch.
//
// It also seems that using software SPI to 
// communicate with the flash chip makes it even
// more ustable. The flash chip should therefore
// be connected to the hardware SPI port on your
// development board.
//
// Also note that serial transfer of data is SLOW.
//
#include <SPI.h>
#include <SPIFlashDMA.h>



// Hardware SPI
// Remember to set the correct pin for your development board
// Parameter order: CE/SS
SPIFlashDMA myFlash(16);

#define RESPONSE_ACK  "ACK"
#define RESPONSE_NAK  "NAK"

char input;
char MagicString[] = "FlUl0001";
uint8_t b1, b2, b3, b4, check1, check2;
long check, page;
char b64buf[344];
const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void sendBuffer()
{
  for (int c=0; c<344; c++)
  {
    Serial.write(b64buf[c]);
  }
}

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
  myFlash.begin();
}

void loop()
{
  if (myFlash.ID_device!=0)
  {
    Serial.print(MagicString);
    while (1)
    {
      if (Serial.available())
      {
        input=Serial.read();
        switch (input)
        {
          case 'I':
            Serial.print(myFlash.ID_manufacturer, HEX);
            Serial.print(myFlash.ID_type, HEX);
            Serial.print(myFlash.ID_device, HEX);
            break;
          case 'C':
            myFlash.eraseChip();
            Serial.print(RESPONSE_ACK);
            break;
          case 'U': // Upload page
            while (Serial.available()<4) {};
            b1=Serial.read();
            b2=Serial.read();
            b3=Serial.read();
            b4=Serial.read();
            page=(long(b1)<<24)+(long(b2)<<16)+(long(b3)<<8)+b4;
            Serial.print(RESPONSE_ACK);
            while (!Serial.available()) {};
            for (int i1=0; i1<256; i1+=32)
            {
              while (Serial.available()<32) {};
              for (int i2=0; i2<32; i2++)
              {
                b1 = Serial.read();
                myFlash.buffer[i1+i2]=b1;
              }
              Serial.print(RESPONSE_ACK);
            }
            while (!Serial.available()) {};
            check1 = Serial.read();    
            check = 0;
            for (int i=0; i<256; i++)
              check += myFlash.buffer[i];
            if ((check & 0xFF) == check1)
            {
              myFlash.writePage(page);
              Serial.print(RESPONSE_ACK);
            }
            else
            {
              Serial.print(RESPONSE_NAK);
            }
            break;
          case 'D': // Download page
            while (Serial.available()<4) {};
            b1=Serial.read();
            b2=Serial.read();
            b3=Serial.read();
            b4=Serial.read();
            page=(long(b1)<<24)+(long(b2)<<16)+(long(b3)<<8)+b4;
            delay(50);
            Serial.print(RESPONSE_ACK);
            myFlash.readPage(page);
            base64encode();
            delay(10);
            sendBuffer();
            break;
        }
      }
    }
  }
}



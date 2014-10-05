// SPIFlashDMA_FileOpen_Test (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program will attempt to open and close files
// in the flash memory chip file system.
// Please note that some tests are supposed to 
// return errors.
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
uint16_t result2 = 0;
uint16_t fh1, fh2, fh3;

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
    Serial.println(F("Trying to open file ID 100 (Should return ERR_FILE_DOES_NOT_EXIST)..."));
    fh1 = myFlash.fileOpen(100);
    Serial.print(F("--> Handle: "));
    printResult(fh1);

    Serial.println(F("Trying to open file ID 0..."));
    fh1 = myFlash.fileOpen(0);
    Serial.print(F("--> Handle: "));
    Serial.println(fh1, HEX);

    Serial.println(F("Trying to seek forward in file ID 0..."));
    result = myFlash.fileSeek(fh1, 10);
    Serial.print(F("--> Result: "));
    printResult(result);

    Serial.println(F("Trying to seek forward beyond end in file ID 0 (Should return ERR_SEEK_PAST_FILE_END)..."));
    result = myFlash.fileSeek(fh1, 10000);
    Serial.print(F("--> Result: "));
    printResult(result);

    Serial.println(F("Trying to seek back in file ID 0 (Should return ERR_SEEK_PAST_FILE_START)..."));
    result = myFlash.fileSeek(fh1, -50);
    Serial.print(F("--> Result: "));
    printResult(result);

    Serial.println(F("Trying to seek back to start in file ID 0..."));
    result = myFlash.fileSeek(fh1, 0);
    Serial.print(F("--> Result: "));
    printResult(result);

    Serial.println(F("Trying to read text from file ID 0..."));
    Serial.println(F("-------------------------------------------------------------------------------------------------"));
    result = 0;
    while (result != ERR_AT_EOF)
    {
      result = ERR_BUFFER_OVERFLOW;
      while (result == ERR_BUFFER_OVERFLOW)
      {
        result = myFlash.fileReadLn(fh1, str, sizeof(str));
        if (result == ERR_BUFFER_OVERFLOW)
          Serial.print (str);
        else
          Serial.println (str);
      }
    }
    Serial.println(F("-------------------------------------------------------------------------------------------------"));

    Serial.println(F("Trying to open file ID 1 (Should return ERR_FILETYPE_INCORRECT)..."));
    fh2 = myFlash.fileOpen(1);
    Serial.print(F("--> Handle: "));
    printResult(fh2);

    Serial.println(F("Trying to open file ID 0 again (Should return ERR_FILE_ALREADY_OPEN)..."));
    fh3 = myFlash.fileOpen(0);
    Serial.print(F("--> Handle: "));
    printResult(fh3);

    Serial.println(F("Closing file handle fh1..."));
    Serial.print(F("--> Result: "));
    printResult(myFlash.fileClose(fh1));

    Serial.println(F("Closing file handle fh2 (Should return ERR_FILE_NOT_OPEN)..."));
    Serial.print(F("--> Result: "));
    printResult(myFlash.fileClose(fh2));

    Serial.println();
    Serial.println(F("All Done..."));
  }
  else
    Serial.println(F("Unknown flash device!"));
  
  while(1){};
}

void printResult(uint16_t res)
{
  switch (res)
  {
    case ERR_FILETYPE_INCORRECT:
      Serial.println (F("ERR_FILETYPE_INCORRECT"));
      break;
    case ERR_FILE_DOES_NOT_EXIST:
      Serial.println (F("ERR_FILE_DOES_NOT_EXIST"));
      break;
    case ERR_BUFFER_OVERFLOW:
      Serial.println (F("ERR_BUFFER_OVERFLOW"));
      break;
    case ERR_OUT_OF_RANGE:
      Serial.println (F("ERR_OUT_OF_RANGE"));
      break;
    case ERR_FILE_NOT_OPEN:
      Serial.println (F("ERR_FILE_NOT_OPEN"));
      break;
    case ERR_FILE_ALREADY_OPEN:
      Serial.println (F("ERR_FILE_ALREADY_OPEN"));
      break;
    case ERR_NO_AVAILABLE_HANDLES:
      Serial.println (F("ERR_NO_AVAILABLE_HANDLES"));
      break;
    case ERR_SEEK_PAST_FILE_START:
      Serial.println (F("ERR_SEEK_PAST_FILE_START"));
      break;
    case ERR_SEEK_PAST_FILE_END:
      Serial.println (F("ERR_SEEK_PAST_FILE_END"));
      break;
    case 0:
      Serial.println (F("OK"));
      break;
    default:
      Serial.print (F("Unknown code: "));
      Serial.println (res, HEX);
      break;
  }
}

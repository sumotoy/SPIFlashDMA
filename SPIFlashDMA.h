/*
  SPIFlashDMA.h - SPI flash chip Arduino and chipKit library
  Copyright (C)2014 Henning Karlsen. All right reserved
  
  This library provides basic support for handling SPI flash memory chips. It 
  also supports a very simple, read-only file system that can be used for 
  storing text files and text (string) resource files.

  You can find the latest version of the library at
	http://www.henningkarlsen.com/electronics

  If you make any modifications or improvements to the code, I would appreciate
  that you share the code with me so that I might include it in the next release.
  I can be contacted through http://www.henningkarlsen.com/electronics/contact.php

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.

  Commercial use of this library requires you to buy a license that
  will allow commercial use. This includes using the library,
  modified or not, as a tool to sell products.

  The license applies to all part of the library including the 
  examples and tools supplied with the library.
*/

#ifndef SPIFlashDMA_h
#define SPIFlashDMA_h

#if defined(__MK20DX128__) || defined(__MK20DX256__) //teensy 3
	#include "Arduino.h"
#endif

#define ERR_FILETYPE_INCORRECT		0xFFFF
#define ERR_FILE_DOES_NOT_EXIST		0xFFFE
#define ERR_BUFFER_OVERFLOW			0xFFFD
#define ERR_OUT_OF_RANGE			0xFFFC
#define ERR_FILE_NOT_OPEN			0xFFFB
#define ERR_FILE_ALREADY_OPEN		0xFFFA
#define ERR_NO_AVAILABLE_HANDLES	0xFFF9
#define ERR_SEEK_PAST_FILE_START	0xFFF8
#define ERR_SEEK_PAST_FILE_END		0xFFF7
#define ERR_AT_EOF					0xFFF6
#define ERR_NO_ERROR				0x0000

#define ERROR_FILE_DOES_NOT_EXIST	0xFFFFFFFE

#define PAGE_SIZE	256
#define MAX_FILEID	255
#define MAX_FILES	5

typedef struct
{
	uint16_t	fileid;
	uint32_t	filesize;
	uint8_t		filetype;
	uint32_t	start;
	uint32_t	position;
	uint16_t	imageXsize;
	uint16_t	imageYsize;
} __fileinfo;

class SPIFlashDMA
{
	public:
		uint8_t		ID_manufacturer, ID_type, ID_device;
		char		*Text_manufacturer, *Text_type, *Text_device;
		uint16_t	Capacity;
		uint32_t	Pages;
		uint8_t		buffer[256];

		SPIFlashDMA(uint8_t CE);
		void		begin();
		uint8_t		readStatus();
		void		readPage(uint32_t page);
		void		writePage(uint32_t page);
		void		waitForReady();
		void		eraseChip();

		uint16_t	fileOpen(uint16_t fileid);
		uint16_t	fileClose(uint8_t filehandle);
		uint16_t	fileSeek(uint8_t filehandle, int32_t offset);
		uint16_t	fileRead(uint8_t filehandle, char *buffer, uint16_t buflen);
		uint16_t	fileReadLn(uint8_t filehandle, char *buffer, uint16_t buflen);

		uint16_t	getFileType(uint16_t fileid);
		uint32_t	getFileSize(uint16_t fileid);
		uint16_t	readFileNote(uint16_t fileid, char *buffer);
		uint16_t	getImageXSize(uint16_t fileid);
		uint16_t	getImageYSize(uint16_t fileid);

		uint16_t	readTextResource(uint16_t fileid, uint16_t resid, char *buffer, uint16_t buflen);

protected:
		uint8_t		_pinCE;
		uint32_t	_max_pages;
		uint8_t		_cmd_write, _flags;
		__fileinfo	_fileinfo[MAX_FILES];

		uint8_t		_readByte();
		void		_setBlockProtection(uint8_t prot);
		void		_ID_Device();

private:
		void 		startSend();
		void 		endSend();
		void 		sendCommand(uint8_t cmd);
		void 		_SPIstart();
		byte 		_SPIread(void);
};

#endif
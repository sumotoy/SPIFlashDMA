/*
  SPIFlashDMA.cpp - SPI flash chip Arduino and chipKit library
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
#include <SPI.h>
#include "SPIFlashDMA.h"


#define WRITETYPE_WORD	0x0001
#define WRITETYPE_CONT	0x0002



struct _s_device{
  uint8_t	_manufacturer_id;
  uint8_t	_type_id;
  uint8_t	_device_id;
  uint16_t  _size;			// in Mbits
  uint32_t	_pages;			// highest page number available
  char* 	_manufacturer_name;
  char* 	_device_type;
  char* 	_device_name;
  uint8_t	_cmd_write;		// command for page write
  uint8_t	_flags;			
};
//W25Q128BV
const _s_device _device[]= {
  {0xBF, 0x25, 0x8C, 0x0002, 0x000003ff, "SST (Microchip)", "SPI Serial Flash", "SST25VF020B",	0xAD, WRITETYPE_WORD},
  {0xBF, 0x25, 0x8D, 0x0004, 0x000007ff, "SST (Microchip)", "SPI Serial Flash", "SST25VF040B",	0xAD, WRITETYPE_WORD},
  {0xBF, 0x25, 0x8E, 0x0008, 0x00000fff, "SST (Microchip)", "SPI Serial Flash", "SST25VF080B",	0xAD, WRITETYPE_WORD},
  {0xBF, 0x25, 0x41, 0x0010, 0x00001fff, "SST (Microchip)", "SPI Serial Flash", "SST25VF016B",	0xAD, WRITETYPE_WORD},
  {0xBF, 0x25, 0x4A, 0x0020, 0x00003fff, "SST (Microchip)", "SPI Serial Flash", "SST25VF032B",	0xAD, WRITETYPE_WORD},
  {0xBF, 0x25, 0x4B, 0x0040, 0x00007fff, "SST (Microchip)", "SPI Serial Flash", "SST25VF064C",	0x02, WRITETYPE_CONT},
  {0xEF, 0x40, 0x14, 0x0008, 0x00000fff, "Winbond",			"SPI Serial Flash", "W25Q08BV",		0x02, WRITETYPE_CONT},
  {0xEF, 0x40, 0x15, 0x0010, 0x00001fff, "Winbond",			"SPI Serial Flash", "W25Q16BV",		0x02, WRITETYPE_CONT},
  {0xEF, 0x40, 0x16, 0x0020, 0x00003fff, "Winbond",			"SPI Serial Flash", "W25Q32BV",		0x02, WRITETYPE_CONT},
  {0xEF, 0x40, 0x17, 0x0040, 0x00007fff, "Winbond",			"SPI Serial Flash", "W25Q64FV",		0x02, WRITETYPE_CONT},
  {0xEF, 0x40, 0x18, 0x0080, 0x0000ffff, "Winbond",			"SPI Serial Flash", "W25Q128BV/FV", 0x02, WRITETYPE_CONT},
  {0xC2, 0x20, 0x15, 0x0010, 0x00001fff, "MXIC",			"SPI Serial Flash",	"MX25L1605D",	0x02, WRITETYPE_CONT},
  {0xC2, 0x20, 0x16, 0x0020, 0x00003fff, "MXIC",			"SPI Serial Flash",	"MX25L3205D",	0x02, WRITETYPE_CONT},
  {0xC2, 0x20, 0x17, 0x0040, 0x00007fff, "MXIC",			"SPI Serial Flash",	"MX25L6405D",	0x02, WRITETYPE_CONT}
};

/****************************************************************************************/
/* Public                                                                               */
/****************************************************************************************/


SPIFlashDMA::SPIFlashDMA(uint8_t CE){
	_pinCE	= CE;
}



void SPIFlashDMA::begin(){
	_SPIstart();
	for (int c=0; c<256; c++) buffer[c]=0;
	for (int c=0; c<5; c++) _fileinfo[c].fileid=0xFFFF;;
	_ID_Device();
}

uint8_t SPIFlashDMA::readStatus() {
	uint8_t status;
	startSend();
	SPI.transfer(0x05);
	status = _readByte();
	endSend();
	return status;
}

void SPIFlashDMA::readPage(uint32_t page){
	if (page<=_max_pages){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer((page & 0x0000FFFF)>>8);
		SPI.transfer(page & 0x000000FF);
		SPI.transfer(0x00);
		SPI.transfer(0x00);
		for (int c=0; c<256; c++) buffer[c]=_readByte();
		endSend();
	}
}

void SPIFlashDMA::writePage(uint32_t page){
	if (page<=_max_pages){
		_setBlockProtection(0);

		startSend();
		SPI.transfer(0x06);
		endSend();

		startSend();
		SPI.transfer(_cmd_write);
		SPI.transfer((page & 0x0000FFFF)>>8);
		SPI.transfer(page & 0x000000FF);
		SPI.transfer(0x00);
		if (_flags & WRITETYPE_WORD){
			SPI.transfer(buffer[0]);
			SPI.transfer(buffer[1]);
			endSend();
			waitForReady();
			for (int c=2; c<256; c+=2){
				startSend();
				SPI.transfer(0xAD);
				SPI.transfer(buffer[c]);
				SPI.transfer(buffer[c+1]);
				endSend();
				waitForReady();
			}
		} else {
			for (int c=0; c<256; c++) SPI.transfer(buffer[c]);
			endSend();
			waitForReady();
		}
		startSend();
		SPI.transfer(0x04);
		endSend();
		_setBlockProtection(0x0f);
	}
}

void SPIFlashDMA::waitForReady(){
	while ((readStatus() & 0x01) == 0x01) {};
}

void SPIFlashDMA::eraseChip(){
	_setBlockProtection(0);
	startSend();
	SPI.transfer(0x06);
	endSend();

	startSend();
	SPI.transfer(0x60);
	endSend();

	waitForReady();
	_setBlockProtection(0x0f);
}

uint16_t SPIFlashDMA::fileOpen(uint16_t fileid)
{
	uint32_t	startpage;
	uint8_t		filehandle = 0xFF;
	uint8_t		c;
	uint32_t	tmp1, tmp2, tmp3, tmp4;

	if (fileid<MAX_FILEID){
		for (c = 0; c<MAX_FILES; c++) if (_fileinfo[c].fileid == fileid) return ERR_FILE_ALREADY_OPEN;
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();

		if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
		c = 0;
		while ((c < MAX_FILES) and (filehandle == 0xFF)){
			if (_fileinfo[c].fileid == 0xFFFF){
				filehandle = c;
				_fileinfo[filehandle].fileid = fileid;
				startSend();
				SPI.transfer(0x0B);
				SPI.transfer(0x00);
				SPI.transfer((((fileid * 32) + 2) & 0x0000FFFF)>>8);
				SPI.transfer(((fileid * 32) + 2) & 0x000000FF);
				SPI.transfer(0x00);
				_fileinfo[filehandle].filetype = _readByte();
				endSend();
				if (_fileinfo[filehandle].filetype == 3){
					_fileinfo[filehandle].fileid = 0xFFFF;
					return ERR_FILETYPE_INCORRECT;
				}
				startSend();
				SPI.transfer(0x0B);
				SPI.transfer(0x00);
				SPI.transfer((((fileid * 32) + 4) & 0x0000FFFF)>>8);
				SPI.transfer(((fileid * 32) + 4) & 0x000000FF);
				SPI.transfer(0x00);

				tmp1 = _readByte();
				tmp2 = _readByte();
				tmp3 = _readByte();
				tmp4 = _readByte();
				_fileinfo[filehandle].filesize = (tmp1<<24) + (tmp2<<16) + (tmp3<<8) + tmp4;
				endSend();
				_fileinfo[filehandle].start = startpage * PAGE_SIZE;
				_fileinfo[filehandle].position = startpage * PAGE_SIZE;
			} else {
				c++;
			}
		}
		if (filehandle == 0xFF) {
			return ERR_NO_AVAILABLE_HANDLES;
		} else {
			return filehandle;
		}
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
}

uint16_t SPIFlashDMA::fileClose(uint8_t filehandle){
	if ((_fileinfo[filehandle].fileid == 0xFFFF) or (filehandle >= MAX_FILES)) {
		return ERR_FILE_NOT_OPEN;
	} else {
		_fileinfo[filehandle].fileid = 0xFFFF;
		return ERR_NO_ERROR;
	}
}

uint16_t SPIFlashDMA::fileSeek(uint8_t filehandle, int32_t offset) {
	if (_fileinfo[filehandle].fileid == 0xFFFF) {
		return ERR_FILE_NOT_OPEN;
	} else {
		if (offset == 0) {
			_fileinfo[filehandle].position = _fileinfo[filehandle].start;
			return ERR_NO_ERROR;
		}
		if (offset < 0) {
			if ((_fileinfo[filehandle].position + offset) < _fileinfo[filehandle].start)
				return ERR_SEEK_PAST_FILE_START;
		} else {
			if ((_fileinfo[filehandle].position + offset) > (_fileinfo[filehandle].position + _fileinfo[filehandle].filesize)) return ERR_SEEK_PAST_FILE_END;
		}
		_fileinfo[filehandle].position += offset;
		return ERR_NO_ERROR;
	}
}

uint16_t SPIFlashDMA::fileRead(uint8_t filehandle, char *buffer, uint16_t buflen) {
	uint32_t bytesToRead,c;
	if (_fileinfo[filehandle].fileid == 0xFFFF) {
		return ERR_FILE_NOT_OPEN;
	} else {
		if (_fileinfo[filehandle].position == uint32_t(_fileinfo[filehandle].start + _fileinfo[filehandle].filesize)){
			buffer[0] = 0;
			return ERR_AT_EOF;
		}
		if (_fileinfo[filehandle].filetype == 2) {
			// Text file
			for (c=0; c<buflen; c++)
				buffer[c] = 0;
			if ((_fileinfo[filehandle].position + (buflen - 1)) > (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize)) {
				bytesToRead = (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize) - _fileinfo[filehandle].position;
			} else {
				bytesToRead = buflen - 1;
			}
			startSend();
			SPI.transfer(0x0B);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x00FF0000)>>16);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x0000FF00)>>8);
			SPI.transfer((_fileinfo[filehandle].position) & 0x000000FF);
			SPI.transfer(0x00);

			for (c=0; c<bytesToRead; c++) buffer[c]=_readByte();
			endSend();
			_fileinfo[filehandle].position += bytesToRead;
			return bytesToRead;
		} else {
			// Other files
			if ((_fileinfo[filehandle].position + buflen) > (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize)) {
				bytesToRead = (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize) - _fileinfo[filehandle].position;
			} else {
				bytesToRead = buflen;
			}
			startSend();
			SPI.transfer(0x0B);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x00FF0000)>>16);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x0000FF00)>>8);
			SPI.transfer((_fileinfo[filehandle].position) & 0x000000FF);
			SPI.transfer(0x00);
			for (c=0; c<bytesToRead; c++) buffer[c]=_readByte();
			endSend();
			_fileinfo[filehandle].position += bytesToRead;
			return bytesToRead;
		}
	}
}

uint16_t SPIFlashDMA::fileReadLn(uint8_t filehandle, char *buffer, uint16_t buflen){
	uint16_t	bytesToRead;
	boolean		foundEOL = false;
	if (_fileinfo[filehandle].fileid == 0xFFFF) {
		return ERR_FILE_NOT_OPEN;
	} else {
		if (_fileinfo[filehandle].filetype == 2) {	// Text files only
			if (_fileinfo[filehandle].position == (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize)) {
				buffer[0] = 0;
				return ERR_AT_EOF;
			}
			for (int c=0; c<buflen; c++) buffer[c] = 0;
			if ((_fileinfo[filehandle].position + (buflen - 1)) > (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize)) {
				bytesToRead = (_fileinfo[filehandle].start + _fileinfo[filehandle].filesize) - _fileinfo[filehandle].position;
			} else {
				bytesToRead = buflen - 1;
			}
			startSend();
			SPI.transfer(0x0B);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x00FF0000)>>16);
			SPI.transfer(((_fileinfo[filehandle].position) & 0x0000FF00)>>8);
			SPI.transfer((_fileinfo[filehandle].position) & 0x000000FF);
			SPI.transfer(0x00);
			int c = 0;
			while ((c < bytesToRead) and (foundEOL == false)){
				buffer[c]=_readByte();
				if (buffer[c] == 0x0A){
					foundEOL = true;
					buffer[c] = 0;
				}
				if (buffer[c] == 0x0D){
					foundEOL = true;
					buffer[c] = 0;
					char temp = _readByte();
					if (temp == 0x0A)
						c++;
				}
				c++;
			}
			endSend();
			_fileinfo[filehandle].position += c;
			if (c == bytesToRead) {
				return ERR_BUFFER_OVERFLOW;
			} else {
				return ERR_NO_ERROR;
			}
		} else {
			return ERR_FILETYPE_INCORRECT;
		}
	}
}

uint16_t SPIFlashDMA::getFileType(uint16_t fileid){
	uint16_t	startpage = 0xFFFF;
	uint8_t		tmp;
	if (fileid < MAX_FILEID){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 2) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 2) & 0x000000FF);
	SPI.transfer(0x00);
	tmp = _readByte();
	endSend();
	return tmp;
}

uint32_t SPIFlashDMA::getFileSize(uint16_t fileid){
	uint16_t	startpage = 0xFFFF;
	uint32_t	tmp;
	uint32_t	tmp1, tmp2, tmp3, tmp4;
	if (fileid < MAX_FILEID){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERROR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERROR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 4) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 4) & 0x000000FF);
	SPI.transfer(0x00);

	tmp1 = _readByte();
	tmp2 = _readByte();
	tmp3 = _readByte();
	tmp4 = _readByte();
	tmp = (tmp1<<24) + (tmp2<<16) + (tmp3<<8) + tmp4;
	endSend();
	return tmp;
}

uint16_t SPIFlashDMA::readFileNote(uint16_t fileid, char *buffer){
	uint16_t	startpage;
	uint8_t		tmp;
	if (fileid < MAX_FILEID) {
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 16) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 16) & 0x000000FF);
	SPI.transfer(0x00);
	for (int c=0; c<16; c++){
		tmp = _readByte();
		if (tmp != 0xFF) {
			buffer[c] = tmp;
		} else {
			buffer[c] = 0;
		}
	}
	buffer[16] = 0;
	endSend();
	return ERR_NO_ERROR;
}

uint16_t SPIFlashDMA::getImageXSize(uint16_t fileid){
	uint16_t	startpage = 0xFFFF;
	uint8_t		tmp;
	uint16_t	size = 0;
	if (fileid < MAX_FILEID){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 2) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 2) & 0x000000FF);
	SPI.transfer(0x00);
	tmp = _readByte();
	endSend();
	if ((tmp >= 4) and (tmp <=6)){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer((((fileid * 32) + 8) & 0x0000FFFF)>>8);
		SPI.transfer(((fileid * 32) + 8) & 0x000000FF);
		SPI.transfer(0x00);
		size = (_readByte()<<8) + _readByte();
		endSend();
		return size;
	} else {
		return ERR_FILETYPE_INCORRECT;
	}
}

uint16_t SPIFlashDMA::getImageYSize(uint16_t fileid) {
	uint16_t	startpage = 0xFFFF;
	uint8_t		tmp;
	uint16_t	size = 0;
	if (fileid < MAX_FILEID){
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 2) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 2) & 0x000000FF);
	SPI.transfer(0x00);
	tmp = _readByte();
	endSend();
	if ((tmp >= 4) and (tmp <=6)) {
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer((((fileid * 32) + 10) & 0x0000FFFF)>>8);
		SPI.transfer(((fileid * 32) + 10) & 0x000000FF);
		SPI.transfer(0x00);
		size = (_readByte()<<8) + _readByte();
		endSend();
		return size;
	} else {
		return ERR_FILETYPE_INCORRECT;
	}
}

uint16_t SPIFlashDMA::readTextResource(uint16_t fileid, uint16_t resid, char *buffer, uint16_t buflen) {
	uint16_t	startpage;
	uint8_t		filetype;
	uint16_t	linecount;
	uint32_t	offset;
	uint16_t	length;
	boolean		overflow = false;
	if (fileid < MAX_FILEID) {
		startSend();
		SPI.transfer(0x0B);
		SPI.transfer(0x00);
		SPI.transfer(((fileid * 32) & 0x0000FFFF)>>8);
		SPI.transfer((fileid * 32) & 0x000000FF);
		SPI.transfer(0x00);
		startpage = (_readByte()<<8) + _readByte();
		endSend();
	} else {
		return ERR_FILE_DOES_NOT_EXIST;
	}
	if (startpage == 0xFFFF) return ERR_FILE_DOES_NOT_EXIST;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer(0x00);
	SPI.transfer((((fileid * 32) + 2) & 0x0000FFFF)>>8);
	SPI.transfer(((fileid * 32) + 2) & 0x000000FF);
	SPI.transfer(0x00);
	filetype = _readByte();
	endSend();
	if (filetype != 3) return ERR_FILETYPE_INCORRECT;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer((startpage & 0x0000FFFF)>>8);
	SPI.transfer(startpage & 0x000000FF);
	SPI.transfer(0x00);
	SPI.transfer(0x00);
	linecount = (_readByte()<<8) + _readByte();
	endSend();
	if (resid > linecount-1) return ERR_OUT_OF_RANGE;
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer((((startpage * PAGE_SIZE) + (resid * 4) + 2) & 0x00FF0000)>>16);
	SPI.transfer((((startpage * PAGE_SIZE) + (resid * 4) + 2) & 0x0000FF00)>>8);
	SPI.transfer(((startpage * PAGE_SIZE) + (resid * 4) + 2) & 0x000000FF);
	SPI.transfer(0x00);
	offset = (_readByte()<<8) + _readByte();
	length = (_readByte()<<8) + _readByte();
	endSend();
	for (int c=0; c<buflen; c++) buffer[c]=0;
	if (buflen<(length+1)) {
		length = buflen-1;
		overflow = true;
	}
	startSend();
	SPI.transfer(0x0B);
	SPI.transfer((((startpage * PAGE_SIZE) + offset) & 0x00FF0000)>>16);
	SPI.transfer((((startpage * PAGE_SIZE) + offset) & 0x0000FF00)>>8);
	SPI.transfer(((startpage * PAGE_SIZE) + offset) & 0x000000FF);
	SPI.transfer(0x00);
	for (int c=0; c<length; c++) buffer[c]=_readByte();
	endSend();
	if (overflow == true) {
		return ERR_BUFFER_OVERFLOW;
	} else {
		return ERR_NO_ERROR;
	}
}

/****************************************************************************************/
/* Proctected                                                                           */
/****************************************************************************************/

uint8_t SPIFlashDMA::_readByte(){
	return _SPIread();
}


void SPIFlashDMA::_setBlockProtection(uint8_t prot){
	sendCommand(0x50);
	startSend();
	SPI.transfer(0x01);
	SPI.transfer((prot & 0x0f)<<2);
	endSend();
}

void SPIFlashDMA::_ID_Device(){
	uint8_t res1, res2, res3;
	uint32_t i;
	startSend();
	SPI.transfer(0x9F);
	res1=_readByte();
	res2=_readByte();
	res3=_readByte();
	endSend();
	ID_manufacturer=res1;
	ID_type=res2;
	ID_device=res3;
	Text_manufacturer = "xxx";
	for(i = 0; i<(sizeof(_device)/sizeof(struct _s_device)); i++){
		if ((_device[i]._manufacturer_id==res1) & (_device[i]._type_id==res2) & (_device[i]._device_id==res3)){
			Text_manufacturer	= _device[i]._manufacturer_name;
			Text_type			= _device[i]._device_type;
			Text_device			= _device[i]._device_name;
			Capacity			= _device[i]._size;
			Pages				= _device[i]._pages+1;
			_max_pages			= _device[i]._pages;
			_cmd_write			= _device[i]._cmd_write;
			_flags				= _device[i]._flags;
		}
	}
	if (Text_manufacturer == "xxx"){
		Text_manufacturer	= "(Unknown manufacturer)";
		Text_type			= "(Unknown type)";
		Text_device			= "(Unknown device)";
		Capacity			= 0;
		ID_manufacturer		= 0;
		ID_type				= 0;
		ID_device			= 0;
		_max_pages			= 0;
		_cmd_write			= 0;
		_flags				= 0;
	}
}

void SPIFlashDMA::_SPIstart(){
	pinMode(_pinCE, OUTPUT);
	digitalWrite(_pinCE, HIGH);
	SPI.begin();
#if defined(SPI_HAS_TRANSACTION)

#else//do not use SPItransactons
	SPI.setClockDivider(SPI_CLOCK_DIV4);//4Mhz
	SPI.setDataMode(SPI_MODE0);
#endif
}


byte SPIFlashDMA::_SPIread(void){
	uint8_t x = SPI.transfer(0x0);
	return x;
}

void SPIFlashDMA::sendCommand(uint8_t cmd){
	startSend();
	SPI.transfer(cmd);
	endSend();
}

/**************************************************************************/
/*! PRIVATE
		starts SPI communication
*/
/**************************************************************************/
void SPIFlashDMA::startSend(){
#if defined(SPI_HAS_TRANSACTION)
	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
#endif
#if defined(__MK20DX128__) || defined(__MK20DX256__)
	digitalWriteFast(_pinCE, LOW);
#else
	digitalWrite(_pinCE, LOW);
#endif
}

/**************************************************************************/
/*! PRIVATE
		ends SPI communication
*/
/**************************************************************************/
void SPIFlashDMA::endSend(){
#if defined(__MK20DX128__) || defined(__MK20DX256__)
	digitalWriteFast(_pinCE, HIGH);
#else
	digitalWrite(_pinCE, HIGH);
#endif
#if defined(SPI_HAS_TRANSACTION)
	SPI.endTransaction();
#endif
}
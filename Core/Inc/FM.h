/*
 * FM.h
 *
 *  Created on: May 9, 2022
 *      Author: Sushant
 */

/*************Begin includes*****************8*/
#include"main.h"
#include <stdio.h>
#ifndef INC_FM_H_
#define INC_FM_H_
/* End includes*/

/* Typedef Structures */
typedef struct RegisterStatus{
	uint8_t WPEN;
	uint8_t WEN;
	uint8_t RDY;
	uint8_t BP0;
	uint8_t BP1;
	uint8_t BP2;
}RegisterStatus;

/* Function prototypes declaration */

void FM_EN();
void FM_DISABLE();

void R_STAT(RegisterStatus *buffer);
void WriteReg(uint8_t instruction);
uint16_t FLASH_ID(uint8_t *buffer);

uint8_t Read_Byte(uint32_t address);
uint8_t Write_Byte(uint32_t address, uint8_t *wr_data);
void Write_Data(uint32_t address, uint8_t *data, int size);
uint8_t Read_Data(uint32_t address, uint8_t *buffer, int size);

uint8_t BulkRead(uint32_t address, uint8_t *buffer, int size);
uint8_t PageWrite(uint32_t address, uint8_t *buffer, int size);

uint8_t Sector_Number(uint32_t address);
uint8_t Pagenum(uint32_t *address);
uint32_t RemBytes(uint32_t *address);

void SectorErase(uint32_t *address);
void ChipErase();

/* End Functions*/


/*Starting address of corresponding sector*/
#define		SectorOne		0x00000000
#define		SectorTwo		0x00010000
#define		SectorThree		0x00020000
#define		SectorFour		0x00030000
#define		SectorFive		0x00040000
#define		SectorSix		0x00050000
#define		SectorSeven		0x00060000
#define		SectorEight		0x00070000
#define		SectorEnd		0x0007ffff
#define		PageSize		256				//size of one page is 256 bytes

/*Sector location End Includes*/

/* Commands of FM */
#define 	RDID			0b00010101			//to read ID of FM
#define 	WREN			0b00000110			//to enable write mode
#define  	RDSR			0b00000101			//to read status register
#define		READ			0b00000011			//to read the data form FM
#define		PROGRAM			0b00000010			//to write data into the FM
#define		WRSR			0b00000001			//to write the status register
#define		WRDI			0b00000100			//to disable write enable mode
#define		SECTOR_ERASE	0b01010010			//to erase the sector
#define		CHIP_ERASE		0b01100010			//to erase the chip
#endif
/* end of commands */

/* INC_FM_H_ */

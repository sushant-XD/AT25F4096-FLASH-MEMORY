/*
 * FM.c
 *
 *  Created on: May 9, 2022
 *      Author: Sushant
 */

/* ------------Includes -------------------------*/
#include "main.h"
#include "FM.h"
/*End includes*/

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

/*-----------------------------Functions ---------------------*/


/* @brief			Sets the write enable mode for FM to write into the FM
 *
 *  @param  		none
 */
static void Write_En() {
	uint8_t cmd = WREN;							//WREN - write mode enable
	FM_EN();									//CS pin low
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 50);
	FM_DISABLE();								//CS pin set high
}

/*
 * @brief			Function to reset write enable latch
 *
 * @param			none
 */
static void Write_Disable() {
	uint8_t cmd = WRDI; 					//WRDI is command of FM to disable write
	FM_EN();
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 50);
	FM_DISABLE();							//After every operation CS pin set high
}
/*
 * @brief		Reads ID of the flash memory
 *
 * @param		*buffer  (pointer) id of the flash memory that is returned
 *
 * @retval		none
 */
uint16_t FLASH_ID(uint8_t *buffer) {
	uint8_t command = RDID;
	FM_EN();
	HAL_Delay(1);  							//delay to make sure FM is enabled
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	HAL_SPI_Receive(&hspi1, buffer, 2, 500);
	FM_DISABLE();
	return *buffer;
}

/*
 * @brief		To write data of just one byte
 * @brief		AT25 only takes 24 bit address & bit shifting is done to send the address
 *
 * @param		addr		address of the FM to write the data
 * 				*wr_data	pointer to the data to be written into the address
 *
 * @retval
 * */
uint8_t Write_Byte(uint32_t addr, uint8_t *wr_data) {
	uint8_t data[5];
	RegisterStatus *wstatus = 0;
	RegisterStatus *status = 0;
	data[0] = PROGRAM & 0xff;
	data[1] = (uint8_t) (addr >> 16 & 0xFF);
	data[2] = (uint8_t) (addr >> 8 & 0xFF);
	data[3] = (uint8_t) (addr & 0xFF);
	data[4] = *wr_data;

	Write_En();
	R_STAT(status);

	//check for the register status to be write enabled
	while (status->WPEN != 0 && status->WEN != 1) {
		Write_En();
		R_STAT(status);
	}

	/**********************************************************************************/
	FM_EN();
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, data, 5, 1000);
	FM_DISABLE();
	HAL_Delay(100);
	/**********************************************************************************/

	R_STAT(wstatus);
	while (wstatus->RDY != 0)		 //To ensure data is written
	{
		R_STAT(wstatus);
	}
	if (wstatus->WEN == 0) {
		return 1; 					//if device is not write enabled
	} else if (wstatus->WEN == 1) {
		return 2;					//if WEN enabled
	}
	return 0;						//if none of above register conditions are met
}


/*
 * @brief		To write data of more than a byte
 * @brief		Each time data is written bytewise so it is inefficient
 *
 *@param		address		Starting address of the FM to write data
 *				*data		Data to be written into the FM
 *				size		size of the Data in bytes
 *
 *@retval		none
 * */
void Write_Data(uint32_t address, uint8_t *data, int size) {
	uint32_t start = address;
	uint32_t end = start + size - 1;
	while (end >= start) {
		Write_Byte(start, data);
		HAL_Delay(100);
		data++;
		start++;
	}
}

/*
 * @brief		function to Read one byte of data
 *
 * @param		address		address of the FM to be read
 *
 * @retval		data		data stored in the corresponding address of the FM
 * */
uint8_t Read_Byte(uint32_t address) {
	uint32_t addr = address;
	uint8_t data;
	uint8_t cmd = READ;
	uint8_t add[3];
	uint8_t buffer[50];
	UNUSED(buffer);
	add[0] = (uint8_t) (addr >> 16 & 0xFF);
	add[1] = (uint8_t) (addr >> 8 & 0xFF);
	add[2] = (uint8_t) (addr & 0xFF);
	FM_EN();
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
	HAL_SPI_Transmit(&hspi1, add, 2, 100);
	HAL_SPI_TransmitReceive(&hspi1, &add[2], &data, 1, 100);//because data is received just after address is sent
	FM_DISABLE();
	return data;
}

/*
 * @brief		to Read data of more than one bytes by receiving one byte at a time
 * @brief		inefficient method, BulkRead function can be used for efficiency
 *
 * @param		address		starting address of the FM
 *	 			*buffer		pointer to the stating address to be read
 *	 			size		size of the data to be read in bytes
 *
 *@retval		data		data of corresponding address of the FM
 * */
uint8_t Read_Data(uint32_t address, uint8_t *buffer, int size) {
	uint32_t start = address;
	uint32_t end = start + size - 1;
	while (end >= start) {
		*buffer = Read_Byte(start);
		HAL_Delay(70);
		start++;
		buffer++;
	}
	return 1;
}

/*
 * @brief		function to read data of FM in bulk
 * 				more efficient than Read_Data because data is read with just one command
 *		on simulation first byte is not received (slower HAL function), alternatives can be considered
 *
 * @param		address		starting address of the FM
 * 				*buffer		data read from FM
 * 				size		size of data to be read in bytes
 *
 * @retval		buffer		data stored in corresponding address of the FM
 * */
uint8_t BulkRead(uint32_t address, uint8_t *buffer, int size) {
	uint8_t command[4];
	uint32_t addr = address-1;
	command[0] = READ;
	command[1] = (uint8_t) (addr >> 16 & 0xff);
	command[2] = (uint8_t) (addr >> 8 & 0xff);
	command[3] = (uint8_t) (addr & 0xff);
	FM_EN();
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, command, 4, 20);
	HAL_SPI_Receive(&hspi1, buffer, size, size);
	FM_DISABLE();
	HAL_Delay(100);
	return 1;
}

/*
 * @brief		to Write bulk of data in the FM( much more efficient in writing data)
 *
 *
 * @param		address		starting address Of the FM
 * 				*buffer		data to be written into the FM
 * 				size		size of the data to be written
 *
 * @retval		1 if data is small enough and is stored in one page
 * 				2 if data doesn't fit in one page and is written in multiple pages
 *
 * */
uint8_t PageWrite(uint32_t address, uint8_t *buffer, int size) {
	uint8_t cmd = PROGRAM;
	uint8_t command[3];
//	uint8_t command1[3];
	uint16_t num;
	uint32_t PageStart;
	uint32_t PageEnd;
	uint16_t Rem;
	uint32_t NextPage;

	/*-------------------------------------------------------------------*/
	int extraa = 0;
	num = Pagenum(&address);
	PageStart = (PageSize) * num;
	PageEnd = PageStart + 255;
	NextPage = PageEnd + 1;
	Rem = ((PageEnd - address) + 1);
	extraa = size - Rem;
	int status;
	if (Rem >= size) {
		status = 1;
	} else if((PageSize+Rem)<=512){
		status = 2;
	}
	//if data is more than 2 pages
	else{
		return 99;
	}
	/*--------------------------------------------------------------*/

	command[0] = (uint8_t) (address >> 16 & 0xff);
	command[1] = (uint8_t) (address >> 8 & 0xff);
	command[2] = (uint8_t) (address & 0xff);
	Write_En();
	FM_EN();
	HAL_Delay(1);
	if (status == 1) {
		HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
		HAL_SPI_Transmit(&hspi1, command, 3, 100);
		HAL_SPI_Transmit(&hspi1, buffer, size, size);
		FM_DISABLE();
		HAL_Delay(100);
		return 1;
	} else if (status == 2) {
		HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
		HAL_SPI_Transmit(&hspi1, command, 3, 100);
		HAL_SPI_Transmit(&hspi1, buffer, Rem, Rem);
		FM_DISABLE();
		HAL_Delay(150);
		buffer = buffer+Rem;
		command[0] = (uint8_t) (NextPage >> 16 & 0xff);
		command[1] = (uint8_t) (NextPage >> 8 & 0xff);
		command[2] = (uint8_t) (NextPage & 0xff);
		Write_En();
		FM_EN();
		HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
		HAL_SPI_Transmit(&hspi1, command, 3, 100);
		HAL_SPI_Transmit(&hspi1, buffer, extraa, extraa);
		FM_DISABLE();
		HAL_Delay(100);
		return 2;
	}
	return 3;
}

/*
 * @brief		 To read status register
 *
 * @param		none
 *
 * retval		status		status of the regisetrs
 * */
void R_STAT(RegisterStatus *buffer) {
	RegisterStatus temp;
	uint8_t command = RDSR & 0xFF;
	uint8_t status = 0;
	FM_EN();
	HAL_SPI_Transmit(&hspi1, &command, 1, 50);
	HAL_SPI_Receive(&hspi1, &status, 1, 100);
	FM_DISABLE();
	temp.WPEN = status >> 7;
	temp.WEN = status >> 1 & 00000001;
	temp.RDY = status & 00000001;
	temp.BP0 = status >> 2 & 00000001;
	temp.BP1 = status >> 3 & 00000001;
	temp.BP2 = status >> 4 & 00000001;
	*buffer = temp;
	return;
}

/*
 * @brief		to write/modify the status register
 *
 * @param		instruction		command to be written to status register
 *
 *
 */
void WriteReg(uint8_t instruction) {
	uint8_t command = WRSR;
	Write_En();
	FM_EN();
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, &instruction, 1, 100);
	HAL_Delay(1);
	FM_DISABLE();
	HAL_Delay(100);
}

/*
 * @brief		to find the sector number of given address
 *
 * @param		address		address of which sector number is requrired
 *
 * @retval		corresponding sector number of the address
 * 				0 if the address is invalid
 */
uint8_t Sector_Number(uint32_t address) {
	uint32_t addr = address;
	int sector;
	if (addr >= SectorOne && addr < SectorTwo) {
		sector = 1;
		return sector;
	} else if (addr >= SectorTwo && addr < SectorThree) {
		sector = 2;
		return 2;
	} else if (addr >= SectorThree && addr < SectorFour) {
		sector = 3;
		return 3;
	} else if (addr >= SectorFour && addr < SectorFive) {
		sector = 4;
		return 4;
	} else if (addr >= SectorFive && addr < SectorSix) {
		sector = 5;
		return 5;
	} else if (addr >= SectorSix && addr < SectorSeven) {
		sector = 6;
		return 6;
	} else if (addr >= SectorSeven && addr < SectorEight) {
		sector = 7;
		return 7;
	} else if (addr >= SectorEight && addr <= SectorEnd) {
		sector = 8;
		return 8;
	}
	return 0;
}

/*
 * @brief		To Find the page of the address given
 *
 * */
uint8_t Pagenum(uint32_t *address) {
	uint32_t addr = *address;
	uint8_t Pagenum;
	Pagenum = (addr / 256);
	return Pagenum;
}


/*
 * @brief		to find the remaining bytes in the given page
 *
 * @param		address		address of which remaining bytes in page is to be found
 *
 * @retval		remaining bytes in the page
 */
uint32_t RemBytes(uint32_t *address) {
	uint32_t addr = *address;
	uint16_t num;
	uint32_t PageStart;
	uint32_t PageEnd;
	uint16_t Rem;
	num = Pagenum(&addr);
	PageStart = (PageSize) * num;		//starting address of corresponding page
	PageEnd = PageStart + 255;			//end address of corresponding page
	Rem = ((PageEnd - addr) + 1);		//remaining bytes in the page
	return Rem;
}

/*
 * @brief		to erase corresponding sector
 *
 */
void SectorErase(uint32_t *address) {
	uint32_t addr = *address;
	uint8_t add[4];
	add[0] = SECTOR_ERASE & 0xFF;
	add[1] = (uint8_t) (addr >> 16 & 0xff);
	add[2] = (uint8_t) (addr >> 8 & 0xff);
	add[3] = (uint8_t) (addr & 0xff);
	Write_En();
	FM_EN();
	HAL_SPI_Transmit(&hspi1, add, 4, 1000);
	FM_DISABLE();
	HAL_Delay(1000);						//1 second time delay to erase 1 sector
}

/*
 * @brief		to erase the whole chip
 * @brief		typically 8s required to erase the whole chip but it is internally controlled
 *
 * @param		none
 */
void ChipErase() {
	uint8_t command = CHIP_ERASE;
	RegisterStatus *status=0;
	Write_En();
	FM_EN();
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, &command, 1, 100);
	FM_DISABLE();
	R_STAT(status);
	while(status->WEN != 0 && status->RDY != 0)
	{
		R_STAT(status);
	}
}

 /* set CS pin low to enable the FM*/
void FM_EN() {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, RESET);
}

/* set CS pin high to disable the FM*/
void FM_DISABLE() {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, SET);
}

/*End Functions */



/*****************************GARBAGE**********************************/

/*-----------------bulkwrite--------------------*/
/*
 uint8_t BulkWrite(uint32_t address, uint8_t *data, int size){
	uint8_t command[size];
	int i;
	uint8_t daataa[10];
	uint8_t input = *data;
	command[0] = PROGRAM & 0xff;
	command[1] = (uint8_t)(address >> 16 & 0xff);
	command[2] = (uint8_t)(address >> 8 & 0xff);
	command[3] = (uint8_t)(address & 0xff);
	for(i=0;i<10;i++)
	{
		daataa[i] = input;
		input++;
	}
	FM_EN();
	HAL_Delay(1);
	HAL_SPI_Transmit(&hspi1, command, 4, 100);
	for(i=0;i<10;i++)
	{

		HAL_SPI_Transmit(&hspi1, daataa, 10, 1000);
		HAL_Delay(1);
	}
	FM_DISABLE();
	HAL_Delay(500);
	return 1;
}
*/


/*--------------------to find page of the sector-----------------*/
/*
	uint32_t addres = *address;
	uint8_t SectorPos;
	SectorPos = Sector_Number(addres);
	switch (SectorPos) {

	}
	case 1:
		Pagenum = ((addr - SectorOne)) / 256;
		return Pagenum;
		break;
	case 2:
		Pagenum = ((addr - SectorTwo)) / 256;
		return Pagenum;
		break;
	case 3:
		Pagenum = ((addr - SectorThree)) / 256;
		return Pagenum;
		break;
	case 4:
		Pagenum = ((addr - SectorFour)) / 256;
		return Pagenum;
		break;
	case 5:
		Pagenum = (addr - SectorFive) / 256;
		return Pagenum;
		break;
	case 6:
		Pagenum = (addr - SectorSix) / 256;
		return Pagenum;
		break;
	case 7:
		Pagenum = (addr - SectorSeven) / 256;
		return Pagenum;
		break;
	case 8:
		Pagenum = (addr - SectorEight) / 256;
		return Pagenum;
		break;
	}
*/

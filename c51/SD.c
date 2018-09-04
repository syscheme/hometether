#include "defines.h"
#include "SD.h"

//--------------------------------------------------------------
bit is_init;        // 1- if during the initialization, 0- if completed
bit sdcard_type;      // card type: 0- MMC, 1- SD
//---------------------------------------------------------------

// --------------------------------------------------
// delayNOP() delay for  (4.34us)
// --------------------------------------------------
void delayNOP()
{
	uchar i=3*12;
	while(i--);
}

// --------------------------------------------------
// SD_writeCmd() write a commad to the flash
// @param pcmd - the 6-byte SPI command
// @return the respond byte from the flash, 0xff if failed
// --------------------------------------------------
uchar SD_writeCmd(uchar* cmd)
{
	uchar temp, i=0;

	SD_CS=1;
	SPI_write(0xff); // to have better compatiblity for some SD cards
	SD_CS=0;

	//send 6-byte command to MMC/SD-Card
	for (i=0; i<6; i++) 
		SPI_write(cmd[i]);

	temp = SPI_read(); // ignore the first byte
	i =0;
	do {  
		temp = SPI_read(); // keep reading until non-0xff
		i++;
	} while( (temp==0xff) && (i<TRY_TIME) ); 

	return temp;
}

static void SD_close()
{
	SD_CS=1; // close the card
	SPI_write(0xff); // follow with 8 CLK
	//	temp = SPI_read();
	SD_SI =1;
	SD_SO =1;
}

static void SD_open()
{
	SD_CS=0;
}

// --------------------------------------------------
// SD_init()	initialize by taking CMD1
// @return 0 if succeeded, INIT_CMD1_ERROR if failed at CMD1
// INIT_CMD0_ERROR if failed at CMD0
// --------------------------------------------------
uchar SD_init()	// initialize by taking CMD1
{  
	uchar i, temp;
	uchar cmd[] = {0x40,0x00,0x00,0x00,0x00,0x95}; // the sequence of CMD0

#ifdef STC_SPI
	SPCTL = 0xdf;	// work as SPI host, ignore SS, start with low speed CPU_CLK/128
#endif

	SD_SCL=1;
	SD_SI =1;
	SD_SO =1;
	SD_CS =1;
    for (i=0; i< 3; i++) delayXms(250);         

   	SD_open();

	do {
		// send 74 clock at least!!!
		for (i=0; i<0x0f; i++) 
			SPI_write(0xff);

		// try up to 200 times to send CMD0 command to MMC/SD Card
		for (temp =0xff, i=0; i < TRY_TIME && temp !=1; i++)
			temp = SD_writeCmd(cmd);

		if (i >=TRY_TIME) //time out at CMD0 response
		{
			i = INIT_CMD0_ERROR;  //CMD0 Error!
			break;
		}

		// try the CMD1 query to determine card type
		cmd[0] = 0x41;  cmd[5] = 0xff;
		for (temp =0xff, i=0; i < TRY_TIME && temp !=0x00; i++)
		{
			// try CMD55
			cmd[0] = 0x77; temp = SD_writeCmd(cmd);

			if (1 == temp) // if CMD55 has response
			{
				// try to take CMD41 to activate SD card
				cmd[0] = 0x69; temp = SD_writeCmd(cmd);
				if(temp == 0x00)               // if CMD41 respond ok, this is a SD card
					sdcard_type =1;
			}
			else
			{
				// failed at CMD55, try CMD1 to activate the card instead
				cmd[0] = 0x41; cmd[5] = 0xff; temp = SD_writeCmd(cmd);
				if(temp == 0x00)             // if CMD1 respond ok, this is a MMC card
					sdcard_type =0;                 //MMC
			}
		}

		if (i >=TRY_TIME) //time out at CMD0 response
		{
			i = INIT_CMD1_ERROR;  //CMD0 Error!
			break;
		}

		is_init=0;
#ifdef STC_SPI
		SPCTL = 0xDC;	// work as SPI host, ignore SS, switch to higher speed =CPU_CLK/4 
#endif
		i =0; // success return code
	} while (0);

	SD_close();
	return i;
}
// --------------------------------------------------
// SD_writeSector()	write a 512Byte sector into the card via CMD24
// @return 0 if succeeded, WRITE_BLOCK_ERROR if failed
// --------------------------------------------------
uchar SD_writeSector(ulong addr, uchar* buffer)
{  
	uchar temp;
	uint i;
	//	char pcmd[] = {0x58,0x00,0x00,0x00,0x00,0xff}; // the sequence of CMD24
	char cmd[] = {0x58,0x00,0x00,0x00,0x00,0xff};  // the sequence of CMD24

	addr <<=9;   // sector = sector * 512, convert the block address into the byte-based address, up to 4GB

	cmd[1]=((addr & 0xff000000) >>24); // fill the address into the CMD
	cmd[2]=((addr & 0x00ff0000) >>16);
	cmd[3]=((addr & 0x0000ff00) >>8);

	SD_open();
	do {
		// try up to 200 times to send CMD24 command to MMC/SD Card
		for (temp =0xff, i=0; i < TRY_TIME && temp !=0; i++)
			temp = SD_writeCmd(cmd);

		if (i >=TRY_TIME) //time out at CMD24 response
		{
			i = WRITE_BLOCK_ERROR;  // CMD24 Error!
			break;
		}

		for(i=0; i<100; i++) // insert some CLK to yield for a while
			SPI_read();

		SPI_write(0xfe); // send the leading byte 0xfe, then follow with the 512B data

		for (i=0; i<512; i++) // write the 512B data from buffer into SD
			SPI_write(*buffer++);

		SPI_write(0xff); 
		SPI_write(0xff); // two-bytes of dummy CRC

		temp =SPI_read();   // read the returning code
		// the write has been accepted by the card if the code is XXX00DELAY_TIME1
		if ((temp & 0x1F) !=0x05) 
		{
			i = WRITE_BLOCK_ERROR;  // CMD24 Error!
			break;
		}

		while(SPI_read() !=0xff); // wait util the card becomes idle: 0-busy, 0xff-idle

		i =0; // success return code
	} while (0);

	SD_close();
	return i;		 // succeeded
} 


// --------------------------------------------------
// SD_readSector()	read a 512Byte sector from the card via CMD17
// @return 0 if succeeded, READ_BLOCK_ERROR if failed
// --------------------------------------------------
uchar SD_readSector(ulong addr, uchar *buffer)
{
	uint i;
	uchar temp;
	uchar cmd[]={0x51,0x00,0x00,0x00,0x00,0xFF}; // the sequence of CMD17

	addr <<= 9; // sector = sector * 512, convert the block address into the byte-based address

	cmd[1]=((addr&0xFF000000)>>24);
	cmd[2]=((addr&0x00FF0000)>>16);
	cmd[3]=((addr&0x0000FF00)>>8);

	SD_open();
	do {
		// try up to 200 times to send CMD17 command to MMC/SD Card
		for (temp =0xff, i=0; i < TRY_TIME && temp !=0; i++)
			temp = SD_writeCmd(cmd);

		if (i >=TRY_TIME) //time out at CMD17 response
		{
			i = READ_BLOCK_ERROR;  // CMD17 Error!
			break;
		}

		while (SPI_read() != 0xfe); // wait until there are data available, the data follows this

		for (i=0; i <512; i++)	 // read the data into the buffer
			buffer[i] = SPI_read();

		temp = SPI_read();
		temp = SPI_read(); // read two-byte CRC code

		i =0; // success return code
	} while (0);

	SD_close();
	return i;		 // succeeded
}


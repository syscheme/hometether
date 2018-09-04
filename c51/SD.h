#ifndef __SD_H__
#define __SD_H__

#include "LcdTouchPad.h"

sbit    SD_CS    =P3^4;
sbit    SD_CD    =P3^5;
#define SD_SCL   SPI_CLK
#define SD_SI    SPI_DI
#define SD_SO    SPI_DO

#define DELAY_TIME 2000 //SD卡的复位与初始化时SPI的延时参数，根据实际速率修改其值，否则会造成SD卡复位或初始化失败
#define TRY_TIME 200   //向SD卡写入命令之后，读取SD卡的回应次数，即读TRY_TIME次，如果在TRY_TIME次中读不到回应，产生超时错误，命令写入失败

extern bit sdcard_type;
extern bit is_init;

//错误码定义
//-------------------------------------------------------------
#define INIT_CMD0_ERROR     0x01 //CMD0错误
#define INIT_CMD1_ERROR     0x02 //CMD1错误
#define WRITE_BLOCK_ERROR   0x03 //写块错误
#define READ_BLOCK_ERROR    0x04 //读块错误
//-------------------------------------------------------------

unsigned char SD_init();
unsigned char SD_writeSector(unsigned long addr,unsigned char *buffer);
unsigned char SD_readSector(unsigned long addr,unsigned char *buffer);

#endif
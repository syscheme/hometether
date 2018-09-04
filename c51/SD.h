#ifndef __SD_H__
#define __SD_H__

#include "LcdTouchPad.h"

sbit    SD_CS    =P3^4;
sbit    SD_CD    =P3^5;
#define SD_SCL   SPI_CLK
#define SD_SI    SPI_DI
#define SD_SO    SPI_DO

#define DELAY_TIME 2000 //SD���ĸ�λ���ʼ��ʱSPI����ʱ����������ʵ�������޸���ֵ����������SD����λ���ʼ��ʧ��
#define TRY_TIME 200   //��SD��д������֮�󣬶�ȡSD���Ļ�Ӧ����������TRY_TIME�Σ������TRY_TIME���ж�������Ӧ��������ʱ��������д��ʧ��

extern bit sdcard_type;
extern bit is_init;

//�����붨��
//-------------------------------------------------------------
#define INIT_CMD0_ERROR     0x01 //CMD0����
#define INIT_CMD1_ERROR     0x02 //CMD1����
#define WRITE_BLOCK_ERROR   0x03 //д�����
#define READ_BLOCK_ERROR    0x04 //�������
//-------------------------------------------------------------

unsigned char SD_init();
unsigned char SD_writeSector(unsigned long addr,unsigned char *buffer);
unsigned char SD_readSector(unsigned long addr,unsigned char *buffer);

#endif
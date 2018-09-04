#ifndef __ICAN_master_H__
#define __ICAN_master_H__

#include "ican.h"

#define ICAN_SLAVE_NUM					4
#define ICAN_MASTER_CHANNEL				0

extern	iCanNode slaves[ICAN_SLAVE_NUM];

uint8_t ican_master_read(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t * buff);
uint8_t ican_master_write(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t * buff);
uint8_t ican_master_event_trager(uint8_t slave_id, uint8_t src_id, uint8_t offset, uint8_t len, uint8_t * buff);
uint8_t ican_master_establish_connect(uint8_t slave_id);
uint8_t ican_master_delete_connect(uint8_t slave_id);
uint8_t ican_master_device_reset(uint8_t slave_id);
void ican_master_init(uint8_t channel);
void ican_master_task(void * pdata);

#endif // __ICAN_master_H__

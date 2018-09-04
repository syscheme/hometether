#ifndef __ICAN_slave_H__
#define __ICAN_slave_H__

#include "ican.h"

#define ICAN_SLAVE_CHANNEL				1

uint8_t ican_slave_macid_check(uint8_t channel); 
uint8_t ican_slave_poll(uint8_t channel);

void ican_slave_init(uint8_t channel);
void ican_slave_task(void * pdata);

#endif // __ICAN_slave_H__

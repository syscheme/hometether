#include "../../common/common.h"
#include "ican.h"
#include "tmr.h"
#include "ican_master.h"

CAN_TypeDef* CAN_CHANNELS[] = {CAN1 }; // , CAN2};
uint8_t      CAN_FIFOS[] = {CAN_FIFO0, CAN_FIFO1};

int main(void)
{
/*
	rcc_init();
	nvic_init();
	gpio_init();
	adc1_init();
	tmr_init();
	tim2_init(TIM2_ARR_VALUE, TIM2_PSC_VALUE);
*/	
	ican_master_task((void *)0);
}

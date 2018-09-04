#include "includes.h"
#include "nrf24l01.h"

// 定义变量
unsigned char Clock1s=0;
uint16_t ADC_DMAbuf[CHANNEL_SZ_ADC];

// Private macro
// Private variables
ADC_InitTypeDef ADC_InitStructure;
vu16 ADC_ConvertedValue;
ErrorStatus HSEStartUpStatus;

// =========================================================================
// tests
// =========================================================================
int itest, jtest;
void test_blink(int stepNo)
{
	printf("step %d. Onboard led blink test\r\n", stepNo);
	for (itest =0; itest<8; itest++)
	{
		DQBoard_setLed(0, 1); delayXusec(100000); DQBoard_setLed(0, 0); 
		DQBoard_setLed(1, 1); delayXusec(100000); DQBoard_setLed(1, 0); 
		DQBoard_setLed(2, 1); delayXusec(100000); DQBoard_setLed(2, 0); 
		DQBoard_setLed(3, 1); delayXusec(100000); DQBoard_setLed(3, 0);
	} 
}

const static Syllable song[] = {
	 {low_5, 4},
	 {mid_1, 6}, {low_7, 2}, {low_6, 4}, {low_5, 4},
	 {low_5, 8}, {low_3, 4}, {low_5, 4},
	 {low_4, 6}, {low_3, 2}, {low_4, 4}, {low_2, 4}, 
	 {low_1, 12}
};


void test_startSong(int stepNo)
{
	printf("step %d. Onboard beep/pwm test\r\n", stepNo);
	for (itest =0; itest < sizeof(song) / sizeof(song[0]); itest++)
		DQBoard_beep(song[itest].pitchReload, song[itest].lenByQuarter *100);
}

// http://www.ourdev.cn/thread-3639584-1-1.html
void test_fireAlarm(int stepNo)
{
	// 600 ~1500hz continously increase in 3~5sec
	printf("step %d. Onboard fire alarm test\r\n", stepNo);
	for (itest =144000/600; itest >144000/1500; itest--)
		DQBoard_beep(itest*100, 5000 /(144000/600 - 144000/1500));
}

void test_policeAlarm(int stepNo)
{
	// 100, 1000hz switched very 0.5sec
	printf("step %d. Onboard police alarm test\r\n", stepNo);
	for (itest =0; itest <10; itest++)
	{
		DQBoard_beep(14400000 /100, 100);
		DQBoard_beep(14400000 /1000,  100);
	}
}

void test_IrSend(int stepNo)
{
	printf("step %d. Ir send test\r\n", stepNo);
	for (itest =0; itest < CHANNEL_SZ_IrLED*4; itest++)
	{
		for (jtest =0; jtest <8; jtest++)
			IrSend_PT2262(itest, 0xc7c7c7);

		delayXusec(20000);
	}
}

void test_relay(int stepNo)
{
	printf("step %d. relay test\r\n", stepNo);
	for (itest =0; itest < CHANNEL_SZ_Relay*2; itest++)
	{
		ECU_setRelay(itest, 1);
		delayXusec(500000);
		ECU_setRelay(itest, 0);
		delayXusec(500000);
	}
}


void read_motion(int stepNo)
{
	printf("step %d. motion read: state[%03x]\r\n", stepNo, MotionState());
}

void read_ADC(int stepNo)
{
	int temp =ADC_DMAbuf[CHANNEL_SZ_ADC-1];
	printf("step %d. ADC read\r\n", stepNo);
	for(itest=0; itest<CHANNEL_SZ_ADC; itest++)
		printf("ADC%d[%03x] ", itest, ADC_DMAbuf[itest]);	//从串口输出采集到的AD值
	printf("\r\n");

	temp = (5857 - temp *33/10) /17612 +25000; // C= (1.43- temp*3.3/4096) / (4.3*1000) +25
	temp /=100;
	printf("temp[%.1fC]\r\n", temp/10.0);
}

void read_ADC2(int stepNo)
{
	printf("step %d. ADC read2, ADC0[%03x]\r\n", stepNo, ADC_GetConversionValue(ADC1));
}

void read_ADC3(int stepNo)
{
	printf("step %d. ADC read via DMA, ADC0[%03x]\r\n", stepNo, ADC_DMAbuf[itest]);
}

// =========================================================================
// main program
// =========================================================================
#define READ_INTERVAL (1000000)
int main(void)
{
	int nstep =0, nround=0, i;

	// initialize the board
	BSP_Init();

	// initialize the onboard RS232 port as the debug output
	// USART_open(USART1, 1152);
	printf("test program starts\r\n");

	test_startSong(++nstep);

	for (i =1; i; i--)
		test_relay(++nstep);

	while (1) {
	for (i =1; i; i--)
		test_blink(++nstep);

//	for (i =3; i; i--)
//		test_fireAlarm(++nstep);

//	for (i =10; i; i--)
//		test_policeAlarm(++nstep);

	for (i =1; i; i--)
		test_IrSend(++nstep);

	for (i =20; i; i--)
	{
		read_motion(++nstep);
		delayXusec(READ_INTERVAL);
	}

	for (i =20; i; i--)
	{
 		read_ADC(++nstep);
		delayXusec(READ_INTERVAL);
	}

	}
}

// dummy receive loop
void canReceiveLoop(void)
{
	
	CanRxMsg RxMessage;
	while(CAN_MessagePending(CAN1, CAN_FIFO0) >0)
	{
		CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);	//从CAN1口缓冲器FIFO 0接收报文到 RxMessage
		// do nothing here

#ifdef CAN_GATEWAY
		canForwardToExt(m);
#endif // CAN_GATEWAY
	}
}

void OnRS232Message(char* msg)
{
}

void OnRS485Message(char* msg)
{
}

void nRF24L01_OnPacket(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen)
{
	printf("received from pipe[%d] %s\r\n", pipeId, (char*) bufPlayLoad);
}

/*********************************************
* vim:sw=8:ts=8:si:et
* To use the above modeline in vim you must have "set modeline" in your .vimrc
* Author: Guido Socher
* Copyright: GPL V2
* See http://www.gnu.org/licenses/gpl.html
*
* Ethernet remote device and sensor
* UDP and HTTP interface 
url looks like this http://baseurl/password/command
or http://baseurl/password/
*
* Chip type           : Atmega88 or Atmega168 with ENC28J60
* Note: there is a version number in the text. Search for tuxgraphics
*********************************************/

/*

\\\|///
\\  - -  //
(  @ @  )
+---------------------oOOo-(_)-oOOo-------------------------+
|                       WEB SERVER                          |
|            ported to LPC2103 ARM7TDMI-S CPU               |
|                     by Xiaoran Liu                        |
|                       2007.12.16                          |
|                 ZERO research Instutute                   |
|                      www.the0.net                         |
|                            Oooo                           |
+----------------------oooO--(   )--------------------------+
(   )   ) /
\ (   (_/
\_)     

*/

#include "common.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_spi.h"
#include "misc.h"

//#include <stdlib.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "net.h"

// #include <LPC2103.H>
// #include <stdio.h>
// #include <string.h>

#define LED  ( 1 << 17 )        //P0.17控制LED

#define PSTR(s) s

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t myip[4] = {192,168,0,55};

// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
//
// listen port for udp
#define MYUDPPORT 1200

static uint8_t packet[MTU_SIZE+1];

// the password string (only the first 5 char checked), (only a-z,0-9,_ characters):
static char password[] ="the0.net"; // must not be longer than 9 char

void ENC28J60_ISR(ENC28J60* nic);

// 
uint8_t verify_password(char *str)
{
	// the first characters of the received string are
	// a simple password/cookie:
	if (strncmp(password, str, 5)==0)
		return(1);

	return(0);
}

// takes a string of the form password/commandNumber and analyse it
// return values: -1 invalid password, otherwise command number
//                -2 no command given but password valid
//                -3 valid password, no command and no trailing "/"
int8_t analyse_get_url(char *str)
{
	uint8_t loop=1;
	uint8_t i=0;
	while(loop)
	{
		if(0 == password[i])
			loop=0; // end of password
		else
		{
			if(*str!=password[i])
				return(-1);
			str++;
			i++;
		}
	}

	// is is now one char after the password
	if (*str != '/')
		return(-3);
	str++;

	// check the first char, garbage after this is ignored (including a slash)
	if (*str < 0x3a && *str > 0x2f)
		return(*str-0x30); // is a ASCII number, return it

	return(-2);
}

// answer HTTP/1.0 301 Moved Permanently\r\nLocation: password/\r\n\r\n
// to redirect to the url ending in a slash
uint16_t moved_perm(uint8_t *packet)
{
	uint16_t plen;
	plen=fill_tcp_data_str(packet,0,PSTR("HTTP/1.0 301 Moved Permanently\r\nLocation: "));
	plen=fill_tcp_data_str(packet,plen,password);
	plen=fill_tcp_data_str(packet,plen,PSTR("/\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
	plen=fill_tcp_data_str(packet,plen,PSTR("<h1>301 Moved Permanently</h1>\n"));
	plen=fill_tcp_data_str(packet,plen,PSTR("add a trailing slash to the url\n"));

	return(plen);
}


// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *packet,uint8_t on_off)
{
	uint16_t plen;
	plen=fill_tcp_data_str(packet,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
	plen=fill_tcp_data_str(packet,plen,PSTR("<center><p>Output is: "));
	if (on_off)
		plen=fill_tcp_data_str(packet,plen,PSTR("<font color=\"#00FF00\"> ON</font>"));
	else
		plen=fill_tcp_data_str(packet,plen,PSTR("OFF"));

	plen=fill_tcp_data_str(packet,plen,PSTR(" <small><a href=\".\">[refresh status]</a></small></p>\n<p><a href=\"."));
	if (on_off)
		plen=fill_tcp_data_str(packet,plen,PSTR("/0\">Switch off</a><p>"));
	else
		plen=fill_tcp_data_str(packet,plen,PSTR("/1\">Switch on</a><p>"));

	plen=fill_tcp_data_str(packet,plen,PSTR("</center><hr><br>version 2.10, tuxgraphics.org www.the0.net\n"));

	return(plen);
}


ENC28J60 nic= {
			{0x00,0x1A,0x6B,0xCE,0x92,0x66},
			NULL,
			SPI2,
			{GPIOD, GPIO_Pin_8},
			{GPIOD, GPIO_Pin_9},
			EXTI_Line7
			};

void RCC_Config(void)
{
	ErrorStatus HSEStartUpStatus;   

	RCC_DeInit(); // RCC system reset(for debug purpose)
	RCC_HSEConfig(RCC_HSE_ON); // Enable HSE
	HSEStartUpStatus = RCC_WaitForHSEStartUp(); // Wait till HSE is ready

	if (HSEStartUpStatus == SUCCESS)
	{
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable); // Enable Prefetch Buffer
		FLASH_SetLatency(FLASH_Latency_2); // Flash 2 wait state
		RCC_HCLKConfig(RCC_SYSCLK_Div1);  // HCLK = SYSCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);  // PCLK2 = HCLK
		RCC_PCLK1Config(RCC_HCLK_Div2); // PCLK1 = HCLK/2
		RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_16); // PLLCLK = 8MHz * 9 = 72 MHz
		RCC_PLLCmd(ENABLE); // Enable PLL 

		while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET); // Wait till PLL is ready

		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK); // Select PLL as system clock source
		while(RCC_GetSYSCLKSource() != 0x08); // Wait till PLL is used as system clock source
	}

	// Enable GPIO clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO, ENABLE);

	// Enable ADC1 and USART1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1 | RCC_APB1Periph_SPI2, ENABLE);

	// enable SPI2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	// enable DMA1
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

}

void GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;

	// section 1. about the onboard LEDs: PA1, PA4, PB0, PB1
	// ------------------------------------------------------
	//PA
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_1 | GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;	// because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	//PB
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;  // because the onboard leds are enabled by vH, so take Push-Pull mode
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// section 2. about the adapted enc28j60
	// ------------------------------------------------------
	// step 1. about D8-CS D9-RESET
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStruct);
	GPIO_SetBits(GPIOD, GPIO_Pin_8 | GPIO_Pin_9);

	// step 2. the SPI2 (B13-15)
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// SPI2 configuration
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_Mode      = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize  = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL      = SPI_CPOL_Low; //?? SPI_CPOL_High=模式3，时钟空闲为高 //SPI_CPOL_Low=模式0，时钟空闲为低
	SPI_InitStruct.SPI_CPHA      = SPI_CPHA_1Edge; //SPI_CPHA_2Edge;//SPI_CPHA_1Edge, SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_NSS       = SPI_NSS_Soft;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // SPI_BaudRatePrescaler_8=9MHz <=24L01的最大SPI时钟为10Mhz
	SPI_InitStruct.SPI_FirstBit  = SPI_FirstBit_MSB;//数据从高位开始发送
	SPI_InitStruct.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStruct);
	SPI_Cmd(SPI2, ENABLE);   // enable SPI2

	// step 7.3. about A7-IRQ
	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// about the exti7 for A7 that connects to IRQ of nRF24l01
	EXTI_DeInit();

	EXTI_StructInit(&EXTI_InitStruct);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7);
	EXTI_InitStruct.EXTI_Line = EXTI_Line7;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	EXTI_ClearITPendingBit(EXTI_Line7);
	EXTI_GenerateSWInterrupt(EXTI_Line7); //中断允许
}

#define VECT_TAB_RAM

void NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStruct;

	//#ifdef  VECT_TAB_RAM
#if defined (VECT_TAB_RAM)
	// Set the Vector Table base location at 0x20000000
	NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#elif defined(VECT_TAB_FLASH_IAP)
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x2000);
#else  // VECT_TAB_FLASH
	// Set the Vector Table base location at 0x08000000
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif 

	// Configure the NVIC Preemption Priority Bits
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

	// enable the EXTI9_5 for A8-IRQ of nRF2401 and A6 of IR recv
	NVIC_InitStruct.NVIC_IRQChannel                   = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

uint8_t groupIp[4] = {239,1,1,2};

int main0(void)
{
	uint16_t plen;
	uint16_t dat_p;
	uint8_t i=0;
	uint8_t cmd_pos=0;
	int8_t cmd;
	uint8_t payloadlen=0;
	char str[30];
	char cmdval;

	// set the clock speed to "no pre-scaler" (8MHz with internal osc or 
	// full external speed)
	// set the clock prescaler. First write CLKPCE to enable setting of clock the
	// next four instructions.
	//CLKPR=(1<<CLKPCE); // change enable
	//CLKPR=0; // "no pre-scaler"
	//delayXmsec(200);
	//printStr("A simple Web Server\n");

	/* enable PD2/INT0, as input */
	//DDRD&= ~(1<<DDD2);

	RCC_Config();   
	GPIO_Config();
	NVIC_Config();


	/*initialize enc28j60*/
	ENC28J60_init(&nic);

#if 0
	// LED
	// enable PB1, LED as output 
	//DDRB|= (1<<DDB1);
	IODIR |= LED;		   //设置IO口为输出口 

	// set output to Vcc, LED off
	//PORTB|= (1<<PB1);
	IOSET = LED;
	/*
	// the transistor on PD7
	DDRD|= (1<<DDD7);
	PORTD &= ~(1<<PD7);// transistor off
	*/
	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDB=yellow LEDA=green
	//
#endif
	
#if 0
	/* set output to GND, red LED on */
	//PORTB &= ~(1<<PB1);
	IOCLR = LED;
	i=1;
#endif

	//init the ethernet/ip layer:
	init_ip_arp_udp_tcp(nic.macaddr, myip, MYWWWPORT);
	//sprintf( str, "Chip rev: 0x%x \n",ENC28J60_getRev());
	//printStr( str );

//	while(1)
//		tcp_ack_from_any(&nic, packet);

	while(1)
	{
		//
		static uint16_t mi=0;
		if (0 == mi--)
		{
			fill_udp_data_p(packet, 0, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 30);
			fill_udp_data_p(packet, 1000, "hello again", 20);
			mcast(&nic, packet, groupIp, 1000, 2013, 1200);
		}

		// get the next new packet:
		plen = ENC28J60_recvPacket(&nic, MTU_SIZE, packet);

		/*plen will ne unequal to zero if there is a valid 
		* packet (without crc error) */
		if (plen==0)
		{
			//printStr( "no data\n" );
			continue;
		}

		//printStr( "Get it\n" );

		// arp is broadcast if unknown but a host may also
		// verify the mac address by sending it to 
		// a unicast address.
		if (eth_type_is_arp_and_my_ip(packet, plen))
		{
			arp_answer_from_request(&nic, packet);
			//printStr( "make_arp_answer_from_request\n" );
			continue;
		}

		// check if ip packets are for us:
		if (!eth_type_is_ip(packet,plen))
			continue;

		if (!is_to_myip(packet,plen))
			continue;

#if 0
		// led----------
		if (i)
		{
			/* set output to Vcc, LED off */
			IOSET = LED;
			//PORTB|= (1<<PB1);
			i=0;
		}
		else
		{
			/* set output to GND, LED on */
			//PORTB &= ~(1<<PB1);
			IOCLR = LED;
			i=1;
		}
#endif 

		if (packet[IP_PROTO_P]==IP_PROTO_ICMP_V && packet[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V)
		{
			// a ping packet, let's send pong
			echo_reply_from_request(&nic, packet,plen);
			//printStr( "make_echo_reply_from_request\n" );
			continue;
		}

		// tcp port www start, compare only the lower byte
		if (packet[IP_PROTO_P]==IP_PROTO_TCP_V&&packet[TCP_DST_PORT_H_P]==0&&packet[TCP_DST_PORT_L_P]==MYWWWPORT)
		{
			if (packet[TCP_FLAGS_P] & TCP_FLAGS_SYN_V)
			{
				tcp_synack_from_syn(&nic, packet);
				// make_tcp_synack_from_syn does already send the syn,ack
				continue;
			}

			if (packet[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
			{
				init_len_info(packet); // init some data structures
				// we can possibly have no data, just ack:
				dat_p=get_tcp_data_pointer();
				if (dat_p==0)
				{
					if (packet[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
					{
						// finack, answer with ack
						tcp_ack_from_any(&nic, packet);
					}
					// just an ack with no data, wait for next packet
					continue;
				}

				if (strncmp("GET ",(char *)&(packet[dat_p]),4)!=0)
				{
					// head, post and other methods:
					//
					// for possible status codes see:
					// http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
					plen=fill_tcp_data_str(packet,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
					goto SENDTCP;
				}

				if (strncmp("/ ",(char *)&(packet[dat_p+4]),2)==0)
				{
					plen=fill_tcp_data_str(packet,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
					plen=fill_tcp_data_str(packet,plen,PSTR("<p>Usage: http://host_or_ip/password</p>\n"));
					goto SENDTCP;
				}

				cmd=analyse_get_url((char *)&(packet[dat_p+5]));
				// for possible status codes see:
				// http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
				if (cmd==-1)
				{
					plen=fill_tcp_data_str(packet,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
					goto SENDTCP;
				}

#if 0
				if (cmd==1)
				{
					//PORTD|= (1<<PD7);// transistor on
					IOCLR = LED;
					i = 1;
				}

				if (cmd==0)
				{
					//PORTD &= ~(1<<PD7);// transistor off
					IOSET = LED;
					i = 0;
				}
#endif

				if (cmd==-3)
				{
					// redirect to add a trailing slash
					plen=moved_perm(packet);
					goto SENDTCP;
				}

				// if (cmd==-2) or any other value
				// just display the status:
				//plen=print_webpage(packet,(PORTD & (1<<PD7)));
				plen=print_webpage(packet,(i));
				//
SENDTCP:
				tcp_ack_from_any(&nic, packet); // send ack for http get
				tcp_ack_with_data(&nic, packet, plen); // send data
				continue;
			}
		} // tcp port www end

		// udp start, we listen on udp port 1200=0x4B0
		if (packet[IP_PROTO_P]==IP_PROTO_UDP_V 
		    && packet[UDP_DST_PORT_H_P]==(MYUDPPORT>>8) && packet[UDP_DST_PORT_L_P]==(MYUDPPORT&0xff))
		{
			payloadlen=packet[UDP_LEN_L_P]-UDP_HEADER_LEN;
			// you must sent a string starting with v
			// e.g udpcom version 10.0.0.24
			if (verify_password((char *)&(packet[UDP_DATA_P])))
			{
				// find the first comma which indicates 
				// the start of a command:
				cmd_pos=0;
				while(cmd_pos<payloadlen)
				{
					cmd_pos++;
					if (packet[UDP_DATA_P+cmd_pos]==',')
					{
						cmd_pos++; // put on start of cmd
						break;
					}
				}

				// a command is one char and a value. At
				// least 3 characters long. It has an '=' on
				// position 2:
				if (cmd_pos<2 || cmd_pos>payloadlen-3 || packet[UDP_DATA_P+cmd_pos+1]!='=')
				{
					strcpy(str,"e=no_cmd");
					goto ANSWER;
				}

				// supported commands are
				// t=1 t=0 t=?
				if (packet[UDP_DATA_P+cmd_pos]=='t')
				{
					cmdval=packet[UDP_DATA_P+cmd_pos+2];
					if(cmdval=='1')
					{
						//PORTD|= (1<<PD7);// transistor on
//						IOCLR = LED;
						strcpy(str,"t=1");
						goto ANSWER;
					}
					else if(cmdval=='0')
					{
						//PORTD &= ~(1<<PD7);// transistor off
//						IOSET = LED;
						strcpy(str,"t=0");
						goto ANSWER;
					}
					else if(cmdval=='?')
					{
						//if (PORTD & (1<<PD7)){
//						if (IOPIN & LED)
						{
							strcpy(str,"t=1");
							goto ANSWER;
						}

						strcpy(str, "t=0");
						goto ANSWER;
					}
				}

				strcpy(str,"e=no_such_cmd");
				goto ANSWER;
			}

			strcpy(str,"e=invalid_pw");
ANSWER:
			udp_reply_from_request(&nic, packet, str, strlen(str), MYUDPPORT);
		}
	}
	//return (0);
}

#define MAX_SEND_FAIL (6)

void NIC_sendPacket(void* nic, uint8_t* packet, uint16_t packetLen)
{
	static uint8_t cFail =MAX_SEND_FAIL;
	if (NULL == nic || NULL ==packet || packetLen<=0)
		return;

	if (ENC28J60_sendPacket((ENC28J60*) nic, packetLen, packet))
	{
		cFail = MAX_SEND_FAIL;
		return;
	}

	if (--cFail)
		return;

	// continuous send fail, reset the NIC
	ENC28J60_init(nic);
}

// =========================================================================
// About NIC interface
// =========================================================================
// ---------------------------------------------------------------------------
//  sample RX handlers
// ---------------------------------------------------------------------------
// ignore PKTIF because is unreliable! (look at the errata datasheet)
// check EPKTCNT is the suggested workaround.
// We don't need to clear interrupt flag, automatically done when
// enc28j60_hw_rx() decrements the packet counter.
uint8_t packetEth[MTU_SIZE+1];
char  wkurl[100];

void GW_nicIsr(ENC28J60* nic)
{
	uint16_t plen=0;
	uint16_t tmp;
	char* p=NULL, *q;
	uint32_t nodeId=0;

	volatile uint32_t eir;

//	ENC28J60_setBank(nic, EIE);
//	ENC28J60_writeOp(nic, EIE, 0);
//	eir = ENC28J60_readOp(nic, EIR);

	do {
		// step 1, get the next new packet:
		tmp  = ENC28J60_readOp(nic, EPKTCNT);
		plen = ENC28J60_recvPacket(nic, sizeof(packetEth)-1, packetEth);

		// plen will ne unequal to zero if there is a valid packet (without crc error)
		if (plen <=0)
			break;

		// step 2. process the arp packet directly
		//   arp is broadcast if unknown but a host may also verify the mac address by sending it to 
		//   a unicast address.
		if (eth_type_is_arp_and_my_ip(packetEth, plen))
		{
			arp_answer_from_request(nic, packetEth);
			continue;
		}

		if (!eth_type_is_ip(packetEth, plen))
			continue;

		// start processing IP packets here
 		if (packet[IP_PROTO_P]==IP_PROTO_ICMP_V && packet[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V)
		{
			// a ping packet, let's send pong
			echo_reply_from_request(nic, packet,plen);
			//printStr( "make_echo_reply_from_request\n" );
			continue;
		}



	} while (1);

//	ENC28J60_writeOp(nic, EIR, 0);
//	ENC28J60_writeOp(nic, EIE, EIE_INTIE | EIE_PKTIE | EIE_TXIE | EIE_TXERIE |EIE_RXERIE);

}

int main(void)
{
	uint16_t plen;
	uint16_t dat_p;
	uint8_t i=0;
	uint8_t cmd_pos=0;
	int8_t cmd;
	uint8_t payloadlen=0;
	char str[30];
	char cmdval;

	RCC_Config();   
	GPIO_Config();

	// initialize enc28j60
	ENC28J60_init(&nic);

	//init the ethernet/ip layer:
	init_ip_arp_udp_tcp(nic.macaddr, myip, MYWWWPORT);
	//sprintf( str, "Chip rev: 0x%x \n",ENC28J60_getRev());
	//printStr( str );
	NVIC_Config();

	while(1);
}

void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line7) != RESET)	// handling of IR receiver as EXTi
	{
		ENC28J60_ISR(&nic);
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
}

void HardFault_Handler(void)
{
	int i=0;
	i++;
}

void ENC28J60_ISR(ENC28J60* nic)
{
	// Variable definitions can be made now
	volatile uint32_t eir, pk_counter;
	volatile bool rx_activiated = FALSE;
	uint16_t plen=0;

	ENC28J60_clrBits(nic, EIE, EIE_INTIE);
	
	// get EIR
	eir = ENC28J60_read(nic, EIR);
	// rt_kprintf("eir: 0x%08x\n", eir);

	do
	{
		// errata #4, PKTIF does not reliable
	    pk_counter = ENC28J60_read(nic, EPKTCNT);
	    if (pk_counter)
	    {
	        // a frame has been received
			GW_nicIsr(nic);
			// plen = ENC28J60_recvPacket(nic, sizeof(packetEth)-1, packetEth);
	        // eth_device_ready((struct eth_device*)&(enc28j60_dev->parent));

			// disable rx interrutps
			ENC28J60_clrBits(nic, EIE, EIE_PKTIE);
	    }

		// clear PKTIF
		if (eir & EIR_PKTIF)
		{
			ENC28J60_clrBits(nic, EIR, EIR_PKTIF);

			rx_activiated = TRUE;
		}

		// clear DMAIF
	    if (eir & EIR_DMAIF)
		{
			ENC28J60_clrBits(nic, EIR, EIR_DMAIF);
		}

	    // LINK changed handler
	    if ( eir & EIR_LINKIF)
	    {
/*
	        ENC28J60_linkStatus(nic);

	        // read PHIR to clear the flag
	        ENC28J60_PhyRead(nic, PHIR);

*/
			ENC28J60_clrBits(nic, EIR, EIR_LINKIF);
	    }

		if (eir & EIR_TXIF)
		{
			// A frame has been transmitted.
			ENC28J60_clrBits(nic, EIR, EIR_TXIF);
		}

		// TX Error handler/
		if ((eir & EIR_TXERIF) != 0)
		{
			ENC28J60_clrBits(nic, EIR, EIR_TXERIF);
		}

		eir = ENC28J60_read(nic, EIR);
		// rt_kprintf("inner eir: 0x%08x\n", eir);
	} while ((rx_activiated != TRUE && eir != 0));

	ENC28J60_clrBits(nic, EIR, 0xff);
	ENC28J60_setBits(nic, EIE, EIE_INTIE | EIE_PKTIE | EIE_TXIE | EIE_TXERIE |EIE_RXERIE);

}

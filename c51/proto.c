#include "BaseBoard.h"
#include "proto.h"
#ifdef NODE_TYPE_LCD
#  include "LcdTouchPad.h"
#  include "FAT.h"
#endif //  NODE_TYPE_LCD

#ifdef ACT_AS_GATEWAY
#   define MSG_BUF_SIZE 1024
#else
#   define MSG_BUF_SIZE 200
#endif //  ACT_AS_GATEWAY

uchar xdata msgData[MSG_BUF_SIZE]; // the common buffer
uchar xdata msgDataLen =0;

#ifdef NODE_TYPE_BASE
uint chFlags_DS1820 =0x00, chFlags_Motion =0x00;

void doSetDS1820Chs(uint chFlags, bit freshOnly)
{
	uchar i =0;
	uint f =1;
	chFlags_DS1820 = chFlags;

	if (!freshOnly)
	{
		UART_writebyte(sizeof(uint)); // length of the message
		UART_writeword(chFlags_DS1820); // the flags of chFlags_DS1820 to echo the setting
	}

	// force to refresh the values of DS1820 channels
	for (; i<sizeof(uint); i++, f<<=1)
	{
		if (chFlags & f)
			sigChData[i] = readDS18B20(i);
	}
}

void doGetDS1820Chs()
{
	uchar i =0, j=0;
	uint f =1;
	for (; i<sizeof(uint); i++) // count the number of DS1820 channels
	{
		if (chFlags_DS1820 & f)
			j++;
	}

	UART_writeword(sizeof(uint) + 2*j); // length of the message
	UART_writeword(chFlags_DS1820);  // the flags of chFlags_DS1820, uint

	for (; i<sizeof(uint); i++, f<<=1)
	{
		if (! (chFlags_DS1820 & f))
			continue;

		UART_writeword(sigChData[i]);
	}

	doSetDS1820Chs(chFlags_DS1820, 1);
}

void doGetAll()
{
	uchar i =0, j=0, idata *p = NULL;
	uint f =1;

	UART_writeword(1	//latch_byte 
				+ ADC_CH_COUNT //ADC
			 	+ SIG_CH_COUNT*2 + SIG_CH_TYPES_COUNT *2 //sigCHs
				); // length of the message

	UART_writebyte(latch_byte); // the latch byte

	for (i=0, p=ADC_channels; i < ADC_CH_COUNT; i++, p++) // the bytes of the ADC channels
		UART_writebyte(*p);

	UART_writeword(chFlags_DS1820); // the flags of chFlags_DS1820 to echo the setting
	for (i=0; i < SIG_CH_COUNT; i++) // the word of the signal channels
		UART_writeword(sigChData[i]);

	UART_writeword(chFlags_Motion); // the flags of chFlags_Motion

	doSetDS1820Chs(chFlags_DS1820, 1);
}

void doGetLatchStates()
{
	UART_writeword(sizeof(uchar));
	UART_writebyte(latch_byte); // the latch byte

	setLatch(latch_byte); // ensure the latch is as what returned
}

void doSetLatchStates()
{
    if (msgDataLen >0)
		latch_byte = msgData[0];

	UART_writeword(sizeof(uchar));
	UART_writebyte(latch_byte); // the latch byte

	setLatch(latch_byte); // execute the set
}

void doSetLatchCh(bit enable)
{
	uchar bit2set=0x01, chId;
    if (msgDataLen >0)
	{
		chId = msgData[0];
		for (; chId>0; chId--)
			bit2set <<= 1;

		if (!enable)
			latch_byte |= bit2set;
		else
			latch_byte &= ~bit2set;
	}

	UART_writeword(sizeof(uchar));
	UART_writebyte(latch_byte); // the latch byte

	setLatch(latch_byte); // execute the set
}
#endif // NODE_TYPE_BASE

void OnComMsg() interrupt 4 // impl of COM INT
{
	uchar src, dest, cmd;
	uint datalen, i, j;
	bit bGateway = 0;

	if (0x87 != UART_readbyte())
	{
		ES =1;
		return;
	}

	dest     = UART_readbyte();
	src      = UART_readbyte();
	cmd      = UART_readbyte();
	datalen  = UART_readword();

#ifdef ACT_AS_GATEWAY
	bGateway = ((BOARD_SEQNO ^ dest) & 0xf0)?0:1;
#endif //  ACT_AS_GATEWAY

	if (!bGateway && BOARD_SEQNO !=dest && 0xff !=dest && BOARD_SEQNO == src)
	{
		// it is not the msg to this node, ignore the full msg
		for (; datalen >0; datalen--)
			UART_readbyte();

		// TODO: yield other node to respond
		// delay100ms();
		ES =1;
		return;
	}

#ifdef NODE_TYPE_LCD
	// the file transfer commands CMD_AppendFile utilizes jumbo in coming message, process seperately
	if (CMD_AppendFile == cmd && datalen >=16)
	{
		// 16-byte filename
		msgData[0] = '\\';
		for (i = 1; i < 17; i++)
			msgData[i] = UART_readbyte();
		msgData[i++] =0x00;
		datalen -= 16;
		j = datalen;

		do {
			if (bFileInUse || !FAT32_Create_File(&FileInfo, msgData, NULL))
			{   
				// access file failed
				datalen = 0xffff;
				break;
			}
			bFileInUse = 1;

			while (j >0)
			{
				tmp = min(datalen, sizeof(file_buf));
				for (i=0; i < tmp; i++)
					file_buf[i] = UART_readbyte();
				j -= tmp;

				if (!FAT32_Add_Dat(&FileInfo, tmp, file_buf))
				{
					// write file failed
					datalen = 0xfffe;
					break;
				}
			}

			FAT32_File_Close(&FileInfo);
			bFileInUse = 0;
		} while (0);

		for (; j >0; j--)
			UART_readbyte();

		//		set485Dir(0); // change the 485 direction to send data
		UART_writebyte(0x87);
		UART_writebyte(srcdest);
		UART_writebyte(RESP(cmd));
		UART_writeword(2);
		UART_writeword(datalen);
		//		set485Dir(1);
		ES =1;
		return;
	} 

#endif // NODE_TYPE_LCD

	// here is a msg needs attention in this node, read the message into the buffer
	msgDataLen = min(sizeof(msgData), datalen);
	for (i = 0; i < msgDataLen; i++)
		msgData[i] = UART_readbyte();
	for (; i < datalen; i++)
		UART_readbyte();

#ifdef ACT_AS_GATEWAY
	if (bGateway && BOARD_SEQNO != dest && datalen< sizeof(msgData)-6)
	{
		// memcpy
		for (; msgDataLen>0; msgDataLen--)
			msgData[msgDataLen +3] = msgData[msgDataLen-1];
		msgData[0] = cmd;
		*((uint*) &msgData[1]) = datalen;
		forward(src, dest, msgData, datalen+3);
		
		return;
	}
#endif //  ACT_AS_GATEWAY

	set485Dir(0); // change the 485 direction to send data
	UART_writebyte(0x87);
	UART_writebyte(src);
	UART_writebyte(dest);
	UART_writebyte(RESP(cmd));

	switch (cmd)
	{
	case CMD_Ping:
		UART_writeword(0x01);      // zero lengh of the msg data
		UART_writebyte(MCU_state); // send the state byte
		break;

	case CMD_Reset:
		if (dest == BOARD_SEQNO) // reset only support peer-to-peer, broadcast is forbidden
		{
			MCU_softReset();
			UART_writeword(0x00);
		}
		break;

	case CMD_SetState:
		MCU_state = msgData[0];
		//		// no need to respond if it is a boardcast to change state
		//		if ((srcdest >>4) == BOARD_SEQNO) // if it is a per-node change state, respond with a msg
		//		{
		UART_writeword(0x01);
		UART_writebyte(MCU_state);
		//		}
		break;

#ifdef NODE_TYPE_BASE
	case CMD_GetLatchStates:
		doGetLatchStates();
		break;

	case CMD_SetLatchStates:
		doSetLatchStates();
		break;

	case CMD_EnableLatchCh:
		doSetLatchCh(1);
		break;

	case CMD_DisableLatchCh:
		doSetLatchCh(0);
		break;
#endif // NODE_TYPE_BASE

	default:
		UART_writeword(0x00); // zero lenght of the msg data
		break;
	}

	set485Dir(1);
	ES =1;
	return;
}

#ifdef NODE_TYPE_BASE
void doSendIr()
{
	// message data 
	//  bytes 1           byte2                    byte N-1
	// +--------+--------+-----------------+-...-+-----------------+
	// |  chId  | IrType |              data to send               |
	// +--------+--------+-----------------+-...-+-----------------+
	//  0               7 0                       0               7

	IrIO idata *p = IrIOs;
	uchar irByte = msgData[0], chId=0, i;
	chId = irByte & 0x0F;
	irByte >>=4;
	// address the Ir IO sender
	for (; p; p++)
	{
		if (irByte == p->IrType)
			break;
	}

	UART_writeword(1); // size of error code
	if (msgDataLen < 2)
	{
		UART_writebyte(0xf0); // invalid length of message
		return;
	}

	if (!p)
	{
		UART_writebyte(0xf0 +1); // unkonwn IR type
		return;
	}

	openAnalogSignal(irByte & 0x0F); // open the channel
	// read the COM data into the buffer
	i = p->IrSend();
	UART_writebyte(0xc0 + i);
}
#endif // NODE_TYPE_BASE


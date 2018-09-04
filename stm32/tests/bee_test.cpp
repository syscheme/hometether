extern "C" {
#include "../common/bee_net.h"
//@param[in] heardfrom is the local bound interface thru which the message is received
void BeeMAC_procPacketReceived(BeeIF* beeIf, uint8_t* msgbuf, uint16_t msglen, uint8_t heardfrom);
}

#define BEE_LOCAL_NODEID 0x55

uint8_t BeeMAC_doSend(BeeIF* beeIf, uint8_t nextHop, uint8_t* data, uint8_t datalen)
{
	BeeMAC_procPacketReceived(beeIf, data, datalen, 0x67);
	return datalen;
}

uint8_t BeeNet_OnDataArrived(BeeIF* beeIf, BeeAddress srcAddr, BeeAddress destAddr, uint8_t* data, uint16_t datalen)
{
	return 0;
}

void BeeNet_OnPong(BeeIF* beeIf, BeeAddress srcAddr, BeeNodeId_t nextHop, uint8_t hops)
{
}

/////////////////////////////////////////


char gmsgbuf[100];
void main()
{
	int i =0;
	uint8_t bitmap[20];
	int16_t size =100, offset= size; // 777 + i*55

	char msg[150]="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	BeeAddress srcAddr, destAddr, addr1, addr2, addr3;
	srcAddr.dw=0x012345be;
	destAddr.dw=0x89abcdbe;
	addr1.dw=0x11abcdef; addr2.dw=0x22abcdef; addr3.dw=0x33abcdef;

/*
	memset(bitmap, 0x00, sizeof(bitmap));
	while (offset != 0)
	{
		uint8_t idx, flag;
		offset -= MAC_DATA_PAYLOAD;
		if (offset <0)
			offset = 0;

		calcBitIdx(offset, size, &idx, &flag);
		bitmap[idx] |= flag;
		printf("offset[%d] of size[%d] => bitmap[%d]=%02x, noff[%d]\n", offset, size, idx, flag, calcOffset(size, idx, flag));
	}
*/
	BeeIF thisBeeIF;

	Bee_init(&thisBeeIF, BEE_LOCAL_NODEID);

	BeeRoute_add(srcAddr, 0x33, 0);
	BeeRoute_add(destAddr, 0x33, 1);
	BeeRoute_add(addr1, 0x44, 1);
	BeeRoute_add(addr2, 0x44, 1);
	BeeRoute_add(addr3, 0x44, 1);
	int8_t aaa = BeeRoute_lookfor(&thisBeeIF, destAddr);
	int8_t bbb = BeeRoute_lookfor(&thisBeeIF, addr1);

	BeeRoute_delete(addr1, 0x44);
//	BeeRoute_delete(srcAddr, 0x33);




	BeeMAC_transmit(&thisBeeIF, (uint8_t*)gmsgbuf, srcAddr, destAddr, (uint8_t*)msg, strlen(msg));
}

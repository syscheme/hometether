/*********************************************
* vim:sw=8:ts=8:si:et
* To use the above modeline in vim you must have "set modeline" in your .vimrc
*
* Author: Guido Socher 
* Copyright: GPL V2
* See http://www.gnu.org/licenses/gpl.html
*
* IP, Arp, UDP and TCP functions.
*
* The TCP implementation uses some size optimisations which are valid
* only if all data can be sent in one single packet. This is however
* not a big limitation for a microcontroller as you will anyhow use
* small web-pages. The TCP stack is therefore a SDP-TCP stack (single data packet TCP).
*
* Chip type           : ATMEGA88 with ENC28J60
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
#include "ip_arp_udp_tcp.h"
#include "net.h"

#define  pgm_read_byte(ptr)  ((char)*(ptr))
#define ShiftToWordH(_B) (((uint16_t) _B)<<8)


static uint8_t wwwport=80;
static uint8_t macaddr[6];
static uint8_t ipaddr[4];
static int16_t info_hdr_len=0;
static int16_t info_data_len=0;
static uint8_t seqnum=0xa; // my initial tcp sequence number

// The Ip checksum is calculated over the ip header only starting
// with the header length field and a total length of 20 bytes
// unitl ip.dst
// You must set the IP checksum field to zero before you start
// the calculation.
// len for ip is 20.
//
// For UDP/TCP we do not make up the required pseudo header. Instead we 
// use the ip.src and ip.dst fields of the real packet:
// The udp checksum calculation starts with the ip.src field
// Ip.src=4bytes,Ip.dst=4 bytes,Udp header=8bytes + data length=16+len
// In other words the len here is 8 + length over which you actually
// want to calculate the checksum.
// You must set the checksum field to zero before you start
// the calculation.
// len for udp is: 8 + 8 + data length
// len for tcp is: 4+4 + 20 + option len + data length
//
// For more information on how this algorithm works see:
// http://www.netfor2.com/checksum.html
// http://www.msc.uky.edu/ken/cs471/notes/chap3.htm
// The RFC has also a C code example: http://www.faqs.org/rfcs/rfc1071.html
uint16_t checksum(uint8_t *packet, uint16_t len, uint8_t type)
{
	// type 0=ip 
	//      1=udp
	//      2=tcp
	uint32_t sum = 0;

	if (type==1)
	{
		sum+=IP_PROTO_UDP_V; // protocol udp
		// the length here is the length of udp (data+header len)
		// =length given to this function - (IP.scr+IP.dst length)
		sum+=len-8; // = real tcp len
	}

	if (type==2)
	{
		sum+=IP_PROTO_TCP_V; 
		// the length here is the length of tcp (data+header len)
		// =length given to this function - (IP.scr+IP.dst length)
		sum+=len-8; // = real tcp len
	}

	// build the sum of 16bit words
	while (len >1)
	{
		sum += 0xFFFF & (ShiftToWordH(*packet)|*(packet+1));
		packet+=2;
		len-=2;
	}

	// if there is a byte left then add it (padded with zero)
	if (len)
		sum += ((uint32_t)(0xFF & *packet))<<8;

	// now calculate the sum over the bytes in the sum
	// until the result is only 16bit long
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// build 1's complement:
	return ( (uint16_t) sum ^ 0xFFFF);
}

// you must call this function once before you use any of the other functions:
void init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint8_t wwwp)
{
	uint8_t i=0;
	wwwport=wwwp;
	for (i=0; i<4; i++)
		ipaddr[i]=myip[i];

	for (i=0; i<6; i++)
		macaddr[i]=mymac[i];
}

uint8_t eth_type_is_arp_and_my_ip(uint8_t *packet, uint16_t len)
{
	uint8_t i=0;
	//  
	if (len<41)
		return(0);

	if (packet[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || packet[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V)
		return(0);

	for (i=0; i<4; i++)
	{
		if (packet[ETH_ARP_DST_IP_P+i] != ipaddr[i])
			return(0);
	}

	return(1);
}

uint8_t eth_type_is_ip_and_my_ip(uint8_t *packet,uint16_t len)
{
	uint8_t i=0;
	//eth+ip+udp header is 42
	if (len<42)
		return(0);

	if (packet[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V || packet[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V)
		return(0);

	if (packet[IP_HEADER_LEN_VER_P]!=0x45)
		return(0); // must be IP V4 and 20 byte header

	for (i=0; i<4; i++)
	{
		if (packet[IP_DST_P+i]!=ipaddr[i])
			return(0);
	}

	return(1);
}
// make a return eth header from a received eth packet
void make_eth(uint8_t *packet, uint8_t* destMac)
{
	uint8_t i=0;
	if (NULL == destMac)
		destMac = &packet[ETH_SRC_MAC];
	//
	//copy the destination mac from the source and fill my mac into src
	for (i=0; i<6; i++)
	{
		packet[ETH_DST_MAC +i]=*destMac++;
		packet[ETH_SRC_MAC +i]=macaddr[i];
	}
}

void fill_ip_hdr_checksum(uint8_t *packet)
{
	uint16_t ck;
	// clear the 2 byte checksum
	packet[IP_CHECKSUM_P]=0;
	packet[IP_CHECKSUM_P+1]=0;
	packet[IP_FLAGS_P]=0x40; // don't fragment
	packet[IP_FLAGS_P+1]=0;  // fragement offset
	packet[IP_TTL_P]=64; // ttl

	// calculate the checksum:
	ck=checksum(&packet[IP_P], IP_HEADER_LEN,0);
	packet[IP_CHECKSUM_P]=ck>>8;
	packet[IP_CHECKSUM_P+1]=ck& 0xff;
}

// make a return ip header from a received ip packet
// destIp==NULL will be known to create a rely packet of the current
void make_ip(uint8_t *packet, uint8_t* destIp)
{
	uint8_t i=0;
	if (NULL == destIp)
		destIp = &packet[IP_SRC_P];

	for (i=0; i<4; i++)
	{
		packet[IP_DST_P+i]=*destIp++;
		packet[IP_SRC_P+i]=ipaddr[i];
	}

	fill_ip_hdr_checksum(packet);
}

// make a return tcp header from a received tcp packet
// rel_ack_num is how much we must step the seq number received from the
// other side. We do not send more than 255 bytes of text (=data) in the tcp packet.
// If mss=1 then mss is included in the options list
//
// After calling this function you can fill in the first data byte at TCP_OPTIONS_P+4
// If cp_seq=0 then an initial sequence number is used (should be use in synack)
// otherwise it is copied from the packet we received
void make_tcphead(uint8_t *packet,uint16_t rel_ack_num,uint8_t mss,uint8_t cp_seq)
{
	uint8_t i=0;
	uint8_t tseq;
	for(i =0; i<2; i++)
	{
		packet[TCP_DST_PORT_H_P+i]=packet[TCP_SRC_PORT_H_P+i];
		packet[TCP_SRC_PORT_H_P+i]=0; // clear source port
	}

	// set source port  (http):
	packet[TCP_SRC_PORT_L_P]=wwwport;

	// sequence numbers:
	// add the rel ack num to SEQACK
	for(i=4; i>0; i--)
	{
		rel_ack_num=packet[TCP_SEQ_H_P+i-1]+rel_ack_num;
		tseq=packet[TCP_SEQACK_H_P+i-1];
		packet[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
		if (cp_seq)
			// copy the acknum sent to us into the sequence number
			packet[TCP_SEQ_H_P+i-1]=tseq;
		else
			packet[TCP_SEQ_H_P+i-1]= 0; // some preset vallue

		rel_ack_num=rel_ack_num>>8;
	}

	if (cp_seq==0)
	{
		// put inital seq number
		packet[TCP_SEQ_H_P+0]= 0;
		packet[TCP_SEQ_H_P+1]= 0;
		// we step only the second byte, this allows us to send packts 
		// with 255 bytes or 512 (if we step the initial seqnum by 2)
		packet[TCP_SEQ_H_P+2]= seqnum; 
		packet[TCP_SEQ_H_P+3]= 0;
		// step the inititial seq num by something we will not use
		// during this tcp session:
		seqnum+=2;
	}

	// zero the checksum
	packet[TCP_CHECKSUM_H_P]=0;
	packet[TCP_CHECKSUM_L_P]=0;

	// The tcp header length is only a 4 bit field (the upper 4 bits).
	// It is calculated in units of 4 bytes. 
	// E.g 24 bytes: 24/4=6 => 0x60=header len field
	//packet[TCP_HEADER_LEN_P]=(((TCP_HEADER_LEN_PLAIN+4)/4)) <<4; // 0x60
	if (mss)
	{
		// the only option we set is MSS to 1408:
		// 1408 in hex is 0x580
		packet[TCP_OPTIONS_P]=2;
		packet[TCP_OPTIONS_P+1]=4;
		packet[TCP_OPTIONS_P+2]=0x05; 
		packet[TCP_OPTIONS_P+3]=0x80;
		// 24 bytes:
		packet[TCP_HEADER_LEN_P]=0x60;
	}
	else
	{
		// no options:
		// 20 bytes:
		packet[TCP_HEADER_LEN_P]=0x50;
	}
}

void arp_answer_from_request(void* nic, uint8_t *packet)
{
	uint8_t i=0;
	//
	make_eth(packet, NULL);
	packet[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
	packet[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
	// fill the mac addresses:
	for (i=0; i<6; i++)
	{
		packet[ETH_ARP_DST_MAC_P+i]=packet[ETH_ARP_SRC_MAC_P+i];
		packet[ETH_ARP_SRC_MAC_P+i]=macaddr[i];
	}

	for(i=0; i<4; i++)
	{
		packet[ETH_ARP_DST_IP_P+i]=packet[ETH_ARP_SRC_IP_P+i];
		packet[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
	}

	// eth+arp is 42 bytes:
	NIC_sendPacket(nic, packet, 42);
}

void echo_reply_from_request(void* nic, uint8_t *packet,uint16_t len)
{
	make_eth(packet, NULL);
	make_ip(packet, NULL);
	packet[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
	// we changed only the icmp.type field from request(=8) to reply(=0).
	// we can therefore easily correct the checksum:
	if (packet[ICMP_CHECKSUM_P] > (0xff-0x08))
		packet[ICMP_CHECKSUM_P+1]++;

	packet[ICMP_CHECKSUM_P]+=0x08;
	//
	NIC_sendPacket(nic, packet, len);
}

// you can send a max of 220 bytes of data
void udp_reply_from_request(void* nic, uint8_t *packet, char *data, uint8_t datalen, uint16_t port)
{
	uint8_t i=0;
	uint16_t ck;
	make_eth(packet, NULL);
	if (datalen>220)
		datalen=220;

	// total length field in the IP header must be set:
	packet[IP_TOTLEN_H_P] =0;
	packet[IP_TOTLEN_L_P] =IP_HEADER_LEN +UDP_HEADER_LEN +datalen;
	make_ip(packet, NULL);
	packet[UDP_DST_PORT_H_P]=port>>8;
	packet[UDP_DST_PORT_L_P]=port & 0xff;
	// source port does not matter and is what the sender used.

	// calculte the udp length:
	packet[UDP_LEN_H_P]=0;
	packet[UDP_LEN_L_P]=UDP_HEADER_LEN +datalen;
	// zero the checksum
	packet[UDP_CHECKSUM_H_P]=0;
	packet[UDP_CHECKSUM_L_P]=0;

	// copy the data:
	for (i=0; i<datalen; i++)
		packet[UDP_DATA_P+i]=data[i];

	ck=checksum(&packet[IP_SRC_P], 16 + datalen,1);
	packet[UDP_CHECKSUM_H_P]=ck >>8;
	packet[UDP_CHECKSUM_L_P]=ck &0xff;

	NIC_sendPacket(nic, packet, UDP_HEADER_LEN +IP_HEADER_LEN +ETH_HEADER_LEN +datalen);
}

void tcp_synack_from_syn(void* nic, uint8_t *packet)
{
	uint16_t ck;
	make_eth(packet, NULL);
	// total length field in the IP header must be set:
	// 20 bytes IP + 24 bytes (20tcp+4tcp options)
	packet[IP_TOTLEN_H_P]=0;
	packet[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
	make_ip(packet, NULL);
	packet[TCP_FLAGS_P]=TCP_FLAGS_SYNACK_V;
	make_tcphead(packet,1,1,0);
	// calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
	ck=checksum(&packet[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
	packet[TCP_CHECKSUM_H_P]=ck>>8;
	packet[TCP_CHECKSUM_L_P]=ck& 0xff;

	// add 4 for option mss:
	NIC_sendPacket(nic, packet, IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN);
}

// get a pointer to the start of tcp data in packet
// Returns 0 if there is no data
// You must call init_len_info once before calling this function
uint16_t get_tcp_data_pointer(void)
{
	if (info_data_len)
		return((uint16_t)TCP_SRC_PORT_H_P+info_hdr_len);
	else return(0);
}

// do some basic length calculations and store the result in static varibales
void init_len_info(uint8_t *packet)
{
	info_data_len= ShiftToWordH(packet[IP_TOTLEN_H_P]) | (packet[IP_TOTLEN_L_P]&0xff);
	info_data_len-=IP_HEADER_LEN;
	info_hdr_len=(packet[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
	info_data_len-=info_hdr_len;
	if (info_data_len<=0)
		info_data_len=0;
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t fill_tcp_data_p(uint8_t *packet, uint16_t pos, const int8_t *progmem_s)
{
	char c;
	// fill in tcp data at position pos
	//
	// with no options the data starts after the checksum + 2 more bytes (urgent ptr)
	while ((c = pgm_read_byte(progmem_s++)))
	{
		packet[TCP_CHECKSUM_L_P+3+pos]=c;
		pos++;
	}

	return(pos);
}

// fill in tcp data at position pos. pos=0 means start of
// tcp data. Returns the position at which the string after
// this string could be filled.
uint16_t fill_tcp_data(uint8_t *packet, uint16_t pos, const char *s)
{
	// fill in tcp data at position pos
	//
	// with no options the data starts after the checksum + 2 more bytes (urgent ptr)
	while (*s)
	{
		packet[TCP_CHECKSUM_L_P+3+pos]=*s;
		pos++;
		s++;
	}

	return(pos);
}

// Make just an ack packet with no tcp data inside
// This will modify the eth/ip/tcp header 
void tcp_ack_from_any(void* nic, uint8_t *packet)
{
	uint16_t j;
	make_eth(packet, NULL);
	// fill the header:
	packet[TCP_FLAGS_P]=TCP_FLAGS_ACK_V;
	if (info_data_len==0)
	{
		// if there is no data then we must still acknoledge one packet
		make_tcphead(packet,1,0,1); // no options
	}
	else
		make_tcphead(packet,info_data_len,0,1); // no options

	// total length field in the IP header must be set:
	// 20 bytes IP + 20 bytes tcp (when no options) 
	j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
	packet[IP_TOTLEN_H_P]=j>>8;
	packet[IP_TOTLEN_L_P]=j& 0xff;
	make_ip(packet, NULL);
	// calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
	j=checksum(&packet[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
	packet[TCP_CHECKSUM_H_P]=j>>8;
	packet[TCP_CHECKSUM_L_P]=j& 0xff;

	NIC_sendPacket(nic, packet, IP_HEADER_LEN +TCP_HEADER_LEN_PLAIN +ETH_HEADER_LEN);
}

// you must have called init_len_info at some time before calling this function
// dlen is the amount of tcp data (http data) we send in this packet
// You can use this function only immediately after make_tcp_ack_from_any
// This is because this function will NOT modify the eth/ip/tcp header except for
// length and checksum
void tcp_ack_with_data(void* nic, uint8_t *packet,uint16_t dlen)
{
	uint16_t j;
	// fill the header:
	// This code requires that we send only one data packet
	// because we keep no state information. We must therefore set
	// the fin here:
	packet[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V;

	// total length field in the IP header must be set:
	// 20 bytes IP + 20 bytes tcp (when no options) + len of data
	j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen;
	packet[IP_TOTLEN_H_P]=j>>8;
	packet[IP_TOTLEN_L_P]=j& 0xff;
	fill_ip_hdr_checksum(packet);

	// zero the checksum
	packet[TCP_CHECKSUM_H_P]=0;
	packet[TCP_CHECKSUM_L_P]=0;

	// calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
	j=checksum(&packet[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlen,2);
	packet[TCP_CHECKSUM_H_P]=j>>8;
	packet[TCP_CHECKSUM_L_P]=j& 0xff;

	NIC_sendPacket(nic, packet, IP_HEADER_LEN +TCP_HEADER_LEN_PLAIN +dlen +ETH_HEADER_LEN);
}

uint16_t fill_udp_data_p(uint8_t *packet, uint16_t pos, const int8_t *data, uint8_t datalen)
{
	int8_t i=datalen;

	if (i + pos >220)
		i = i - pos;

	if (i <=0)
		return 0;

	// copy the data:
	for (datalen =i, i=0; i<datalen; i++)
		packet[UDP_DATA_P+i]=data[i];

	return (pos+i);
}

// you can send a max of 220 bytes of data
void udp_reply_with_data(void* nic, uint8_t *packet, uint16_t datalen, uint16_t port)
{
	uint16_t ck;
	make_eth(packet, NULL);
	if (datalen>1300)
		datalen=1300;

	// total length field in the IP header must be set:
	ck = IP_HEADER_LEN +UDP_HEADER_LEN +datalen;
	packet[IP_TOTLEN_H_P] =(ck >>8);
	packet[IP_TOTLEN_L_P] =(ck & 0xff);

	make_ip(packet, NULL);
	packet[UDP_DST_PORT_H_P]=port>>8;
	packet[UDP_DST_PORT_L_P]=port & 0xff;
	// source port does not matter and is what the sender used.

	// calculte the udp length:
	ck = UDP_HEADER_LEN +datalen;
	packet[UDP_LEN_H_P] =(ck >>8);
	packet[UDP_LEN_L_P] =(ck & 0xff);

	// zero the checksum
	packet[UDP_CHECKSUM_H_P]=0;
	packet[UDP_CHECKSUM_L_P]=0;

	ck=checksum(&packet[IP_SRC_P], 16 + datalen, 1);
	packet[UDP_CHECKSUM_H_P]=ck >>8;
	packet[UDP_CHECKSUM_L_P]=ck &0xff;

	NIC_sendPacket(nic, packet, UDP_HEADER_LEN +IP_HEADER_LEN +ETH_HEADER_LEN +datalen);
}

// initialize an out-going ip packet
// ******* IP *******
// ip.src
#define IP_SRC_P 0x1a
#define IP_DST_P 0x1e
#define IP_HEADER_LEN_VER_P 0xe
#define IP_CHECKSUM_P 0x18
#define IP_TTL_P 0x16
#define IP_FLAGS_P 0x14
#define IP_P 0xe
#define IP_TOTLEN_H_P 0x10
#define IP_TOTLEN_L_P 0x11

#define IP_PROTO_P 0x17  

#define IP_PROTO_ICMP_V 1
#define IP_PROTO_TCP_V  6
// 17=0x11
#define IP_PROTO_UDP_V  17

uint16_t lastSrcPort=0;

void mcast(void* nic, uint8_t *packet, uint8_t* groupIp, uint16_t destPort, uint16_t srcPort, uint16_t datalen)
{
	uint16_t ck;
	uint8_t  destMac[6] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0x00 };
	destMac[5] = groupIp[3];
	destMac[4] = groupIp[2];
	destMac[3] = groupIp[1] & 0x7f;
	groupIp[0] |= 0xe0;	groupIp[0] &= 0xef;

	packet[ETH_TYPE_H_P] =ETHTYPE_IP_H_V;
	packet[ETH_TYPE_L_P] =ETHTYPE_IP_L_V;
	make_eth(packet, destMac);

	if (datalen>1300)
		datalen=1300;

	// total length field in the IP header must be set:
	ck = IP_HEADER_LEN +UDP_HEADER_LEN +datalen;
	packet[IP_TOTLEN_H_P] =(ck >>8);
	packet[IP_TOTLEN_L_P] =(ck & 0xff);

 	packet[IP_HEADER_LEN_VER_P]=0x45;
	make_ip(packet, groupIp);

	packet[IP_PROTO_P] = IP_PROTO_UDP_V;

	if (0== srcPort)
		srcPort = ShiftToWordH(packet[UDP_SRC_PORT_H_P]) | packet[UDP_SRC_PORT_L_P];

	if (0== srcPort)
		srcPort = (lastSrcPort++%(60*1024)) + 1024;

	packet[UDP_SRC_PORT_H_P] = srcPort>>8;
	packet[UDP_SRC_PORT_L_P] = srcPort&0xff;

	packet[UDP_DST_PORT_H_P] = destPort>>8;
	packet[UDP_DST_PORT_L_P] = destPort&0xff;

	// calculte the udp length:
	ck = UDP_HEADER_LEN +datalen;
	packet[UDP_LEN_H_P] =(ck >>8);
	packet[UDP_LEN_L_P] =(ck & 0xff);

	// zero the checksum
	packet[UDP_CHECKSUM_H_P]=0;
	packet[UDP_CHECKSUM_L_P]=0;

	ck=checksum(&packet[IP_SRC_P], 16 + datalen, 1);
	packet[UDP_CHECKSUM_H_P]=ck >>8;
	packet[UDP_CHECKSUM_L_P]=ck &0xff;

	NIC_sendPacket(nic, packet, UDP_HEADER_LEN +IP_HEADER_LEN +ETH_HEADER_LEN +datalen);
}


/* end of ip_arp_udp.c */

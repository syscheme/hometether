/*********************************************
* vim:sw=8:ts=8:si:et
* To use the above modeline in vim you must have "set modeline" in your .vimrc
* Author: Guido Socher 
* Copyright: GPL V2
*
* IP/ARP/UDP/TCP functions
*
* Chip type           : ATMEGA88 with ENC28J60
*********************************************/
//@{
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

#ifndef __IP_ARP_UDP_TCP_H__
#define __IP_ARP_UDP_TCP_H__

#include "htcomm.h"

#ifndef nic_send_packet
#  include "enc28j60.h"
#  define nic_send_packet ENC28J60_sendPacket
#endif // nic_send_packet

// you must call this function once before you use any of the other functions:
extern void init_ip_arp_udp_tcp(const uint8_t* mymac, const uint8_t* myip, uint16_t wwwp);
//
extern uint8_t eth_type_is_arp_and_my_ip(uint8_t* packet, uint16_t len);
extern uint8_t eth_type_is_ip(uint8_t *packet, uint16_t len);
extern uint8_t is_to_myip(uint8_t *packet, uint16_t len);
extern uint8_t match_dest_ip(const uint8_t *destIp, uint8_t *packet, uint16_t len);
extern uint8_t match_src_ip(const uint8_t *sourceIp, uint8_t *packet, uint16_t len);

extern void arp_answer_from_request(void* nic, uint8_t* packet);
extern void echo_reply_from_request(void* nic, uint8_t* packet, uint16_t len);
extern void udp_reply_from_request(void* nic, uint8_t* packet, char* data, uint8_t datalen, uint16_t port);

extern uint16_t fill_udp_data_p(uint8_t *packet, uint16_t pos, const uint8_t *data, uint16_t datalen);
extern void udp_reply_with_data(void* nic, uint8_t *packet, uint16_t datalen, uint16_t port);

extern void tcp_synack_from_syn(void* nic, uint8_t* packet);
extern void init_len_info(uint8_t* packet);
extern uint16_t get_tcp_data_pointer(void);
extern uint16_t fill_tcp_data_p(uint8_t *packet, uint16_t pos, const uint8_t* s, uint16_t len);
extern uint16_t fill_tcp_data_str(uint8_t *packet, uint16_t pos, const char* str);

extern void tcp_ack_from_any(void* nic, uint8_t* packet);
extern void tcp_ack_with_data(void* nic, uint8_t* packet, uint16_t dlen);

// portal: to send a packet
// void NIC_sendPacket(void* nic, uint8_t* packet, uint16_t packetLen);

void mcast(void* nic, uint8_t *packet, const uint8_t* groupIp, uint16_t destPort, uint16_t srcPort, uint16_t datalen);



#endif // __IP_ARP_UDP_TCP_H__

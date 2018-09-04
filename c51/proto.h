#ifndef __HC_Prot_H__
#define __HC_Prot_H__

/* message format
  +-----------------+--------+--------+-----------------+-----------------+-----------------+-----------------+
  |      0x87       |     destaddr    |     srcaddr     |     command     |            data-length            |
  +--------+--------+-----------------+--------+--------+-----------------+-----------------+-----------------+
  |      byte1      |      byte2      |       byte3     |      byte4      |      byte5      |    byte6        |
  +-----------------+-----------------+-----------------+-----------------+-----------------+-----------------+

  leading byte = 0x87
  1-byte dest addr, 0xff means boardcast
  1-byte source addr and
  7-bit command with the highest bit 0 as request, 1 as response
  2-byte length to follow up to 65535 bytes of data. the data bytes are up to the each command to define
*/

#define RESP(_CMD)           (0x80 | _CMD)
enum {
	CMD_Ping,        			// PING with dest=0xF means heartbeat, 
	CMD_SetTimeout,
	CMD_SetState,			    // host to force the slave MCU enter the destination state
	CMD_GetAll,      			// to get all the on-board status
	CMD_GetLatchStates,    		// to get the byte value of latch
	CMD_SetLatchStates,
	CMD_EnableLatchCh,
	CMD_DisableLatchCh,
	CMD_GetOneAdcCh,			// to get the byte value of a specified ADC0838 channel, the askee must return the latest value then perform a new conversion
	CMD_GetAllAdcCh,  			// to get the latest byte values of all ADC0838 channels
	CMD_GetDS1820Chs,  			// to get the latest word status of all the DS1820 channels on the board
	CMD_SetDS1820Chs,			// to specify which siginal channels should work as DS1820 channels
	CMD_GetMotionChs,			// to get the latest word status of all the motion channels on the board
	CMD_SetMotionChs,			// to specify which siginal channels should work as motion channels
	CMD_Reset,					// to softreset a MCU if supported
	CMD_AppendFile,		        // host to push a file segment to the slave MCU
	CMD_ReadFile,		        // host to pull a file segment from the slave MCU
	CMD_NOP
};

extern uchar xdata msgData[]; // the common buffer
extern uchar xdata msgDataLen;

code enum _IrTypes
{
	Ir_M50560,
	Ir_u6121,
	Ir_tc9012, 
	Ir_m3004
};

// the impl of IrRead will read IR_REV for the IR signal, the impl of IrSend will send the IR signal after the switched ANALOG_SIG
typedef BYTE (*FuncIrIO) (void); 
typedef struct
{
	BYTE IrType;
	FuncIrIO IrRead;
	FuncIrIO IrSend;
} IrIO;

extern IrIO code IrIOs[];


#endif // __HC_Prot_H__

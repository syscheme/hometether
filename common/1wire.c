#include "1wire.h"

uint8_t* OWFirst(const OneWire* hostPin);
uint8_t* OWNext(void);
BOOL OWVerify(const OneWire* hostPin, OneWireAddr_t rom);
void OWTargetSetup(unsigned char family_code);
void OWFamilySkipSetup(void);

BOOL  OWSearch(void);
// unsigned char docrc8(unsigned char value);

// global search state
unsigned char ROM_NO[8];
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
unsigned char crc8;
const OneWire* busCtx;

hterr OneWire_resetBus(const OneWire* hostPin)
{
	int i;
	pinSET(hostPin->pin,1);   delayXusec(50);   // 保持高电平一段时间时间
	pinSET(hostPin->pin,0);   delayXusec(600);  // 总线将其拉低电平, 延时400us-960us，这里延时600us
	pinSET(hostPin->pin,1);   delayXusec(40);   // 总线释放低电平, 延时15us-60us，这里延时30us
	for(i=0; pinGET(hostPin->pin) && i <10000; i++) // wait till the bus get released, maximally wait 100msec
		delayXusec(10);
	if (i >= 10000)	
		return ERR_ADDRFAULT;

	delayXusec(500);	                   // yield 500usec
	pinSET(hostPin->pin,1);                 // 总线将电平拉高

	return ERR_SUCCESS;
}

// Send 1 bit of data to teh 1-Wire chip
static void OWPortal_writeBit(const OneWire* hostPin, uint8_t bit_value)
{
	pinSET(hostPin->pin,0); delayXusec(5); //在15us内送数到数据线，在15-60us内采样

	if (bit_value)
		pinSET(hostPin->pin,1);

	delayXusec(65); //将剩余时间消耗完
	pinSET(hostPin->pin,1);	delayXusec(2); //写两个位之间间隔大于1us
}

static uint8_t OWPortal_readBit(const OneWire* hostPin)
{
	uint8_t bit =0;

	pinSET(hostPin->pin,0);   delayXusec(5); //从读时序开始到采样信号线必须在15us内，且采样尽量安排在15us最后
	pinSET(hostPin->pin,1);   delayXusec(5);  //释放总线，然后才能进行采样，否则无意义。只有低电平
	bit = (pinGET(hostPin->pin) ? 1:0);

	delayXusec(65); //将剩余时间消耗完
	pinSET(hostPin->pin,1);	delayXusec(2);

	return bit;
}

// Send 8 bits of data to the 1-Wire chip
void  OneWire_writeByte(const OneWire* hostPin, uint8_t byte_value)
{
	uint8_t i;

	for (i=8; i>0; i--)
	{
		OWPortal_writeBit(hostPin, byte_value & 0x01);
		byte_value >>= 1;
	}
}

uint8_t OneWire_readByte(const OneWire* hostPin)
{
	uint8_t i, data =0;
	for (i=8; i>0; i--)
	{
		data >>= 1;
		if (OWPortal_readBit(hostPin))
			data |= 0x80;
	}

	return data;
}

//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
static uint8_t* OWFirst(const OneWire* hostPin)
{
	// reset the search state
	LastDiscrepancy = 0;
	LastDeviceFlag = FALSE;
	LastFamilyDiscrepancy = 0;

	busCtx = hostPin;

	return OWSearch() ? ROM_NO : NULL;
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
static uint8_t* OWNext()
{
	// leave the search state alone
	return OWSearch() ? ROM_NO : NULL;
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
static BOOL OWSearch()
{
	int id_bit_number;
	int last_zero, rom_byte_number, search_result;
	int id_bit, cmp_id_bit;
	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;
	crc8 = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		if (ERR_SUCCESS != OneWire_resetBus(busCtx))
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command 
		OneWire_writeByte(busCtx, 0xF0);  

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = OWPortal_readBit(busCtx);
			cmp_id_bit = OWPortal_readBit(busCtx);

			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;

			// all devices coupled have 0 or 1
			if (id_bit != cmp_id_bit)
				search_direction = id_bit;  // bit write value for search
			else
			{
				// if this discrepancy if before the Last Discrepancy
				// on a previous next then pick the same as last time
				if (id_bit_number < LastDiscrepancy)
					search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
				else
					// if equal to last pick 1, if not then pick 0
					search_direction = (id_bit_number == LastDiscrepancy);

				// if 0 was picked then record its position in LastZero
				if (search_direction == 0)
				{
					last_zero = id_bit_number;

					// check for Last discrepancy in family
					if (last_zero < 9)
						LastFamilyDiscrepancy = last_zero;
				}
			}

			// set or clear the bit in the ROM byte rom_byte_number
			// with mask rom_byte_mask
			if (search_direction == 1)
				ROM_NO[rom_byte_number] |= rom_byte_mask;
			else
				ROM_NO[rom_byte_number] &= ~rom_byte_mask;

			// serial number search direction write bit
			OWPortal_writeBit(busCtx, search_direction);

			// increment the byte counter id_bit_number
			// and shift the mask rom_byte_mask
			id_bit_number++;
			rom_byte_mask <<= 1;

			// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
			if (rom_byte_mask == 0)
			{
				// docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
				calcCRC8(&ROM_NO[rom_byte_number], 1);
				rom_byte_number++;
				rom_byte_mask = 1;
			}

		} while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!((id_bit_number < 65) || (crc8 != 0)))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}

	return search_result;
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
//
BOOL OneWire_verify(const OneWire* hostPin, OneWireAddr_t rom)
{
	unsigned char rom_backup[8];
	int rslt,ld_backup,ldf_backup,lfd_backup;
	const OneWire* ctx_backup;

	// keep a backup copy of the current state
	memcpy(rom_backup, ROM_NO, sizeof(ROM_NO));
	ld_backup = LastDiscrepancy;
	ldf_backup = LastDeviceFlag;
	lfd_backup = LastFamilyDiscrepancy;
	ctx_backup = busCtx;

	// set search to find the same device
	memcpy(ROM_NO, &rom, sizeof(ROM_NO));
	LastDiscrepancy = 64;
	LastDeviceFlag = FALSE;
	busCtx = hostPin;

	if (!OWSearch())
		rslt = FALSE;
	else	
	{
		// check if same device found
		rslt = (0 == memcmp(ROM_NO, &rom, sizeof(ROM_NO))) ? TRUE : FALSE;
	}

	// restore the search state 
	memcpy(ROM_NO, rom_backup, sizeof(ROM_NO));
	LastDiscrepancy = ld_backup;
	LastDeviceFlag = ldf_backup;
	LastFamilyDiscrepancy = lfd_backup;
	busCtx = ctx_backup;

	// return the result of the verify
	return rslt;
}

//--------------------------------------------------------------------------
// Setup the search to find the device type 'family_code' on the next call
// to OWNext() if it is present.
//
void OWTargetSetup(unsigned char family_code)
{
	int i;

	// set the search state to find SearchFamily type devices
	ROM_NO[0] = family_code;
	for (i = 1; i < 8; i++)
		ROM_NO[i] = 0;

	LastDiscrepancy = 64;
	LastFamilyDiscrepancy = 0;
	LastDeviceFlag = FALSE;
}

//--------------------------------------------------------------------------
// Setup the search to skip the current device type on the next call
// to OWNext().
//
void OWFamilySkipSetup()
{
	// set the Last discrepancy to last family discrepancy
	LastDiscrepancy = LastFamilyDiscrepancy;
	LastFamilyDiscrepancy = 0;

	// check for end of list
	if (LastDiscrepancy == 0)
		LastDeviceFlag = TRUE;
}

uint8_t OneWire_scanForDevices(const OneWire* hostPin, OW_cbDeviceDetected_f cbDetected)
{
	uint8_t cFound =0;
	uint8_t* romAddr = NULL;
	// search for all devices on the 1 wire bus
	for (cFound=0, romAddr = OWFirst(hostPin); NULL != romAddr;	romAddr = OWNext(), cFound++)
	{
		if (NULL == cbDetected)
			continue;

		if (ERR_SUCCESS != cbDetected(hostPin, *((OneWireAddr_t*)romAddr)))
			break;
	}

	return cFound;
}

//--------------------------------------------------------------------------
// 1-Wire Functions to be implemented for a particular platform
//--------------------------------------------------------------------------

/*
//--------------------------------------------------------------------------
// Reset the 1-Wire bus and return the presence of any device
// Return TRUE  : device present
//        FALSE : no device present
//
BOOL OWReset()
{
// platform specific
// TMEX API TEST BUILD
return (TMTouchReset(session_handle) == 1);
}

//--------------------------------------------------------------------------
// Send 8 bits of data to the 1-Wire bus
//
void OneWire_writeByte(unsigned char byte_value)
{
// platform specific

// TMEX API TEST BUILD
TMTouchByte(session_handle,byte_value);
}

//--------------------------------------------------------------------------
// Send 1 bit of data to teh 1-Wire bus
//
void OWPortal_writeBit(hostPin, unsigned char bit_value)
{
// platform specific

// TMEX API TEST BUILD
TMTouchBit(session_handle,(short)bit_value);
}

//--------------------------------------------------------------------------
// Read 1 bit of data from the 1-Wire bus 
// Return 1 : bit read is 1
//        0 : bit read is 0
//
unsigned char OWPortal_readBit(hostPin)
{
// platform specific

// TMEX API TEST BUILD
return (unsigned char)TMTouchBit(session_handle,0x01);

}

// TEST BUILD
static unsigned char dscrc_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
	// See Application Note 27

	// TEST BUILD
	crc8 = dscrc_table[crc8 ^ value];
	return crc8;
}

*/

/*
//--------------------------------------------------------------------------
// TEST BUILD MAIN
//
int main(short argc, char **argv)
{
short PortType=5,PortNum=1;
int rslt,i,cnt;

// TMEX API SETUP
// get a session
session_handle = TMExtendedStartSession(PortNum, PortType, NULL);
if (session_handle <= 0)
{
printf("No session, %d\n",session_handle);
exit(0);
}

// setup the port
rslt = TMSetup(session_handle);
if (rslt != 1)
{
printf("Fail setup, %d\n",rslt);
exit(0);
}
// END TMEX API SETUP

// find ALL devices
printf("\nFIND ALL\n");
cnt = 0;
rslt = OWFirst();
while (rslt)
{
// print device found
for (i = 7; i >= 0; i--)
printf("%02X", ROM_NO[i]);
printf("  %d\n",++cnt);

rslt = OWNext();
}

// find only 0x1A
printf("\nFIND ONLY 0x1A\n");
cnt = 0;
OWTargetSetup(0x1A);
while (OWNext())
{
// check for incorrect type
if (ROM_NO[0] != 0x1A)
break;

// print device found
for (i = 7; i >= 0; i--)
printf("%02X", ROM_NO[i]);
printf("  %d\n",++cnt);
}

// find all but 0x04, 0x1A, 0x23, and 0x01
printf("\nFIND ALL EXCEPT 0x10, 0x04, 0x0A, 0x1A, 0x23, 0x01\n");
cnt = 0;
rslt = OWFirst();
while (rslt)
{
// check for incorrect type
if ((ROM_NO[0] == 0x04) || (ROM_NO[0] == 0x1A) || 
(ROM_NO[0] == 0x01) || (ROM_NO[0] == 0x23) ||
(ROM_NO[0] == 0x0A) || (ROM_NO[0] == 0x10))
OWFamilySkipSetup();
else
{
// print device found
for (i = 7; i >= 0; i--)
printf("%02X", ROM_NO[i]);
printf("  %d\n",++cnt);
}

rslt = OWNext();
}

// TMEX API CLEANUP
// release the session
TMEndSession(session_handle);
// END TMEX API CLEANUP
}

*/

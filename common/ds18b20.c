#include "ds18b20.h"

enum DS18B20_CMD
{
	DSCMD_MatchROM  = 0x55,
	DSCMD_SkipROM   = 0xcc,
	DSCMD_Convert   = 0x44,
	DSCMD_Read      = 0xbe,
};

//初始化一个DS18B20	data structure
void DS18B20_init(DS18B20* chip, OneWire* bus, OneWireAddr_t rom)
{
	if (chip && bus)
	{
		chip->bus = bus;
		chip->rom = rom;
	}

	if (DSCMD_SkipROM== chip->rom || 0xffffffff == chip->rom)
		chip->rom = 0;
}

static void DS18B20_cmd(DS18B20* chip, uint8_t cmd)
{
	uint8_t *pROM=NULL, i;
	if (NULL == chip)
		return;

	// step 1. reset DS18B20
	OneWire_resetBus(chip->bus);

	// step 2. send the byte 0xcc and 0xbe
	if (0 == chip->rom)
		OneWire_writeByte(chip->bus, DSCMD_SkipROM);
	else
	{
		pROM=(uint8_t*) &chip->rom;
		OneWire_writeByte(chip->bus, DSCMD_MatchROM);
		for (i=0; i<8; i++)// 64bit ROM
			OneWire_writeByte(chip->bus, *pROM++);
	}

	OneWire_writeByte(chip->bus, cmd);
}

void DS18B20_convert(DS18B20* chip)
{
	if (NULL == chip)
		return;

	DS18B20_cmd(chip, DSCMD_Convert);
}

int16_t DS18B20_read(DS18B20* chip)
{
	int16_t data =0;
	
	if (NULL == chip)
		return 0;

	DS18B20_cmd(chip, DSCMD_Read);

	// step 3 read the value
	data =  OneWire_readByte(chip->bus); // the low byte
	data |= (OneWire_readByte(chip->bus) << 8); // the high byte

	if (data == -1) // test if read succeeded
		data =DS18B20_INVALID_VALUE;
	else
	{
		// step 4. calcuate the true temperature value
		//   every 1 means 1/16 degree in C, convert it to dec via wTemp = Round(wTemp/16 *10 +0.5)
		data = data *10 +8;
		data >>= 4;	 // =/16
	}

	DS18B20_convert(chip);
	if (data <-800 || data >800 ) // take [-80C, 80C] as valid range
		data =DS18B20_INVALID_VALUE;

	// step 5. return the result
	return data; // the return value is in 0.1 degree, for example, 238 mean 23.8C
}

//read the DHT-11 integrated temperture and humidity sensor
//@output *temperature - temperature in 0.1 Celsius degree
//@output *humidity - humidity in 0.1%
hterr DHT11_read(const OneWire* hostPin, uint16_t* temperature, uint8_t* humidity)
{
	uint8_t i, n, j;
	uint8_t data[5] = {0,0,0,0,0};
	// step 1. reset the bus
	pinSET(hostPin->pin,1);   delayXmsec(1);   // 保持高电平一段时间时间
	pinSET(hostPin->pin,0);   delayXmsec(20);   // keep L no less than 18msec
	pinSET(hostPin->pin,1);   delayXusec(35);   // 总线释放低电平, 延时15us-60us，这里延时30us
	for(i=255; pinGET(hostPin->pin) && i; i--) // wait till the bus get released, maximally wait 2.5msec
		delayXusec(10);
	if (0 == i)	
		return ERR_ADDRFAULT;

	// test the 80us DHT11's L output
	for(i=255; !pinGET(hostPin->pin) && i; i--) // wait till the bus get released, maximally wait 2.5msec
		delayXusec(10);
	if (0 == i)	
		return ERR_ADDRFAULT;

	// test the 80us DHT11's H output
	for(i=255; pinGET(hostPin->pin) && i; i--) // wait till the bus get released, maximally wait 10msec
		delayXusec(10);
	if (0 == i)	
		return ERR_ADDRFAULT;

	// step3. start reading the five bytes: 0-humidity, 1-humidity decimals, 2-temperture, 3-temperture decimals, 4-checksum 
	for (n=0; n<5; n++)
	{
		for (j=0; j<8; j++)
		{
			data[n] <<=1;
			// wait till H
			for (i=255; !pinGET(hostPin->pin) && i; i--) delayXusec(5);

			// read the value after 40usec
			delayXusec(40);
			if (pinGET(hostPin->pin))
			{
				data[n] |= 1;
				// wait till L
				for (i=255; pinGET(hostPin->pin) && i; i--) delayXusec(5);
			}
		}
	}

	delayXmsec(1);
	pinSET(hostPin->pin,1);

	// step4. validate the checksum
	i = data[0] + data[1] + data[2] + data[3];
	if (data[4] != i)
		return ERR_CRC;

	// output the data
	if (NULL != temperature)
		*temperature = (uint16_t)data[2]*10 +((uint16_t)data[3])*10/256;

	if (NULL != humidity)
		*humidity = data[0];

	return ERR_SUCCESS;
}

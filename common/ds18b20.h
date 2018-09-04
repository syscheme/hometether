#ifndef __DS18B20_H__
#define __DS18B20_H__

// pin1-GND, pin2-Data, pin3-VDD

#include "htcomm.h"
#include "1wire.h"

typedef struct _DS18B20
{
	const OneWire* bus;
	OneWireAddr_t  rom;
} DS18B20;

#define DS18B20_INVALID_VALUE (999) // maps to an impossible temperature 99.9C

// ---------------------------------------------------------------------------
// DS18B20 methods
// ---------------------------------------------------------------------------

// Initialize a DS18B20 chip by specifying 1wire IO bus context and chip rom
// rom=0 when there is a single device on the bus
void DS18B20_init(DS18B20* chip, OneWire* bus, OneWireAddr_t rom);

// read the temperature from a DS18B20 chip, the temperature value is in 0.1C
// DS18B20_INVALID_VALUE if failed to read
int16_t DS18B20_read(DS18B20* chip);

// ---------------------------------------------------------------------------
// DHT-11 methods
// ---------------------------------------------------------------------------
//@note: DHT-11 only allows a single one on the bus with no addressing, so read it directly

//read the DHT-11 integrated temperture and humidity sensor
//@output *temperature - temperature in 0.1 Celsius degree
//@output *humidity - humidity in 1%
hterr DHT11_read(const OneWire* hostPin, uint16_t* temperature, uint8_t* humidity);

#endif // __DS18B20_H__

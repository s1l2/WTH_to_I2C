// (c) valserivp@gmail.com

#include "wtr_i2c_slave.h"


class Twtr_i2c_slave {
	typedef struct TPacketData {
		union {
			byte abid[2];
			uint16_t id;
		};
		uint8_t timeLabelH;
		union{
			uint8_t timeLabelL4h;
			uint8_t temperatureH4l;
		};
		uint8_t temperatureL;
		uint8_t humidity;
		uint8_t crc;
	} TPacketData;
};
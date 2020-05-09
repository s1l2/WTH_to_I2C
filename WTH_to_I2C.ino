#include <Wire.h>
//#include <HardWire.h>
#include "treceiver.h"

void PrintData(const TH433ReceiverClass::packet_data& buffer);
uint8_t crc8(const uint8_t * data, const uint8_t size);
const char * toBin(uint8_t x);

typedef struct i2c_packet {
	union {
		byte abid[2]; // в самом младшем полубайте поместим тип датчика и номер канала
		uint16_t id;
	};
	uint8_t timeLabel;
	union {
		byte abtemperature[2]; // в самом старшем полубайте поместим номер датчика
		uint16_t temperature;
	};
	uint8_t humidity; // в старшем бите поместим флаг батареи
	//uint8_t reserved;
	uint8_t crc;

	void calc_crc(uint8_t pos) {
		abtemperature[1] = abtemperature[1] & 0b00001111 | (pos << 4);
		crc = crc8((uint8_t*)this, sizeof(*this) - 1);
	};
};

enum Settings :uint8_t {
	maxSensors = 0x0f
};
uint8_t currentSensor = 0, countSensors = 0;
i2c_packet i2c_data[Settings::maxSensors] = {};



void receiveEvent(int countToRead) {
	//Serial.println("Receive: ");

	while (0 < Wire.available()) {
		currentSensor = Wire.read();
		//Serial.println(currentSensor);
	}
}

void requestEvent() {
	static uint16_t empty = 0;
	//static uint8_t data[] = {0xf0, 0xff, 0x0f, 0x55};
//	Wire.write("hello "); // respond with message of 6 bytes
	//Wire.write(data, sizeof(data));
//	Wire.write(data[3]);
	//return;
	if (currentSensor >= countSensors) {
		//Wire.write((uint8_t*)nullptr, 0);
		Wire.write((const uint8_t *)&empty, 2);
		currentSensor = 0;
		//Serial.println("-");

	}
	else {
		Wire.write((uint8_t*) &(i2c_data[currentSensor]), sizeof(i2c_packet));
		//Wire.write(currentSensor);
		//Serial.println();
		currentSensor++;
		//Serial.println(currentSensor);
	}
	//Serial.println("Request event");
	//Serial.println(currentSensor);
}


void setup() {
	Serial.begin(115200);
	TH433Receiver.begin();
	Wire.begin(0x60);
	Wire.onRequest(requestEvent); // data request to slave
	Wire.onReceive(receiveEvent); // data slave received

}


void loop() {
	static i2c_packet tmpData;
	TH433ReceiverClass::packet_data* lpbuffer = TH433Receiver.next_data();
	if (lpbuffer) {
		TH433ReceiverClass::packet_data& buffer = *lpbuffer;

		PrintData(buffer);
		switch (lpbuffer->type) {
			case TH433ReceiverClass::SensorType::type0: {
				tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
				tmpData.abid[0] = buffer[1] & 0b00110000 | (lpbuffer->type << 2);
				tmpData.temperature = buffer[1] & 0b1111;
				tmpData.temperature = (tmpData.temperature << 8) | (buffer[2]); // 00001100|1011
				tmpData.temperature += 500;

				tmpData.humidity = ((buffer[3] << 4) | (buffer[4] >> 4)) & 0x7f;
				tmpData.humidity |= (buffer[1]) & 0b10000000; // battery
			}break;
			case TH433ReceiverClass::SensorType::type1:
			case TH433ReceiverClass::SensorType::type3: {

				if ( (lpbuffer->type == TH433ReceiverClass::SensorType::type1
					&& (buffer[0] & 0b11110000) ^ 0b01010000)
					|| (lpbuffer->type == TH433ReceiverClass::SensorType::type3
						&& (buffer[0] & 0b11110000) ^ 0b10010000) ) {
					memset((void*)&tmpData, 0, sizeof(tmpData));
				}else {
					tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
					tmpData.abid[0] = buffer[1] & 0b11110011 | (lpbuffer->type << 2);
					tmpData.temperature = buffer[2];
					tmpData.temperature = (tmpData.temperature << 4) | (buffer[3] >> 4); // 00001100|1011
					tmpData.temperature += 500;

					tmpData.humidity = ((buffer[3] << 4) | (buffer[4] >> 4)) & 0x7f;
					if (lpbuffer->type == TH433ReceiverClass::SensorType::type1) {
						tmpData.humidity |= (buffer[1] << 4) & 0b10000000; // battery
					}
					else {
						tmpData.humidity |= (((buffer[1] << 4) ^ 0b10000000) & 0b10000000);// ^ 0b10000000; // battery
						//Serial.println((((buffer[1] << 4) ^ 0b10000000) & 0b10000000));
					}
				}

			}break;
			case TH433ReceiverClass::SensorType::type2: {
				uint8_t sum = buffer[0]; sum += buffer[1]; sum += buffer[2]; sum += buffer[3];
				if (sum == buffer[4]) {
					tmpData.abid[1] = buffer[0]; // байты хранятся в обратном порядке
					tmpData.abid[0] = buffer[1] & 0b00110000 | (lpbuffer->type << 2);
					tmpData.temperature = buffer[1] & 0b1111;
					tmpData.temperature = (tmpData.temperature << 8) | (buffer[2]); // 00001100|1011
					tmpData.temperature = (tmpData.temperature - 320) * 5 / 9;
					tmpData.humidity = buffer[3] & 0x7f;
					tmpData.humidity |= (~buffer[1]) & 0b10000000; // battery
				}else{
					memset((void*)&tmpData, 0, sizeof(tmpData));
				}
			}break;
		}

		for (uint8_t i = 0; i < sizeof(tmpData); i++) {
			Serial.print(toBin(((uint8_t*)&tmpData)[i])); Serial.print(' ');
		}
		Serial.print(tmpData.id, HEX); Serial.print(' ');
		Serial.print(tmpData.temperature - 500); Serial.print(' ');
		Serial.print(tmpData.humidity & 0b1111111); Serial.print(' ');
		Serial.print(tmpData.humidity >> 7); Serial.print(' ');
		Serial.println();

		if (tmpData.id) {
			uint8_t oldestSensor = 0, maxTime = 0, pos = 0;

			for (; pos < countSensors; pos++) {
				i2c_packet& currentPacket = i2c_data[pos];
				if (currentPacket.id == tmpData.id)
					break;
				if (maxTime < currentPacket.timeLabel) {
					oldestSensor = pos;
					maxTime = currentPacket.timeLabel;
				}
			}
			if (pos == countSensors)
				countSensors++;
			else if (pos == Settings::maxSensors)
				pos = oldestSensor;
			i2c_data[pos] = tmpData;
		}

	}

	static unsigned long prevMillis = 0;

	while (millis() - prevMillis >= 1000) {
		prevMillis += 1000;

		for (uint8_t i = 0; i < countSensors; i++) {
			i2c_packet& currentPacket = i2c_data[i];
			currentPacket.timeLabel++;
			if (!currentPacket.timeLabel) { // старые данные, удалим
				countSensors--;
				for (uint8_t j = i; j < countSensors; j++)
					i2c_data[j] = i2c_data[j + 1];
				if (i < currentSensor)
					currentSensor--;
			}
			else {
				currentPacket.calc_crc(i);

				/*for (uint8_t i = 0; i < sizeof(i2c_packet); i++) {
					Serial.print(toBin(((uint8_t*)&currentPacket)[i]));
					Serial.print(' ');
				}
				Serial.println();*/
			}
		}
	}

}




uint8_t crc8(const uint8_t * data, const uint8_t size)
{
	static uint8_t crc_array[256] =
	{
		0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
		0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
		0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
		0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
		0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
		0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
		0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
		0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
		0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
		0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
		0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
		0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
		0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
		0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
		0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
		0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
		0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
		0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
		0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
		0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
		0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
		0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
		0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
		0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
		0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
		0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
		0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
		0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
		0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
		0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
		0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
		0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
	};

	unsigned char crc = 0;
	for (unsigned int i = 0; i < size; ++i)
	{
		crc = crc_array[data[i] ^ crc];
	}
	return crc;
}


void PrintData(const TH433ReceiverClass::packet_data& buffer) {
	for (uint8_t i = 0; i < TH433ReceiverClass::packet_len_bytes; i++) {
		Serial.print(toBin(buffer[i]));
		Serial.print(' ');
	}
	Serial.print(buffer.type);
	Serial.print(' ');
	Serial.println(millis());
}
const char * toBin(uint8_t x) {
	static char res[9] = "00000000";
	for (int8_t i = 0; i < 8; i++) {
		bitWrite(res[7 - i], 0, bitRead(x, i));
	}
	return res;
}

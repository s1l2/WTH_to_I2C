// (c) valerivp@gmail.com

#include "CyberLib.h"

#include "treceiver.h"


TH433ReceiverClass TH433Receiver;
TH433ReceiverClass::packet_data TH433ReceiverClass::ring_buf[];
uint8_t TH433ReceiverClass::posPush = 1;
uint8_t TH433ReceiverClass::posPop = 0;

void TH433ReceiverClass::begin()
{
	D8_In; //Настраиваем пин D8 на вход - пин данных приемника
	D13_Out; //Настраиваем пин D13 на выход - LED, индикация

	D9_In;
	D6_In;

	StartTimer1(timer_interrupt, 100); // 10kHz

}

inline void TH433ReceiverClass::check_data_treceiver_onL_type0(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 36;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type0 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 4, 6)) {
		if (CHECK_IN_RANGE(countL, 35, 42)) { // 9.1ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					D13_High;
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}
				else {
					nEqPackets = false;
					D13_Low;
				}
			}
		}
		else if (CHECK_IN_RANGE(countL, 7, 11)) {  // 1.9ms
			bit = 0;
		}else if (CHECK_IN_RANGE(countL, 17, 22)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}
		else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}

inline void TH433ReceiverClass::check_data_treceiver_onL_type1(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 36;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type1 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 4, 6)) {
		if (CHECK_IN_RANGE(countL, 80, 96)) { // 9.1ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					D13_High;
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}else{
					nEqPackets = false;
					D13_Low;
				}
			}
		}else if (CHECK_IN_RANGE(countL, 16, 21)) {  // 1.9ms
			bit = 0;
		}else if (CHECK_IN_RANGE(countL, 36, 42)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}

inline void TH433ReceiverClass::check_data_treceiver_onL_type2(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 40;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = {SensorType::type2};
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;
	if (CHECK_IN_RANGE(countH, 7, 9) && CHECK_IN_RANGE(countL, 6, 7)) { // начало пакета
		bits_count = 0;
	}else if (CHECK_IN_RANGE(countH, 7, 8) && CHECK_IN_RANGE(countL, 8, 10)) { // конец пакета
		if (bits_count == packet_len) { // полная посылка
			last_packet_time = micros(); // отметка времни получения пакета
			nPacketNum++; // увеличим счетчик пакетов
			if (buffers[0] == buffers[1]) {
				// приняли два одинаковых пакета
				D13_High;
				if (!nEqPackets) {
					ring_buf[posPush] = buffers[0];
					++posPush &= Const::ring_mask;
					if (posPush == posPop)
						++posPop &= Const::ring_mask;
				}
				nEqPackets = true;
			}else{
				nEqPackets = false;
				D13_Low;
			}
		}
	}else if (CHECK_IN_RANGE(countH, 4, 5)) {
		bit = 1;
	}else if (CHECK_IN_RANGE(countH, 2, 3)) {
		bit = 0;
	}
	if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
		packet_data& buffer = buffers[nPacketNum % 2];

		if (!bits_count)
			buffer = 0;

		if (bit) // буфер чист - он же очищается перед приемом первого бита
			buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

		bits_count++;
	}
	else {
		if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
			// прошло больше секунды, чистим счетчик пакетов
			nPacketNum = 0;
			nEqPackets = false;

		}
	}

}

inline void TH433ReceiverClass::check_data_treceiver_onL_type3(uint8_t countH, uint8_t countL) {
	const byte	packet_len = 37;
	static byte buffer[2 * Const::packet_len_bytes];
	static packet_data buffers[2] = { SensorType::type3 };
	static byte nPacketNum = 0;
	static bool nEqPackets = false;
	static byte bits_count = 0;

	static unsigned long last_packet_time = 0;
	int8_t bit = -1;

	if (CHECK_IN_RANGE(countH, 5, 7)) {
		if (CHECK_IN_RANGE(countL, 85, 96)) { // 8.9ms
			if (bits_count == packet_len) { // полная посылка
				last_packet_time = micros(); // отметка времни получения пакета
				nPacketNum++; // увеличим счетчик пакетов
				if (buffers[0] == buffers[1]) {
					// приняли два одинаковых пакета
					D13_High;
					if (!nEqPackets) {
						ring_buf[posPush] = buffers[0];
						++posPush &= Const::ring_mask;
						if (posPush == posPop)
							++posPop &= Const::ring_mask;
					}
					nEqPackets = true;
				}
				else {
					nEqPackets = false;
					D13_Low;
				}
			}
		}
		else if (CHECK_IN_RANGE(countL, 16, 21)) {  // 1.9ms
			bit = 0;
		}
		else if (CHECK_IN_RANGE(countL, 36, 42)) { // 3.9ms
			bit = 1;
		}
		if (bit >= 0 && bits_count < packet_len) { // принимаем только биты в количестве длины пакета
			packet_data& buffer = buffers[nPacketNum % 2];

			if (!bits_count) {
				buffer = 0;
			}

			if (bit) // буфер чист - он же очищается перед приемом первого бита
				buffer[bits_count / 8] |= 0x80 >> (bits_count % 8);

			bits_count++;
		}
		else {
			bits_count = 0;
			if (nPacketNum && (micros() - last_packet_time) > 1000000ul) {
				// прошло больше секунды, чистим счетчик пакетов
				nPacketNum = 0;
				nEqPackets = false;
			}
		}
	}
}


void TH433ReceiverClass::timer_interrupt()
{
	static uint8_t countH = 0;
	static uint8_t countL = 0;
	if (D8_Read) {
		countH++;
	}else{
		if (countH >= 2) { // шумоподавление. 
			// если countH > 0 - значит задний фронт
			//D13_High;
			check_data_treceiver_onL_type0(countH, countL);
			check_data_treceiver_onL_type1(countH, countL);
			check_data_treceiver_onL_type2(countH, countL);
			check_data_treceiver_onL_type3(countH, countL);
			countL = 0;
			//D13_Low;
		}
		countH = 0; // сейчас L, поэтому H обнуляем
		countL++; 
	}

}

TH433ReceiverClass::packet_data * TH433ReceiverClass::next_data()
{
	static packet_data buffer;
	packet_data* res = nullptr;

	cli();

	uint8_t newPopPos = (posPop + 1 & Const::ring_mask);
	if (newPopPos != posPush) {
		posPop = newPopPos;
		buffer = ring_buf[posPop];
		res = &buffer;
	}

	sei();

	return res;
}

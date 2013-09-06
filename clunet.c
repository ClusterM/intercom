/* Name: clunet.c
 * Project: CLUNET network driver
 * Author: Alexey Avdyukhin
 * Creation Date: 2013-07-22
 * License: DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 */


#include "defines.h"
#include "clunet_config.h"
#include "bits.h"
#include "clunet.h"
#include <avr/io.h>
#include <avr/interrupt.h>

void (*on_data_received)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size) = 0;
void (*on_data_received_sniff)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size) = 0;

volatile unsigned char clunetSendingState = CLUNET_SENDING_STATE_IDLE;
volatile unsigned short int clunetSendingDataLength;
volatile unsigned char clunetSendingCurrentByte;
volatile unsigned char clunetSendingCurrentBit;
volatile unsigned char clunetReadingState = CLUNET_READING_STATE_IDLE;
volatile unsigned char clunetReadingCurrentByte;
volatile unsigned char clunetReadingCurrentBit;
volatile unsigned char clunetCurrentPrio;

volatile unsigned char clunetReceivingState = 0;
volatile unsigned char clunetReceivingPrio = 0;
volatile unsigned char clunetTimerStart = 0;
volatile unsigned char clunetTimerPeriods = 0;

volatile char dataToSend[CLUNET_SEND_BUFFER_SIZE];
volatile char dataToRead[CLUNET_READ_BUFFER_SIZE];

char check_crc(char* data, unsigned char size)
{
      uint8_t crc=0;
      uint8_t i,j;
      for (i=0; i<size;i++) 
      {
            uint8_t inbyte = data[i];
            for (j=0;j<8;j++) 
            {
                  uint8_t mix = (crc ^ inbyte) & 0x01;
                  crc >>= 1;
                  if (mix) 
                        crc ^= 0x8C;
                  
                  inbyte >>= 1;
            }
      }
      return crc;
}

ISR(CLUNET_TIMER_COMP_VECTOR)
{		
	unsigned char now = CLUNET_TIMER_REG;     // ���������� ������� �����
	
	switch (clunetSendingState)
	{
		case CLUNET_SENDING_STATE_PREINIT: // ����� ��������� ����� ���������
			CLUNET_TIMER_REG_OCR = now + CLUNET_T;	
			clunetSendingState = CLUNET_SENDING_STATE_INIT;  // �������� ��������� ����
			return;	
		case CLUNET_SENDING_STATE_STOP:	// ���������� ��������, �� ���� ��� ���������
			CLUNET_SEND_0;				// ��������� �����
			CLUNET_TIMER_REG_OCR = now + CLUNET_T;
			clunetSendingState = CLUNET_SENDING_STATE_DONE;
			return;
		case CLUNET_SENDING_STATE_DONE:	// ���������� ��������
			CLUNET_DISABLE_TIMER_COMP; // ��������� ������-���������
			clunetSendingState = CLUNET_SENDING_STATE_IDLE; // ������ ����, ��� ���������� ��������
			return;		
	}	

	if (/*!((clunetReadingState == CLUNET_READING_STATE_DATA) && // ���� �� ������ �� [�������� ������
		(clunetCurrentPrio > clunetReceivingPrio) 				// � ��������� ���������� ������ �� ����
		&& (clunetSendingState == CLUNET_SENDING_STATE_INIT))  // � �� �� �������� ������ �������������]		
		&&*/ (!CLUNET_SENDING && CLUNET_READING)) // �� ��� �������� �� ��������. ���� �� ����� �� ������, �� ��� ��� ������
	{
		CLUNET_DISABLE_TIMER_COMP; // ��������� ������-���������
		clunetSendingState = CLUNET_SENDING_STATE_WAITING_LINE; // ��������� � ����� �������� �����
		return;											 // � ��������
	}

	CLUNET_SEND_INVERT;	 // ����� ���������� �������� �������, � ��� ��� �������������
	
	if (!CLUNET_SENDING)        // ���� �� ��������� �����...
	{
		CLUNET_TIMER_REG_OCR = now + CLUNET_T; // �� ������� � ��� ���������� ����� CLUNET_T ������ �������
		return;
	}
	switch (clunetSendingState)
	{
		case CLUNET_SENDING_STATE_INIT: // �������������
			CLUNET_TIMER_REG_OCR = now + CLUNET_INIT_T;	
			clunetSendingState = CLUNET_SENDING_STATE_PRIO1;  // �������� ��������� ����
			return;
		case CLUNET_SENDING_STATE_PRIO1: // ���� �������� ����������, ������� ���
			if ((clunetCurrentPrio-1) & 2) // ���� 1, �� ��� 3T, � ���� 0, �� 1T
				CLUNET_TIMER_REG_OCR = now + CLUNET_1_T;
			else CLUNET_TIMER_REG_OCR = now + CLUNET_0_T;	
			clunetSendingState = CLUNET_SENDING_STATE_PRIO2;
			return;
		case CLUNET_SENDING_STATE_PRIO2: // ���� �������� ����������, ������� ���
			if ((clunetCurrentPrio-1) & 1) // ���� 1, �� ��� 3T, � ���� 0, �� 1T
				CLUNET_TIMER_REG_OCR = now + CLUNET_1_T;
			else CLUNET_TIMER_REG_OCR = now + CLUNET_0_T;	
			clunetSendingState = CLUNET_SENDING_STATE_DATA;
			return;
		case CLUNET_SENDING_STATE_DATA: // ���� �������� ������
			if (dataToSend[clunetSendingCurrentByte] & (1 << clunetSendingCurrentBit)) // ���� 1, �� ��� 3T, � ���� 0, �� 1T
				CLUNET_TIMER_REG_OCR = now + CLUNET_1_T;
			else CLUNET_TIMER_REG_OCR = now + CLUNET_0_T;
			clunetSendingCurrentBit++; // ��������� � ���������� ����
			if (clunetSendingCurrentBit >= 8)
			{
				clunetSendingCurrentBit = 0;
				clunetSendingCurrentByte++;
			}
			if (clunetSendingCurrentByte >= clunetSendingDataLength) // ������ �����������
			{
				clunetSendingState = CLUNET_SENDING_STATE_STOP; // ��������� ����
			}
			return;
	}
}


void clunet_start_send()
{
	CLUNET_SEND_0;
	if (clunetSendingState != CLUNET_SENDING_STATE_PREINIT) // ���� �� ����� �����...
		clunetSendingState = CLUNET_SENDING_STATE_INIT; // ������������� ��������
	clunetSendingCurrentByte = clunetSendingCurrentBit = 0; // �������� �������
	CLUNET_TIMER_REG_OCR = CLUNET_TIMER_REG+CLUNET_T;			// ��������� ������, ������ ������-�� ���������� ����������� �����
	CLUNET_ENABLE_TIMER_COMP;			// �������� ���������� �������-���������
}

void clunet_send(unsigned char address, unsigned char prio, unsigned char command, char* data, unsigned char size)
{
	if (CLUNET_OFFSET_DATA+size+1 > CLUNET_SEND_BUFFER_SIZE) return; // �� ������� ������
	CLUNET_DISABLE_TIMER_COMP;CLUNET_SEND_0; // ��������� ������� ��������, ���� ���� �����
	// ��������� ����������
	if (clunetSendingState != CLUNET_SENDING_STATE_PREINIT)
		clunetSendingState = CLUNET_SENDING_STATE_IDLE;
	clunetCurrentPrio = prio;
	dataToSend[CLUNET_OFFSET_SRC_ADDRESS] = CLUNET_DEVICE_ID;
	dataToSend[CLUNET_OFFSET_DST_ADDRESS] = address;
	dataToSend[CLUNET_OFFSET_COMMAND] = command;
	dataToSend[CLUNET_OFFSET_SIZE] = size;
	unsigned char i;
	for (i = 0; i < size; i++)	
		dataToSend[CLUNET_OFFSET_DATA+i] = data[i];
	dataToSend[CLUNET_OFFSET_DATA+size] = check_crc((char*)dataToSend, CLUNET_OFFSET_DATA+size);
	clunetSendingDataLength = CLUNET_OFFSET_DATA + size + 1;
	if (
		(clunetReadingState == CLUNET_READING_STATE_IDLE) // ���� �� ������ �� �������� � ������ ������, �� �������� �����		
//		|| ((clunetReadingState == CLUNET_READING_STATE_DATA) && (prio > clunetReceivingPrio)) // ���� ���� ��������, �� � ����� ������ �����������
		)
		clunet_start_send(); // ��������� �������� �����
	else clunetSendingState = CLUNET_SENDING_STATE_WAITING_LINE; // ����� ��� �����
}

inline void clunet_data_received(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size)
{	
	if (on_data_received_sniff)
		(*on_data_received_sniff)(src_address, dst_address, command, data, size);

	if (src_address == CLUNET_DEVICE_ID) return; // ���������� ��������� �� ������ ����!

	if ((dst_address != CLUNET_DEVICE_ID) &&
		(dst_address != CLUNET_BROADCAST_ADDRESS)) return; // ���������� ��������� �� ��� ���					

	if (command == CLUNET_COMMAND_REBOOT) // ������ �����. � ��, ��������� ���� �� �����
	{
		cli();
		set_bit(WDTCR, WDE);
		while(1);
	}

	if ((clunetSendingState == CLUNET_SENDING_STATE_IDLE) || (clunetCurrentPrio <= CLUNET_PRIORITY_MESSAGE))
	{
		if (command == CLUNET_COMMAND_DISCOVERY) // ����� �� ����� ���������
		{
			char buf[] = CLUNET_DEVICE_NAME;
			int len = 0; while(buf[len]) len++;
			clunetSendingState = CLUNET_SENDING_STATE_PREINIT;
			clunet_send(src_address, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_DISCOVERY_RESPONSE, buf, len);
		}
		else if (command == CLUNET_COMMAND_PING) // ����� �� ����
		{
			clunetSendingState = CLUNET_SENDING_STATE_PREINIT;
			clunet_send(src_address, CLUNET_PRIORITY_COMMAND, CLUNET_COMMAND_PING_REPLY, data, size);
		}
	}
	
	if (on_data_received)
		(*on_data_received)(src_address, dst_address, command, data, size);
		
	if ((clunetSendingState == CLUNET_SENDING_STATE_WAITING_LINE) && !CLUNET_READING) // ���� ���� ������������ ������, ���, ����� ������������
	{
		clunetSendingState = CLUNET_SENDING_STATE_PREINIT;
		clunet_start_send();		
	}
}

ISR(CLUNET_TIMER_OVF_VECTOR)
{		
	if (clunetTimerPeriods < 3)
		clunetTimerPeriods++;
	else // ������� ����� ��� �������, ����� � ���������� ����������
	{
		CLUNET_SEND_0; 					// � ����� �� ������ ����� ������? ���� �� ���� �� ������...
		clunetReadingState = CLUNET_READING_STATE_IDLE;
		if ((clunetSendingState == CLUNET_SENDING_STATE_IDLE) && (!CLUNET_READING))
			CLUNET_DISABLE_TIMER_OVF;
		if ((clunetSendingState == CLUNET_SENDING_STATE_WAITING_LINE) && (!CLUNET_READING)) // ���� ���� ������������ ������, ���, ����� ������������
			clunet_start_send();
	}
}


ISR(CLUNET_INT_VECTOR)
{
	unsigned char time = (unsigned char)((CLUNET_TIMER_REG-clunetTimerStart) & 0xFF);
	if (!CLUNET_READING) // ����� ���������
	{
		CLUNET_ENABLE_TIMER_OVF;
		if (time >= (CLUNET_INIT_T+CLUNET_1_T)/2) // ���� ���-�� ����� ��� �����, ��� �������������
		{
			clunetReadingState = CLUNET_READING_STATE_PRIO1;
		}
		else switch (clunetReadingState) // � ���� �� �����, �� ������� �� ����
		{
			case CLUNET_READING_STATE_PRIO1:    // ��������� ����������, ������� �� �� �����
				if (time > (CLUNET_0_T+CLUNET_1_T)/2)
					clunetReceivingPrio = 3;
					else clunetReceivingPrio = 1;
				clunetReadingState = CLUNET_READING_STATE_PRIO2;
				break;
			case CLUNET_READING_STATE_PRIO2:	 // ��������� ����������, ������� �� �� �����
				if (time > (CLUNET_0_T+CLUNET_1_T)/2)
					clunetReceivingPrio++;
				clunetReadingState = CLUNET_READING_STATE_DATA;
				clunetReadingCurrentByte = 0;
				clunetReadingCurrentBit = 0;
				dataToRead[0] = 0;
				break;
			case CLUNET_READING_STATE_DATA:    // ������ ���� ������
				if (time > (CLUNET_0_T+CLUNET_1_T)/2)
					dataToRead[clunetReadingCurrentByte] |= (1 << clunetReadingCurrentBit);
				clunetReadingCurrentBit++;
				if (clunetReadingCurrentBit >= 8)  // ��������� � ���������� �����
				{
					clunetReadingCurrentByte++;
					clunetReadingCurrentBit = 0;
					if (clunetReadingCurrentByte < CLUNET_READ_BUFFER_SIZE)
						dataToRead[clunetReadingCurrentByte] = 0;
					else // ���� ����� ����������
					{
						clunetReadingState = CLUNET_READING_STATE_IDLE;
						return;
					}
				}				
				if ((clunetReadingCurrentByte > CLUNET_OFFSET_SIZE) && (clunetReadingCurrentByte > dataToRead[CLUNET_OFFSET_SIZE]+CLUNET_OFFSET_DATA))
				{
					// �������� ������ ���������, ���!
					clunetReadingState = CLUNET_READING_STATE_IDLE;
					char crc = check_crc((char*)dataToRead,clunetReadingCurrentByte); // ��������� CRC
					if (crc == 0)
						clunet_data_received(dataToRead[CLUNET_OFFSET_SRC_ADDRESS], dataToRead[CLUNET_OFFSET_DST_ADDRESS], dataToRead[CLUNET_OFFSET_COMMAND], (char*)(dataToRead+CLUNET_OFFSET_DATA), dataToRead[CLUNET_OFFSET_SIZE]);
				}
				break;
		}
	}	
	else 
	{
		clunetTimerStart = CLUNET_TIMER_REG;
		clunetTimerPeriods = 0;
		CLUNET_ENABLE_TIMER_OVF;
	}
}

void clunet_init()
{
	sei();
	CLUNET_SEND_INIT;
	CLUNET_READ_INIT;
	CLUNET_TIMER_INIT;
	CLUNET_INIT_INT;
	CLUNET_ENABLE_INT;
	char reset_source = MCUCSR;
	clunet_send(CLUNET_BROADCAST_ADDRESS, CLUNET_PRIORITY_MESSAGE,	CLUNET_COMMAND_BOOT_COMPLETED, &reset_source, 1);
	MCUCSR = 0;
}

int clunet_ready_to_send() // ���������� 0, ���� ����� � ��������, ����� ��������� ������� ������
{
	if (clunetSendingState == CLUNET_SENDING_STATE_IDLE) return 0;
	return clunetCurrentPrio;
}

void clunet_set_on_data_received(void (*f)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size))
{	
	on_data_received = f;
}

void clunet_set_on_data_received_sniff(void (*f)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size))
{	
	on_data_received_sniff = f;
}


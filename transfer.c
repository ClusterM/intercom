#include "defines.h"
#include "transfer.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "clunet.h"
#include "ff.h"
#include "diskio.h"

volatile char transfer_active = 0;
volatile unsigned char transfer_address = 0;
volatile unsigned long int transfer_file_number = 0;
volatile unsigned long int transfer_file_position = 0;
volatile int transfer_timeout = 0;
volatile int transfer_repeats = 0;

// ������� �������� ������, �������� ������������
void transfer_data()
{
	if (!transfer_active) return; // ���� ��� �������� ������, ��� ��� ������ ������
	LED_GREEN_ON;
	LED_RED_ON;
	if (transfer_timeout == 0) // ���� �� � ��������� �������� ������
	{
		unsigned char buffer[4+4+TRANSFER_PACKET_SIZE];
		*((unsigned long int*)buffer) = transfer_file_number; // ����� �����
		*((unsigned long int*)&buffer[4]) = transfer_file_position; // �������...
		char filename[16];
		sprintf(filename, "/%08lu.wav", transfer_file_number); // ��������� ��� �����
		UINT br = 0; // ������� ��������� ����...
		FIL file;
		if (f_open(&file, filename, FA_READ) == 0)
		{
			f_lseek(&file, transfer_file_position);
			f_read(&file, &buffer[8], TRANSFER_PACKET_SIZE, &br);
			f_close(&file);
		}
		clunet_send(transfer_address, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_INTERCOM_RECORD_DATA, (char*)&buffer, 8+br); // ����� �����
	}
	transfer_timeout++; // ������� �������...
	if (transfer_timeout >= TRANSFER_TIMEOUT) // � ���� ����� ��� ������
	{ 
		transfer_repeats++; // �������� ���������
		transfer_timeout = 0; 
		if (transfer_repeats >= 3) transfer_active = 0; // �� ����� ��� �������� �������� ��������
	}
	LED_RED_OFF;
	_delay_ms(TRANSFER_PAUSE); // ������������� ������ ���
	LED_GREEN_OFF;
}

// �������� ��� ������ �������� ������
void transfer_start(unsigned long int number, unsigned char address)
{
	transfer_file_number = number;
	transfer_file_position = 
		transfer_timeout =
		transfer_repeats = 0;
	transfer_address = address;
	transfer_active = 1;
}

// ������������� ����� ������
void transfer_ack()
{
	// ���� ������ ����������, �������� �� ����� ������
	transfer_file_position += TRANSFER_PACKET_SIZE;
		transfer_timeout =
		transfer_repeats = 0;
}

// ����������� ��������
void transfer_stop()
{
	transfer_active = 0;
}


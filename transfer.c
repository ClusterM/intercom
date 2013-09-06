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

// Функция передачи файлов, вызывать периодически
void transfer_data()
{
	if (!transfer_active) return; // Если нет передачи данных, нам тут нечего делать
	LED_GREEN_ON;
	LED_RED_ON;
	if (transfer_timeout == 0) // Если не в состоянии ожидания ответа
	{
		unsigned char buffer[4+4+TRANSFER_PACKET_SIZE];
		*((unsigned long int*)buffer) = transfer_file_number; // Номер файла
		*((unsigned long int*)&buffer[4]) = transfer_file_position; // Позиция...
		char filename[16];
		sprintf(filename, "/%08lu.wav", transfer_file_number); // Формируем имя файла
		UINT br = 0; // Сколько прочитано байт...
		FIL file;
		if (f_open(&file, filename, FA_READ) == 0)
		{
			f_lseek(&file, transfer_file_position);
			f_read(&file, &buffer[8], TRANSFER_PACKET_SIZE, &br);
			f_close(&file);
		}
		clunet_send(transfer_address, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_INTERCOM_RECORD_DATA, (char*)&buffer, 8+br); // Кусок файла
	}
	transfer_timeout++; // Считаем таймаут...
	if (transfer_timeout >= TRANSFER_TIMEOUT) // И если долго нет ответа
	{ 
		transfer_repeats++; // Пытаемся повторить
		transfer_timeout = 0; 
		if (transfer_repeats >= 3) transfer_active = 0; // Но после трёх повторов отменяем передачу
	}
	LED_RED_OFF;
	_delay_ms(TRANSFER_PAUSE); // Задерживаемся каждый раз
	LED_GREEN_OFF;
}

// Вызывать для начала передачи файлов
void transfer_start(unsigned long int number, unsigned char address)
{
	transfer_file_number = number;
	transfer_file_position = 
		transfer_timeout =
		transfer_repeats = 0;
	transfer_address = address;
	transfer_active = 1;
}

// Подтверждение приёма пакета
void transfer_ack()
{
	// Приём пакета подтверждём, движемся по файлу дальше
	transfer_file_position += TRANSFER_PACKET_SIZE;
		transfer_timeout =
		transfer_repeats = 0;
}

// Прекращение передачи
void transfer_stop()
{
	transfer_active = 0;
}


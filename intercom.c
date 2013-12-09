#include "defines.h"
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "clunet.h"
#include "ff.h"
#include "diskio.h"
#include "sound.h"
#include "rtc.h"
#include "transfer.h"


volatile unsigned char mode_current = 1; // Текущий режим
volatile unsigned char mode_temp = 0;    // Режим на один раз
volatile unsigned long int record_num = 0;
char buffer[32];

char STARTED_WAV[] PROGMEM = "/system/started.wav";
char AUTOLONG_WAV[] PROGMEM = "/system/automid.wav";
char AUTOFAST_WAV[] PROGMEM = "/system/autofast.wav";
char SAVED_WAV[] PROGMEM = "/system/saved.wav";
char OPENME_WAV[] PROGMEM = "/system/openme.wav";
char OPEN_WAV[] PROGMEM = "/system/open.wav";
char WAITLONG_WAV[] PROGMEM = "/system/waitlong.wav";
char EMERGENCY_OPEN_WAV[] PROGMEM = "/system/emeropen.wav";
char MODE_WAV[] PROGMEM = "/system/mode.wav";
char MODETEMP_WAV[] PROGMEM = "/system/modetemp.wav";
char MAINMENU_WAV[] PROGMEM = "/system/mainmenu.wav";
char MODELIST_WAV[] PROGMEM = "/system/modelist.wav";
char MODE0_WAV[] PROGMEM = "/system/mode0.wav";
char MODE1_WAV[] PROGMEM = "/system/mode1.wav";
char MODE2_WAV[] PROGMEM = "/system/mode2.wav";
char MODE3_WAV[] PROGMEM = "/system/mode3.wav";
char MODE4_WAV[] PROGMEM = "/system/mode4.wav";
char MODE5_WAV[] PROGMEM = "/system/mode5.wav";
char MODE9_WAV[] PROGMEM = "/system/zdrasti.wav";
char MODEX_WAV[] PROGMEM = "/system/modex.wav";

void send_current_mode(unsigned char dest)
{
	buffer[0] = mode_current;
	buffer[1] = mode_temp;
	clunet_send(dest, CLUNET_PRIORITY_INFO, CLUNET_COMMAND_INTERCOM_MODE_INFO, (char*)&buffer, 2); // Отправляем в сеть текущий режим
}

void save_mode()
{
	eeprom_write_byte((void*)4, mode_current); // Режим 
	eeprom_write_byte((void*)5, mode_temp); // Временный режим
	send_current_mode(CLUNET_BROADCAST_ADDRESS);
}

void data_received(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size)
{
	if (command == CLUNET_COMMAND_TIME && size >= 6) // Синхронизация времени
	{
		set_time(data[3]+2000, data[4], data[5], data[0],data[1],data[2]);
	}
	else if (command == CLUNET_COMMAND_INTERCOM_MODE_REQUEST) // У нас запрашивают режим
	{
		send_current_mode(src_address);
	}
	else if (command == CLUNET_COMMAND_INTERCOM_MODE_SET) // Установка режима
	{
		if (size >= 1 && (unsigned char)data[0] != 0xFF) mode_current = data[0];
		if (size >= 2 && (unsigned char)data[1] != 0xFF) mode_temp = data[1];
		save_mode();
	}	
	else if (command == CLUNET_COMMAND_INTERCOM_RECORD_REQUEST) // Передача записей
	{
		if (size == 4) transfer_start(*((unsigned long int*)data), src_address); // Начинаем передачу
		else if (size == 1 && data[0] == 1) transfer_ack(); // Подтвержают приём, переходим к следующему куску
		else if (size == 1 && data[0] == 0) transfer_stop(); // Прекращение передачи данных
	}
}

int is_LINE_POWER() // Определяет, что есть стабильный сигнал в линии
{
	int i;	
	for (i = 0; i < 10; i++)
	{
		if (!LINE_POWER) return 0;
		_delay_ms(10);
	}
	return 1;
}

void intercom_bell() // Издаёт звук входящего звонка
{
	int t;
	for (t = 0; t < 9; t++)
	{
		if (t % 3 == 0)
			_delay_ms(200);
		beep(3000, 30);
		beep(2000, 20);
		beep(1000, 10);
	}
}

int answer_play(char* filename) // Отвечает и воспроизводит файл
{
	ANSWER;
	MODE_MYSOUND;
	_delay_ms(1000);
	if (play_wav_pgm(filename) == 0)
	{
		while (sound_read() >= 0)
		{
			if (!LINE_POWER || OFFHOOK) // Сняли трубку, или сигнал исчез
			{
				sound_stop();
				return 1;
			}
		}
		sound_stop();
	}
	return 0;
}

int answer_play_open(char* filename) // Отвечает, воспроизводит файл и открывает дверь
{
	if (answer_play(filename)) return 1; // Сняли трубку, или сигнал исчез
	OPEN;
	return 0;
}

int answer_open_play(char* filename) // Отвечает, открывает дверь и воспроизводит файл
{
	ANSWER;
	OPEN;
	_delay_ms(1000);
	if (answer_play(filename)) return 1; // Сняли трубку, или сигнал исчез
	return 0;
}


int answer_record(char* filename1, char* filename2) // Отвечает, записывает сообщение,
{
	if (answer_play(filename1)) return 1; // Отвечаем, предлагаем оставить сообщение
	beep(3000, 500);	// Биип
	sprintf(buffer, "/%08lu.wav", record_num); // Формируем имя файла
	clunet_send(CLUNET_BROADCAST_ADDRESS, CLUNET_PRIORITY_INFO, CLUNET_COMMAND_INTERCOM_MESSAGE, (char*)&record_num, sizeof(record_num)); // Отправляем в сеть сообщение
	record_num++;
	eeprom_write_dword((void*)0, record_num); // Запоминаем кол-во записей
	if (rec_wav(buffer) == 0) // Пишем сообщение
	{
		int s = 0;
		long int totalSize = 0;
		while (s >= 0 && totalSize < 8000UL*RECORD_MAX_LENGTH)
		{
			s = sound_write();
			totalSize += s;
			if (!LINE_POWER || OFFHOOK) // Сняли трубку, или сигнал исчез
			{
				sound_stop();
				return 1;
			}
		}
		sound_stop();
	}	
	if (play_wav_pgm(filename2) == 0) // Если пациент дождался, благодарим
	{
		while (sound_read() >= 0)
		{
			if (!LINE_POWER || OFFHOOK) // Сняли трубку, или сигнал исчез
			{
				sound_stop();
				return 1;
			}
		}
		sound_stop();
	}
	return 0;
}

void incoming_ring() // Выполняется при любом входящем звонке
{
	LED_RED_ON;	
	unsigned char mode = (mode_temp != 0) ? mode_temp : mode_current; // Текущий режим
	// Сообщение о звонке, указываем текущий режим
	clunet_send(CLUNET_BROADCAST_ADDRESS, CLUNET_PRIORITY_INFO, CLUNET_COMMAND_INTERCOM_RING, (char*)&mode, 1);
	if (mode != 3 && mode != 0xFF) 
		intercom_bell();
	if (mode_temp) // Обнуляем временный режим
	{
		mode_temp = 0;
		save_mode();
	}
	if (!LINE_POWER) // Аварийное откртие двери, если набрали и сразу сбросили
	{
		mode_temp = 0xFF; 
		save_mode();
	}
	if (mode)
	{
		switch (mode)
		{
			case 1: // Автоответчик, долго ждёт ответа, потом записывает
				answer_record(AUTOLONG_WAV, SAVED_WAV);
				break;
			case 2: // Автоответчик, сразу записывает
				answer_record(AUTOFAST_WAV, SAVED_WAV);
				break;
			case 3: // Приветствует хозяина, открывает дверь
				answer_play_open(OPENME_WAV);
				break;
			case 4: // Приветствует гостя, открывает дверь
				answer_play_open(OPEN_WAV);
				break;
			case 5: // Долго ждёт ответа. Сообщение не пишет.
				answer_play(WAITLONG_WAV);
				break;
			case 9: // Здрасти, здрастите. Проходи, братишка, проходи.
				answer_play_open(MODE9_WAV);
				break;				
			case 0xFF:  // Аварийное откртие двери
				answer_play_open(EMERGENCY_OPEN_WAV);
		}
		MODE_NORMAL;
		_delay_ms(200); // На случай, если контакты снятия трубки не успели разомкнуться
		HANGUP;
	}
	while (LINE_POWER); // Ждём пока не пропадёт сигнал 
	LED_RED_OFF;
	while (OFFHOOK); // И пользователь не положит трубку
	_delay_ms(100); // Защита от дребезга контактов
}

int count_disk() // Считает, что набрали на диске
{
	int cnt = 1;
	while (1)
	{
		while (CONTROL);
		_delay_ms(1);
		int t = 0;
		while (!CONTROL)
		{
			_delay_ms(1);
			t++;
			if (t > 200) return cnt;
		}
		cnt++;
	}
}

void say_mode(char istemp)
{
	unsigned char mode = 0;
	if (!istemp)
	{
		mode = mode_current;
		play_wav_auto_pgm(MODE_WAV);
	}
	else 
	{
		mode = mode_temp;
		play_wav_auto_pgm(MODETEMP_WAV);				
	}
	switch (mode)
	{
		case 0:
			play_wav_auto_pgm(MODE0_WAV);
			break;
		case 1:
			play_wav_auto_pgm(MODE1_WAV);
			break;
		case 2:
			play_wav_auto_pgm(MODE2_WAV);
			break;
		case 3:
			play_wav_auto_pgm(MODE3_WAV);
			break;
		case 4:
			play_wav_auto_pgm(MODE4_WAV);
			break;
		case 5:
			play_wav_auto_pgm(MODE5_WAV);
			break;
		case 9:
			play_wav_auto_pgm(MODE9_WAV);
			break;
		default:
			play_wav_auto_pgm(MODEX_WAV);
			break;
	}
}

void select_mode(char istemp) // Выбот режима
{
	say_mode(istemp);
	while (!CONTROL && OFFHOOK) play_wav_auto_pgm(MODELIST_WAV);
	if (!CONTROL) return;
	int cnt = count_disk();
	if (cnt >= 10) cnt = 0;
	if (!istemp)
	{
		mode_current = cnt;
		say_mode(0);
	} else {
		mode_temp = cnt;
		say_mode(1);
	}
	save_mode();
}

void play_record(long unsigned int num)
{
	sprintf(buffer, "/%08lu.wav", num); // Формируем имя файла
	play_wav_auto(buffer);
}

void control_mode() // Режим управления
{
	LED_GREEN_ON;
	while (OFFHOOK)
	{
		if (!CONTROL)
			play_wav_auto_pgm(MAINMENU_WAV);
		while (!CONTROL && OFFHOOK) play_wav_auto_pgm(MAINMENU_WAV);
		if (CONTROL)
		{
			int cnt = count_disk();
			switch (cnt)
			{
				case 1:
					select_mode(0);
					break;
				case 2:
					select_mode(1);
					break;
				case 10:
					break;					
				default:
					play_record(record_num-(cnt-2));
					break;
			}
		}
	}
	LED_GREEN_OFF;
	_delay_ms(100); // Защита от дребезга контактов
}

int main (void)
{
	clunet_init();  
	clunet_set_on_data_received(data_received);	
	time_init();
	sei();
	//eeprom_write_dword((void*)0, 0);
	record_num = eeprom_read_dword((void*)0); // Читаем кол-во записей
	mode_current = eeprom_read_byte((void*)4); // Режим 
	mode_temp = eeprom_read_byte((void*)5); // Временный режим

	disk_initialize(0);	

	unset_bit(DDRA, 3); set_bit(PORTA, 3);	 // Определение сигнала в линии	
	unset_bit(DDRA, 4);	unset_bit(PORTA, 4); // Открывалка двери
	set_bit(DDRA, 5); HANGUP; // Реле снимания трубки
	set_bit(DDRA, 6); MODE_NORMAL; // Реле выбора режима
	unset_bit(DDRG, 0); set_bit(PORTG, 0); // Определение, лежит ли трубка	
	set_bit(DDRD, 6); set_bit(DDRD, 7); // Светодиоды
	unset_bit(DDRA, 7); set_bit(PORTA, 7); // Счётчик оборотов диска

	unset_bit(DDRF, 0); // ADC+
	unset_bit(PORTF, 0);
	unset_bit(DDRF, 1); // ADC-
	unset_bit(PORTF, 1);

	beep(500, 200);
	beep(1500, 200);
	beep(3000, 200);
	_delay_ms(1000);
	if (play_wav_pgm(STARTED_WAV) == 0)
	{
		LED_GREEN_ON;
		while (sound_read() >= 0) ;
		LED_GREEN_OFF;
		sound_stop();
	} else {
		LED_RED_ON;
		beep(3000, 200);
		beep(1500, 200);
		beep(500, 200);
		LED_RED_OFF;
	}
	
	send_current_mode(CLUNET_BROADCAST_ADDRESS);

	while(1)
	{
		if (is_LINE_POWER()) incoming_ring();
		if (OFFHOOK) control_mode();
		transfer_data(); // Передаём данные на досуге.
		//play_wav_auto("/00000007.wav");
	}
}


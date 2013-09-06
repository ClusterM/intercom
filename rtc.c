#include <avr/io.h>
#include <avr/interrupt.h>
#include "rtc.h"
#include "bits.h"

volatile char rtc_ok = 0;
volatile int year;
volatile int month;
volatile int mday;
volatile int hour;
volatile int min;
volatile int sec;

DWORD get_fattime(void)
{
	if (!rtc_ok) return 0;

	return	  ((DWORD)(year - 1980) << 25)
			| ((DWORD)month << 21)
			| ((DWORD)mday << 16)
			| ((DWORD)hour << 11)
			| ((DWORD)min << 5)
			| ((DWORD)sec >> 1);
}

void set_time(int s_year, int s_month, int s_mday, int s_hour, int s_min, int s_sec)
{
	year = s_year;
	month = s_month;
	mday = s_mday;
	hour = s_hour;
	min = s_min;
	sec = s_sec;
	rtc_ok = 1;
}

void time_init()
{
	OCR2 = 0; // Запускаем часы
	set_bit(ASSR, AS0);
	set_bit2(TCCR0, CS22, CS20); // x128, каждую секунду
	set_bit(TIMSK, TOIE0);
}

int days(int month, int year)
{
	switch (month)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:		
			return 31;
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		case 2:
			return (year % 4 == 0) ? 29 : 28;
	}
	return 0;
}

ISR(TIMER0_OVF_vect)
{
	sec++;
	if (sec > 59)
	{
		min++;
		sec = 0;
	}
	if (min > 59)
	{
		hour++;
		min = 0;
	}
	if (hour > 23)
	{
		mday++;
		hour = 0;
	}
	if (mday > days(month, year))
	{
		month++;
		mday = 1;
	}
	if (month > 12)
	{
		year++;
		month = 1;
	}
}


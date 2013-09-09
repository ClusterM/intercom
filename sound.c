#include "defines.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "sound.h"
#include "bits.h"

#define NBSIZE 32
#define FCC(c1,c2,c3,c4)	(((DWORD)c4<<24)+((DWORD)c3<<16)+(c2<<8)+c1)	/* FourCC */

FATFS FATFS_Obj;
FIL file;
BYTE buffer[2048];
volatile UINT playMode;
volatile UINT readPosition = 0;
volatile UINT writePosition = 0;
volatile UINT bufferLeft = 0;
DWORD fileLeft = 0;
char playingSound = 0;
char recordingSound = 0;
volatile DWORD beep_left = 0;
UINT beep_freq = 0;
DWORD recordedBytes = 0;

void sound_init()
{
	/* Configure OC3B/OC3C as audio output (Fast PWM) */
	TCNT3 = 0;
	TCCR3A = _BV(COM3B1) | _BV(COM3C1) | _BV(WGM30);
	TCCR3B = _BV(WGM32) | _BV(CS30);
	OCR3B = 0x80; 
	OCR3C = 0x80;		/* Center level */
	DDRE |= _BV(4) | _BV(5);	/* Attach OC3B/OC3C to I/O pad */
}

void sound_freq_timer(unsigned int freq)
{
	if (!freq)
	{
		TCCR1B = 0;			/* Stop sampling interrupt (Timer1) */
		TIMSK &= ~_BV(OCIE1A);
	} else {
	
		/* Configure TIMER1 as sampling interval timer */
		OCR1A = F_CPU / freq - 1;
		TCNT1 = 0;
		TCCR1A = 0;
		TCCR1B = _BV(WGM12) | _BV(CS10);
		TIMSK |= _BV(OCIE1A);
	}
}

ISR(TIMER1_COMPA_vect)
{
	if (playingSound)
	{
		UINT ri = readPosition;
		BYTE *buff, l, r;
		buff = buffer + ri;

		switch (playMode) {
		case 0:		/* Mono, 8bit */
			if (bufferLeft < 1) return;
			l = r = buff[0];
			bufferLeft -= 1; ri += 1;
			break;
		case 1:		/* Stereo, 8bit */
			if (bufferLeft < 2) return;
			l = buff[0]; r = buff[1];
			bufferLeft -= 2; ri += 2;
			break;
		case 2:		/* Mono, 16bit */
			if (bufferLeft < 2) return;
			l = r = buff[1] + 128;
			bufferLeft -= 2; ri += 2;
			break;
		default:	/* Stereo, 16bit */
			if (bufferLeft < 4) return;
			l = buff[1]; r = buff[3];
			bufferLeft -= 4; ri += 4;
		}
	
		readPosition = ri & (sizeof(buffer) - 1);

		OCR3B = l;
		OCR3C = r;
	} else if (recordingSound)
	{
		UINT wi = writePosition;
		BYTE *buff;
		buff = buffer + wi;
		int value = ADC;
		if (value < 0x200) 
			value += 512;
			else value -= 512;
		value += -384+(RECORD_FIX);		
		if (value < 0) value = 0;
		if (value > 0xFF) value = 0xFF;		
		//if (value >= 0x80-NOICE_LEVEL && value <= 0x80+NOICE_LEVEL) value = 0x80; // Небольшая защита от шумов...
		*buff = value;
		wi++;
		OCR3C = value; // Воспроизведение на встроенный динамик
		writePosition = wi & (sizeof(buffer) - 1);
		bufferLeft++;
		set_bit(ADCSRA, ADSC);
	} else if (beep_left)
	{
		if ((beep_left * beep_freq * 2 / 8000) % 2 == 0)
		{
//			PORTE |= _BV(4);	
//			DDRE |= _BV(4);	
			OCR3B = 0xFF;
			OCR3C = 0xFF;
		} else {
//			PORTE &= ~_BV(4);	
			OCR3B = 0;
			OCR3C = 0;
		}
		beep_left--;
	}
}

void sound_stop(void)
{	
	OCR3B = 0x80;
	OCR3C = 0x80;
	sound_freq_timer(0);

	if (recordingSound)
	{
		UINT bw;
		f_lseek(&file, 0x04);
		DWORD chunkSize = recordedBytes + 36;		
		f_write(&file, &chunkSize, 4, &bw);
		f_lseek(&file, 0x28);
		f_write(&file, &recordedBytes, 4, &bw);
	}

	playingSound = recordingSound = 0;

	f_close(&file);
}

int sound_read()
{
	if (fileLeft || bufferLeft >= 4)
	{
		if (fileLeft && bufferLeft <= sizeof(buffer) / 2)
		{	/* Refill FIFO when it gets half empty */
			UINT br = 0;
			UINT btr = (fileLeft >= sizeof(buffer) / 2) ? sizeof(buffer) / 2 : fileLeft;
			f_read(&file, &buffer[writePosition], btr, &br);
			if (br != btr)
			{
				sound_stop();
				return -1;
			}
			fileLeft -= br;
			writePosition = (writePosition + br) & (sizeof(buffer) - 1);
			cli();
			bufferLeft += br;
			sei();
			return br;
		}
		return 0;
	}
	sound_stop();
	return -1;
}

int sound_write()
{
	if (bufferLeft >= sizeof(buffer) / 2)
	{
		UINT br = 0;
		UINT btr = (bufferLeft >= sizeof(buffer) / 2) ? sizeof(buffer) / 2 : bufferLeft;
		f_write(&file, &buffer[readPosition], btr, &br);
		if (br != btr)
		{
			sound_stop();
			return 0;
		}
		readPosition = (readPosition + br) & (sizeof(buffer) - 1);
		//cli();
		bufferLeft -= br;
		//sei();
		recordedBytes += br;
		return br;
	}
	return 0;
}


int init_wav_play()
{
	UINT br;
	DWORD sz, ssz, offw, wsmp, fsmp, eof;
	char *p, nam[NBSIZE], art[NBSIZE];


	/* Is it a WAV file? */
	if (f_read(&file, buffer, 12, &br) || br != 12) return -1;
	if (LD_DWORD(&buffer[0]) != FCC('R','I','F','F')) return -2;
	if (LD_DWORD(&buffer[8]) != FCC('W','A','V','E')) return -3;
	eof = LD_DWORD(&buffer[4]) + 8;

	/* Analyze the RIFF-WAVE header and get properties */
	nam[0] = art[0] = 0;
	playMode = fsmp = wsmp = offw = fileLeft = 0;
	while (f_tell(&file) < eof) {
		if (f_read(&file, buffer, 8, &br) || br != 8) return -4;
		sz = (LD_DWORD(&buffer[4]) + 1) & ~1;
		switch (LD_DWORD(&buffer[0])) {
		case FCC('f','m','t',' ') :
			if (sz > 1000 || sz < 16 || f_read(&file, buffer, sz, &br) || sz != br) return -5;
			if (LD_WORD(&buffer[0]) != 0x1) return -6;	/* Check if LPCM */
			if (LD_WORD(&buffer[2]) == 2) {	/* Channels (1 or 2) */
				playMode = 1; wsmp = 2;
			} else {
				playMode = 0; wsmp = 1;
			}
			if (LD_WORD(&buffer[14]) == 16) {	/* Resolution (8 or 16) */
				playMode |= 2; wsmp *= 2;
			}
			fsmp = LD_DWORD(&buffer[4]);		/* Sampling rate */
			break;

		case FCC('f','a','c','t') :
			f_lseek(&file, f_tell(&file) + sz);
			break;

		case FCC('d','a','t','a') :
			offw = f_tell(&file);	/* Wave data start offset */
			fileLeft = sz;			/* Wave data length [byte] */
			f_lseek(&file, f_tell(&file) + sz);
			break;

		case FCC('L','I','S','T'):
			sz += f_tell(&file);
			if (f_read(&file, buffer, 4, &br) || br != 4) return -7;
			if (LD_DWORD(buffer) == FCC('I','N','F','O')) {	/* LIST/INFO chunk */
				while (f_tell(&file) < sz) {
					if (f_read(&file, buffer, 8, &br) || br != 8) return -8;
					ssz = (LD_DWORD(&buffer[4]) + 1) & ~1;
					p = 0;
					switch (LD_DWORD(buffer)) {
					case FCC('I','N','A','M'):		/* INAM sub-chunk */
						p = nam; break;
					case FCC('I','A','R','T'):		/* IART sub-cnunk */
						p = art; break;
					}
					if (p && ssz <= NBSIZE) {
						if (f_read(&file, p, ssz, &br) || br != ssz) return -9;
					} else {
						if (f_lseek(&file, f_tell(&file) + ssz)) return -10;
					}
				}
			} else {
				if (f_lseek(&file, sz)) return -11;	/* Skip unknown sub-chunk type */
			}
			break;

		default :	/* Unknown chunk */
			return -12;
		}
	}
	if (!fileLeft || !fsmp) return -13;		/* Check if valid WAV file */
	if (f_lseek(&file, offw)) return -14;	/* Seek to top of wav data */
	if (fsmp < 8000 || fsmp > 44100) return -15;	/* Check fs range */
	
	readPosition = 0; writePosition = 0; bufferLeft = 0;	/* Flush FIFO */
	
	sound_read();

	sound_init();
	sound_freq_timer(fsmp);
	
	return 0;
}

int play_wav(char* filename)
{
	sound_stop();
	f_mount(0, &FATFS_Obj); // Перемонтирование на случай, если SD карта вытаскивалась
	if (f_open(&file, filename, FA_READ) != 0) return -1;
	if (init_wav_play() < 0)
	{
		f_close(&file);
		return -1;
	}
	playingSound = 1;
	return 0;
}

int play_wav_pgm(char* filename)
{
	BYTE* b = buffer;
	while ((*b = pgm_read_byte(filename)) != 0)
	{
		filename++;
		b++;
	}
	return play_wav((char*)buffer);
}

int play_wav_auto(char* filename)
{
	if (play_wav(filename) != 0) return -1;
	while (sound_read() >= 0 && !CONTROL) ;
	sound_stop();
	return 0;
}

int play_wav_auto_pgm(char* filename)
{
	BYTE* b = buffer;
	while ((*b = pgm_read_byte(filename)) != 0)
	{
		filename++;
		b++;
	}
	return play_wav_auto((char*)buffer);
}

int init_wav_rec()
{
	recordedBytes = readPosition = 0; writePosition = 0; bufferLeft = 0;	/* Flush FIFO */
	// Для вывода на встроенный динамик того, что пишем
	TCNT3 = 0;
	TCCR3A = _BV(COM3C1) | _BV(WGM30);
	TCCR3B = _BV(WGM32) | _BV(CS30);
	OCR3C = 0x80;
	DDRE |= _BV(5);	

	// Снимаем напряжение с ноги вывода звука
	DDRE &= ~_BV(4);
	PORTE &= ~_BV(4);
	
	// ADC1 - позитивный, ADC0 - негативный, усиление 10x
	unset_bit(ADCSRA, ADEN);
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX0); /*| (1 << REFS1);*/ 
	//ADCSRB = 0;
	unset_bit(DDRF, 0);
	unset_bit(PORTF, 0);
	unset_bit(DDRF, 1);
	unset_bit(PORTF, 1);
	//set_bit(ADCSRA, ADATE);
	unset_bit3(ADCSRA, ADPS2, ADPS1, ADPS0);
	set_bit(ADCSRA, ADEN);
	set_bit(ADCSRA, ADSC);

	sound_freq_timer(8000);
	
	return 0;
}

int rec_wav(char* filename)
{
	// Заголовок на 8кГц
	BYTE header[] = {0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
					  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x40, 0x1F, 0x00, 0x00, 0x40, 0x1F, 0x00, 0x00,
					  0x01, 0x00, 0x08, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00};
/*	
	// Заголовок на 16кГц
	BYTE header[] = {0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
					  0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x3E, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00,
					  0x01, 0x00, 0x08, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00};
*/					  
	sound_stop();
	f_mount(0, &FATFS_Obj);
	if (f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE) != 0) return -1;
	UINT bw;
	f_write(&file, header, sizeof(header), &bw);
	if (bw != sizeof(header)) return -1;
	if (init_wav_rec() < 0)
	{
		f_close(&file);
		return -1;
	}
	
	recordingSound = 1;
	return 0;
}

void beep(int freq, int length)
{
	beep_left = length * 8;
	beep_freq = freq;

	sound_init();
	sound_freq_timer(8000);

	while (beep_left) ;

	sound_freq_timer(0);
}


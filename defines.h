#define F_CPU 8000000UL

#define LINE_POWER (!((PINA >> 3) & 1))
#define ANSWER set_bit(PORTA, 5)
#define HANGUP unset_bit(PORTA, 5)
#define MODE_NORMAL { unset_bit(PORTA, 6); } // Переключить реле и подтянуть вывод звука к +
//#define MODE_NORMAL { set_bit(PORTA, 6); } // Всегда внутринний звук, работа без трубки
#define MODE_MYSOUND { set_bit(PORTA, 6); } // Вывод звука на вход и переключить реле
#define OFFHOOK (!(PING & 1))
#define CONTROL (PINA>>7)
//#define OPEN {set_bit(DDRA,4); _delay_ms(500); unset_bit(DDRA,4);} // напрямую
#define OPEN {set_bit(PORTA,4); _delay_ms(500); unset_bit(PORTA,4);} // через реле
#define LED_RED_ON set_bit(PORTD, 6)
#define LED_RED_OFF unset_bit(PORTD, 6)
#define LED_GREEN_ON set_bit(PORTD, 7)
#define LED_GREEN_OFF unset_bit(PORTD, 7)

#define RECORD_MAX_LENGTH 5
#define RECORD_FIX -0x0A
#define TRANSFER_PACKET_SIZE 64
#define TRANSFER_TIMEOUT 20
#define TRANSFER_PAUSE 500

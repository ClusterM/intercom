/* Name: clunet.h
 * Project: CLUNET network driver
 * Author: Alexey Avdyukhin
 * Creation Date: 2013-07-23
 * License: DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 */
 
#ifndef __clunet_h_included__
#define __clunet_h_included__

#include "bits.h"
#include "clunet_config.h"

#define CLUNET_SENDING_STATE_IDLE 0
#define CLUNET_SENDING_STATE_INIT 1
#define CLUNET_SENDING_STATE_PRIO1 2
#define CLUNET_SENDING_STATE_PRIO2 3
#define CLUNET_SENDING_STATE_DATA 4
#define CLUNET_SENDING_STATE_WAITING_LINE 6
#define CLUNET_SENDING_STATE_PREINIT 7
#define CLUNET_SENDING_STATE_STOP 8
#define CLUNET_SENDING_STATE_DONE 9

#define CLUNET_READING_STATE_IDLE 0
#define CLUNET_READING_STATE_INIT 1
#define CLUNET_READING_STATE_PRIO1 2
#define CLUNET_READING_STATE_PRIO2 3
#define CLUNET_READING_STATE_HEADER 4
#define CLUNET_READING_STATE_DATA 5

#define CLUNET_OFFSET_SRC_ADDRESS 0
#define CLUNET_OFFSET_DST_ADDRESS 1
#define CLUNET_OFFSET_COMMAND 2
#define CLUNET_OFFSET_SIZE 3
#define CLUNET_OFFSET_DATA 4
#define CLUNET_BROADCAST_ADDRESS 0xFF

#define CLUNET_COMMAND_DISCOVERY 0x00
/* ����� ������ ���������, ���������� ��� */

#define CLUNET_COMMAND_DISCOVERY_RESPONSE 0x01
/* ����� ��������� �� �����, � �������� ��������� - �������� ���������� (�����) */

#define CLUNET_COMMAND_BOOT_CONTROL 0x02
/* ������ � �����������. ������ - ����������.
<-0 - ��������� �������
->1 - ������� � ����� ���������� ��������
<-2 - ������������� ��������, ���� ��� ����� - ������ ��������
->3 ������ ��������, 4 ����� - �����, �� ��������� - ������ (������ ������� ��������)
<-4 ���� �������� �������
->5 ����� �� ������ �������� */

#define CLUNET_COMMAND_REBOOT 0x03
/* ������������� ���������� � ���������. */

#define CLUNET_COMMAND_BOOT_COMPLETED 0x04
/* ���������� ����������� ����� ������������� ����������, �������� �� �������� �������� ����������. �������� - ���������� MCU ��������, ��������� � ������� ������������. */

#define CLUNET_COMMAND_DOOR_INFO 0x05
/*���������� �� �������� �����.
0 - �������
1 - �������
2 - ������� */

#define CLUNET_COMMAND_DEVICE_POWER_INFO_REQUEST 0x06
/* ����������� ���������� � ��������� ������������ */

#define CLUNET_COMMAND_DEVICE_POWER_INFO 0x07
/* ��������� ������������. �������� - ������� ����� ��������� ������������. */

#define CLUNET_COMMAND_DEVICE_POWER_COMMAND 0x08
/* ��������/��������� �����������. �������� - ������� �����. ��� ����� 0xFE - ������� ����, � 0xFF - ���������. */

#define CLUNET_COMMAND_SET_TIMER 0x09
/* ��������� �������. �������� - ���-�� ������ (��� �����) */

#define CLUNET_COMMAND_RC_BUTTON_PRESSED 0x0A
/* ������ ������ �� ������. ������ ���� - ��� ����, ����� - ����� ������.
�� ������ ������ 01 - ����� ���������� �������� (Sony �����?) ����� ���� ������ - 4 �����.
02 - ��������� ������ ��� ��. */

#define CLUNET_COMMAND_RC_BUTTON_PRESSED_RAW 0x0B
/* ���������������� ������ � ������� ������ �� ������ ��� ������������� �������. ���� ������� �� 4 �����. 2 �� ������� - ������������ ������� � 2 - ������������ ��� ���������� � 0.032 ����� ������������. (1/8000000*256 ���) */

#define CLUNET_COMMAND_RC_BUTTON_SEND 0x0C
/* ������������� ������ ������. ������ ������ ���������� CLUNET_COMMAND_RC_BUTTON_PRESSED, ���� � ����� ������������ ���� ������������ ��������� ������ (���-�� �������������� ��������), ��� Sony ��� ~30�� */

#define CLUNET_COMMAND_RC_BUTTON_SEND_RAW 0x0D
/* ������������� ������ ������ �� ������ ����� ������. ������ ������ ���������� CLUNET_COMMAND_RC_BUTTON_PRESSED_RAW. */

#define CLUNET_COMMAND_LIGHT_LEVEL 0x0E
/* �������� �� ������ ���������. �������� - 2 ����� (0x0000 - 0x1FFF, ��� 0x1FFF - 0 ����� ����� ���������� � �����, � 0x0000 - 5 �����). */

#define CLUNET_COMMAND_ONEWIRE_START_SEARCH 0x0F
/* ������ ������ 1-wire ���������, ������ ������. */

#define CLUNET_COMMAND_ONEWIRE_DEVICE_FOUND 0x10
/* �������� � ��������� 1-wire ���������. ������ - 8 ����, ���������� ��� ����������, �������� ����� � CRC. */

#define CLUNET_COMMAND_TEMPERATURE 0x11
/* �������� � �����������. 1 ���� - ��� ����������, 6 - ��������, 2 - ����������� � ������� ���������� (�������� �� ���, ����� ������������!) */

#define CLUNET_COMMAND_TIME 0x12
/* �������� �����. ����� ���� - ����, ������, �������, ��� (�� 1900), ����� (�� 0), ���� (�� 1) */

#define CLUNET_COMMAND_WHEEL 0x13
/* �������� ������� ������ ����, ������ ���� - ID ������, ����� ��� ����� - ������� */

#define CLUNET_COMMAND_VOLTAGE 0x14
/* �������� ���������� �� ���������, ������ ���� - ID ����������, ����� ��� ����� - �������, ��� 0x3FF = 5.12 */

#define CLUNET_COMMAND_MOTION 0x15
/* ��������, ��� � ��������� ���� ��������, ������ ���� - ID �������/������ */

#define CLUNET_COMMAND_INTERCOM_RING 0x16
/* ������ � ������� */

#define CLUNET_COMMAND_INTERCOM_MESSAGE 0x17
/* ����� ��������� �� ������������� ��������, � ������ 4 ����� - ����� ��������� */

#define CLUNET_COMMAND_INTERCOM_MODE_REQUEST 0x18
/* ����������� ����� ������ �������� */

#define CLUNET_COMMAND_INTERCOM_MODE_INFO 0x19
/* �������� ����� ������ ��������, ������ ���� - ���������� �����, ������ - ��������� */

#define CLUNET_COMMAND_INTERCOM_MODE_SET 0x1A
/* ����� ����� ������ ��������, ������ ���� - ���������� ����� (��� 0xFF, ����� �� �������), ������ - ��������� (�����������) */

#define CLUNET_COMMAND_INTERCOM_RECORD_REQUEST 0x1B
/* ����������� ������ � ��������, ������������ �������� ��� ��������� ��������
 ���� 4 �����, �� ��� ����� ������������� ������
 ���� 1 ����, �� 1 � ������ ������������� ��������� ������, 0 - ���������� �������� */
 
#define CLUNET_COMMAND_INTERCOM_RECORD_DATA 0x1C
/* ������� ����� ������ � �������������. ������ 4 ����� - ����� ������, ����� 4 ����� - �������� �� ������ �����, �� ����� - ������ �� ����� */

#define CLUNET_COMMAND_PING 0xFE
/* ����, �� ��� ������� ���������� ������ �������� ��������� ��������, ��������� ���� ����� */

#define CLUNET_COMMAND_PING_REPLY 0xFF
/* ����� �� ����, � ������ ��, ��� ���� �������� � ���������� ������� */

#define CLUNET_PRIORITY_NOTICE 1
/* ��������� ������ 1 - �������� �����������, ������� ������ ����� ���� �������� ��� ����������� */

#define CLUNET_PRIORITY_INFO 2
/* ��������� ������ 2 - �����-�� ����������, �� ����� ������ */

#define CLUNET_PRIORITY_MESSAGE 3
/* ��������� ������ 3 - ��������� � �����-�� ������ ����������� */

#define CLUNET_PRIORITY_COMMAND 4
/* ��������� ������ 4 - �������, �� ������� ����� ����� ������������� */

#define CLUNET_T ((F_CPU / CLUNET_TIMER_PRESCALER) / 15625)
#if CLUNET_T < 8
#  error Timer frequency is too small, increase CPU frequency or decrease timer prescaler
#endif
#if CLUNET_T > 24
#  error Timer frequency is too big, decrease CPU frequency or increase timer prescaler
#endif

#define CLUNET_0_T (CLUNET_T)
#define CLUNET_1_T (3*CLUNET_T)
#define CLUNET_INIT_T (10*CLUNET_T)

#define CLUNET_CONCAT(a, b)            a ## b
#define CLUNET_OUTPORT(name)           CLUNET_CONCAT(PORT, name)
#define CLUNET_INPORT(name)            CLUNET_CONCAT(PIN, name)
#define CLUNET_DDRPORT(name)           CLUNET_CONCAT(DDR, name)

#ifndef CLUNET_WRITE_TRANSISTOR
#  define CLUNET_SEND_1 CLUNET_DDRPORT(CLUNET_WRITE_PORT) |= (1 << CLUNET_WRITE_PIN)
#  define CLUNET_SEND_0 CLUNET_DDRPORT(CLUNET_WRITE_PORT) &= ~(1 << CLUNET_WRITE_PIN)
#  define CLUNET_SENDING (CLUNET_DDRPORT(CLUNET_WRITE_PORT) & (1 << CLUNET_WRITE_PIN))
#  define CLUNET_SEND_INVERT CLUNET_DDRPORT(CLUNET_WRITE_PORT) ^= (1 << CLUNET_WRITE_PIN)
#  define CLUNET_SEND_INIT { CLUNET_OUTPORT(CLUNET_WRITE_PORT) &= ~(1 << CLUNET_WRITE_PIN); CLUNET_SEND_0; }
#else
#  define CLUNET_SEND_1 CLUNET_OUTPORT(CLUNET_WRITE_PORT) |= (1 << CLUNET_WRITE_PIN)
#  define CLUNET_SEND_0 CLUNET_OUTPORT(CLUNET_WRITE_PORT) &= ~(1 << CLUNET_WRITE_PIN)
#  define CLUNET_SENDING (CLUNET_OUTPORT(CLUNET_WRITE_PORT) & (1 << CLUNET_WRITE_PIN))
#  define CLUNET_SEND_INVERT CLUNET_OUTPORT(CLUNET_WRITE_PORT) ^= (1 << CLUNET_WRITE_PIN)
#  define CLUNET_SEND_INIT { CLUNET_DDRPORT(CLUNET_WRITE_PORT) |= (1 << CLUNET_WRITE_PIN); CLUNET_SEND_0; }
#endif

#define CLUNET_READ_INIT { CLUNET_DDRPORT(CLUNET_READ_PORT) &= ~(1 << CLUNET_READ_PIN); CLUNET_OUTPORT(CLUNET_READ_PORT) |= (1 << CLUNET_READ_PIN); }
#define CLUNET_READING (!(CLUNET_INPORT(CLUNET_READ_PORT) & (1 << CLUNET_READ_PIN)))

#ifndef CLUNET_SEND_BUFFER_SIZE
#  error CLUNET_SEND_BUFFER_SIZE is not defined
#endif
#ifndef CLUNET_READ_BUFFER_SIZE
#  error CLUNET_READ_BUFFER_SIZE is not defined
#endif
#if CLUNET_SEND_BUFFER_SIZE > 255
#  error CLUNET_SEND_BUFFER_SIZE must be <= 255
#endif
#if CLUNET_READ_BUFFER_SIZE > 255
#  error CLUNET_READ_BUFFER_SIZE must be <= 255
#endif

// �������������
void clunet_init();

// �������� ������
void clunet_send(unsigned char address, unsigned char prio, unsigned char command, char* data, unsigned char size);

// ���������� 0, ���� ����� � ��������, ����� ��������� ������� ������
int clunet_ready_to_send();

// ��������� �������, ������� ���������� ��� ��������� �������
// ��� - �������� ������, ������� ���������� ���
void clunet_set_on_data_received(void (*f)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size));

// � ��� - ��������� ���, ������� ����� �� ����, ������� ����
void clunet_set_on_data_received_sniff(void (*f)(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size));

char check_crc(char* data, unsigned char size);

#endif

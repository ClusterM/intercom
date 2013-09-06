#ifndef RTC_DEFINED
#define RTC_DEFINED

#include "integer.h"

DWORD get_fattime (void);
void set_time(int s_year, int s_month, int s_mday, int s_hour, int s_min, int s_sec);
void time_init();

#endif

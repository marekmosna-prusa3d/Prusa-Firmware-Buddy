#ifndef RTC_TIME_HEADER_H_
#define RTC_TIME_HEADER_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern time_t get_system_time(void);
extern void set_system_time(uint32_t sec, int8_t last_timezone);
extern void add_time_to_timestamp(int32_t secs_to_add, struct tm *timestamp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

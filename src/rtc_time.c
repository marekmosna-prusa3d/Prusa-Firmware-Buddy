#include "rtc_time.h"

#include "stm32f4xx_hal.h"
#include "main.h"

#include <stdint.h>

static uint32_t time_init = 0;

time_t get_system_time(void) {

    if (time_init) {
        RTC_TimeTypeDef currTime;
        RTC_DateTypeDef currDate;
        HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &currDate, RTC_FORMAT_BIN);
        time_t secs;
        struct tm system_time;
        system_time.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
        system_time.tm_hour = currTime.Hours;
        system_time.tm_min = currTime.Minutes;
        system_time.tm_sec = currTime.Seconds;
        system_time.tm_mday = currDate.Date;
        system_time.tm_mon = currDate.Month;
        system_time.tm_year = currDate.Year;
        system_time.tm_wday = currDate.WeekDay;
        secs = mktime(&system_time);
        return secs;
    } else {
        return 0;
    }
}

void set_system_time(uint32_t sec, int8_t last_timezone) {
    struct tm current_time_val;
    time_t current_time = 0;
    RTC_TimeTypeDef currTime;
    RTC_DateTypeDef currDate;

#ifdef WUI
    ETH_config_t config = {};
    config.var_mask = ETHVAR_MSK(ETHVAR_TIMEZONE);
    load_eth_params(&config);

    int8_t diff = config.timezone - last_timezone;
    current_time = (time_t)sec + (diff * 3600);
#else
    current_time = (time_t)sec;

#endif
    localtime_r(&current_time, &current_time_val);

    currTime.Seconds = current_time_val.tm_sec;
    currTime.Minutes = current_time_val.tm_min;
    currTime.Hours = current_time_val.tm_hour;
    currDate.Date = current_time_val.tm_mday;
    currDate.Month = current_time_val.tm_mon;
    currDate.Year = current_time_val.tm_year;
    currDate.WeekDay = current_time_val.tm_wday;

    HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &currDate, RTC_FORMAT_BIN);

    time_init = true;
}

void add_time_to_timestamp(int32_t secs_to_add, struct tm *timestamp) {

    if (secs_to_add == 0) {
        return;
    }
    time_t secs_from_epoch_start = mktime(timestamp);
    time_t current_time = secs_from_epoch_start + secs_to_add;
    localtime_r(&current_time, timestamp);
}

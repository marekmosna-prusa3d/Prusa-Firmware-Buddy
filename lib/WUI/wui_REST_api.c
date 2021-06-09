/*
 * wui_REST_api.c
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_REST_api.h"
#include "wui_api.h"
#include "wui.h"
#include "filament.h" //get_selected_filament_name
#include <string.h>
#include "wui_vars.h"
#include "eeprom.h"
#include "marlin_vars.h"

#define BDY_WUI_API_BUFFER_SIZE 512

// for data exchange between wui thread and HTTP thread
static wui_vars_t wui_vars_copy;

// wui_vars.temp_nozzle = wui_marlin_vars->temp_nozzle;
// wui_vars.temp_bed = wui_marlin_vars->temp_bed;
// wui_vars.target_nozzle = wui_marlin_vars->target_nozzle;
// wui_vars.target_bed = wui_marlin_vars->target_bed;
// wui_vars.fan_speed = wui_marlin_vars->fan_speed;
// wui_vars.print_speed = wui_marlin_vars->print_speed;
// wui_vars.flow_factor = wui_marlin_vars->flow_factor;
// wui_vars.print_dur = wui_marlin_vars->print_duration;
// wui_vars.sd_precent_done = wui_marlin_vars->sd_percent_done;
// wui_vars.sd_printing = wui_marlin_vars->sd_printing;
// wui_vars.time_to_end = wui_marlin_vars->time_to_end;
// wui_vars.print_state = wui_marlin_vars->print_state;
// if (marlin_change_clr(MARLIN_VAR_FILENAME)) {
//     strlcpy(wui_vars.gcode_name, wui_marlin_vars->media_LFN, FILE_NAME_MAX_LEN);
// }

void get_printer(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    const char *filament_material = get_selected_filament_name();
    uint32_t operational = 1;
    uint32_t paused = 0;
    uint32_t printing = 0;
    uint32_t cancelling = 0;
    uint32_t pausing = 0;
    uint32_t sd_ready = 1;
    uint32_t error = 0;
    uint32_t ready = 1;
    uint32_t closed_on_error = 0;
    uint32_t busy = 0;

    switch (wui_vars_copy.print_state) {
    case mpsPrinting:
        printing = busy = 1;
        ready = operational = 0;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        paused = busy = 1;
        ready = operational = 0;
        break;
    case mpsPaused:
        paused = 1;
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead:
        ready = operational = 0;
        busy = printing = 1;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        cancelling = busy = 1;
        ready = operational = 0;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        busy = 1;
        ready = operational = 0;
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsIdle:
    default:
        break;
    }

    snprintf(data, buf_len,
        "{"
        "\"telemetry\": {"
        "\"material\": \"%s\""
        "},"
        "\"temperature\": {"
        "\"tool0\": {"
        "\"actual\": %d.%.1d,"
        "\"target\": 0,"
        "\"offset\": 0"
        "},"
        "\"bed\": {"
        "\"actual\": %d.%.1d,"
        "\"target\": 0,"
        "\"offset\": 0"
        "},"
        "\"chamber\": {"
        "\"actual\": 0,"
        "\"target\": 0,"
        "\"offset\": 0"
        "}"
        "},"
        "\"sd\": {"
        "\"ready\": 1"
        "},"
        "\"state\": {"
        "\"text\": \"Operational\","
        "\"flags\": {"
        "\"operational\": %ld,"
        "\"paused\": %ld,"
        "\"printing\": %ld,"
        "\"cancelling\": %ld,"
        "\"pausing\": %ld,"
        "\"sdReady\": %ld,"
        "\"error\": %ld,"
        "\"ready\": %ld,"
        "\"closedOrError\": %ld,"
        "\"busy\": %ld"
        "}"
        "}"
        "}",
        filament_material,
        (int)wui_vars_copy.temp_nozzle, (int)((wui_vars_copy.temp_nozzle - (int)wui_vars_copy.temp_nozzle) * 10),
        (int)wui_vars_copy.temp_bed, (int)((wui_vars_copy.temp_bed - (int)wui_vars_copy.temp_bed) * 10),
        operational, paused, printing, cancelling, pausing, sd_ready,
        error, ready, closed_on_error, busy);
}

void get_version(char *data, const uint32_t buf_len) {
    snprintf(data, buf_len,
        "{"
        "\"api\": \"0.1\","
        "\"server\": \"2.0.0\","
        "\"text\": \"Prusa Local MINI 2.0.0\","
        "\"hostname\": \"prusa-mini\""
        "}");
}

void get_job(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    snprintf(data, buf_len,
        "{"
        "\"job\":{"
        "\"estimatedPrintTime\":%ld,"
        "\"file\":{"
        "\"date\":null,"
        "\"name\":%s,"
        "\"origin\":\"USB\","
        "\"path\":%s,"
        "\"size\":%ld"
        "}"
        "},"
        "\"state\":\"Printing\","
        "\"progress\":{"
        "\"completion\":0.06666666666666667,"
        "\"filepos\":629439.0666666667,"
        "\"printTime\":8,"
        "\"printTimeLeft\":112,"
        "\"pos_z_mm\":3.3333333333333335,"
        "\"printSpeed\":5.019598656078543,"
        "\"flow_factor\":2.2267336678608296,"
        "\"filament_status\":3"
        "},"
        "\"filament\":{"
        "\"length\":3,"
        "\"volume\":5.333333333333333"
        "}"
        "}", );
}

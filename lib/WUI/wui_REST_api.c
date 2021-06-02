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

#define BDY_WUI_API_BUFFER_SIZE 512

// for data exchange between wui thread and HTTP thread
static wui_vars_t wui_vars_copy;

void get_telemetry_for_local(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    const char *filament_material = get_selected_filament_name();

    snprintf(data, buf_len,
        "{"
        "\"telemetry\": {"
        "\"material\": \"%s\""
        "},"
        "\"temperature\": {"
        "\"tool0\": {"
        "\"actual\": %.2f,"
        "\"target\": 0,"
        "\"offset\": 0"
        "},"
        "\"bed\": {"
        "\"actual\": %.2f,"
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
        "\"ready\": true"
        "},"
        "\"state\": {"
        "\"text\": \"Operational\","
        "\"flags\": {"
        "\"operational\": true,"
        "\"paused\": false,"
        "\"printing\": false,"
        "\"cancelling\": false,"
        "\"pausing\": false,"
        "\"sdReady\": true,"
        "\"error\": false,"
        "\"ready\": true,"
        "\"closedOrError\": false,"
        "\"busy\": false"
        "}"
        "}"
        "}",
        filament_material, wui_vars_copy.temp_nozzle, wui_vars_copy.temp_bed);
}

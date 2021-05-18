#include "gui_time.hpp"
#include "stm32f4xx_hal.h" //HAL_GetTick

static uint32_t current_tick = 0;
static uint32_t current_tick_overflows = 0;

void gui::TickLoop() {
    uint32_t now = HAL_GetTick();
    if (current_tick > now) { //overflow
        ++current_tick_overflows;
    }
    current_tick = now;
}

uint32_t gui::GetTick() {
    return current_tick;
}

uint64_t gui::GetTickU64() {
    uint64_t u64_tick = uint64_t(current_tick_overflows);
    u64_tick <<= 32;
    u64_tick |= uint64_t(current_tick);
    return u64_tick;
}

uint32_t gui::GetTick_IgnoreTickLoop() {
    return HAL_GetTick();
}

uint32_t gui::GetTick_ForceActualization() {
    gui::TickLoop();
    return gui::GetTick();
};

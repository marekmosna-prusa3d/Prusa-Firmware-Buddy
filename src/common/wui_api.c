/*
 * wui_api.c
 * \brief   interface functions for Web User Interface library
 *
 *  Created on: April 22, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */
#include "wui_api.h"
#include "version.h"
#include "otp.h"
#include <string.h>
#include <stdio.h>
#include "ini_handler.h"
#include "eeprom.h"
#include "string.h"
#include <stdbool.h>

// static const uint16_t MAX_UINT16 = 65535;
static const uint32_t PRINTER_TYPE_ADDR = 0x0802002F;    // 1 B
static const uint32_t PRINTER_VERSION_ADDR = 0x08020030; // 1 B

uint32_t load_ini_file(ETH_config_t *config) {
    return ini_load_file(config);
}

uint32_t save_eth_params(ETH_config_t *ethconfig) {

    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_FLAGS)) {
        eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(ethconfig->lan.flag));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_ADDR, variant8_ui32(ethconfig->lan.addr_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_MSK, variant8_ui32(ethconfig->lan.msk_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_GW, variant8_ui32(ethconfig->lan.gw_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        variant8_t hostname = variant8_pchar(ethconfig->hostname, 0, 0);
        eeprom_set_var(EEVAR_LAN_HOSTNAME, hostname);
        //variant8_done() is not called, variant_pchar with init flag 0 doesnt hold its memory
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_TIMEZONE)) {
        eeprom_set_var(EEVAR_TIMEZONE, variant8_i8(ethconfig->timezone));
    }

    return 0;
}

uint32_t load_eth_params(ETH_config_t *ethconfig) {

    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_FLAGS)) {
        ethconfig->lan.flag = variant_get_ui8(eeprom_get_var(EEVAR_LAN_FLAG));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        ethconfig->lan.addr_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_ADDR));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        ethconfig->lan.msk_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_MSK));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        ethconfig->lan.gw_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_GW));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        variant8_t hostname = eeprom_get_var(EEVAR_LAN_HOSTNAME);
        variant8_t *pvar = &hostname;
        strlcpy(ethconfig->hostname, variant8_get_pch(hostname), ETH_HOSTNAME_LEN + 1);
        variant8_done(&pvar);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_TIMEZONE)) {
        ethconfig->timezone = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));
    }

    return 0;
}

void get_printer_info(printer_info_t *printer_info) {
    // FIRMWARE VERSION
    strlcpy(printer_info->firmware_version, project_version_full, FW_VER_STR_LEN);
    // PRINTER TYPE
    printer_info->printer_type = *(volatile uint8_t *)PRINTER_TYPE_ADDR;
    // PRINTER_VERSION
    printer_info->printer_version = *(volatile uint8_t *)PRINTER_VERSION_ADDR;
    // MAC ADDRESS
    parse_MAC_address(&printer_info->mac_address);
    // SERIAL NUMBER
    for (int i = 0; i < OTP_SERIAL_NUMBER_SIZE; i++) {
        printer_info->serial_number[i] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }
    // UUID - 96 bits
    volatile uint32_t *uuid_ptr = (volatile uint32_t *)OTP_STM32_UUID_ADDR;
    snprintf(printer_info->mcu_uuid, UUID_STR_LEN, "%08lx-%08lx-%08lx", *uuid_ptr, *(uuid_ptr + 1), *(uuid_ptr + 2));
}

void parse_MAC_address(mac_address_t *dest) {
    volatile uint8_t *mac_ptr = (volatile uint8_t *)OTP_MAC_ADDRESS_ADDR;
    snprintf(*dest, MAC_ADDR_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
        *mac_ptr, *(mac_ptr + 1), *(mac_ptr + 2), *(mac_ptr + 3), *(mac_ptr + 4), *(mac_ptr + 5));
}

void stringify_eth_for_screen(lan_descp_str_t *dest, ETH_config_t *config) {
    char addr[IP4_ADDR_STR_SIZE], msk[IP4_ADDR_STR_SIZE], gw[IP4_ADDR_STR_SIZE];
    mac_address_t mac;
    parse_MAC_address(&mac);

    if (get_eth_status() == ETH_NETIF_UP) {

        ip4addr_ntoa_r(&(config->lan.addr_ip4), addr, IP4_ADDR_STR_SIZE);
        ip4addr_ntoa_r(&(config->lan.msk_ip4), msk, IP4_ADDR_STR_SIZE);
        ip4addr_ntoa_r(&(config->lan.gw_ip4), gw, IP4_ADDR_STR_SIZE);

        snprintf(*dest, LAN_DESCP_SIZE, "IPv4 Address:\n%s\nIPv4 Netmask:\n%s\nIPv4 Gateway:\n%s\nMAC Address:\n%s",
            addr, msk, gw, mac);
    } else {
        snprintf(*dest, LAN_DESCP_SIZE, "NO CONNECTION\n\nMAC Address:\n%s", mac);
    }
}

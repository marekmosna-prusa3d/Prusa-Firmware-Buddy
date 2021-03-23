#pragma once

#include "lwip/ip_addr.h"

#define LAN_FLAG_ONOFF_POS (1 << 0) // position of ONOFF switch in lan.flag
#define LAN_FLAG_TYPE_POS  (1 << 1) // position of DHCP/STATIC switch in lan.flag

#define IS_LAN_OFF(flg)    (flg & LAN_FLAG_ONOFF_POS)        // returns true if flag is set to OFF
#define IS_LAN_ON(flg)     ((flg & LAN_FLAG_ONOFF_POS) == 0) // returns true if flag is set to ON
#define IS_LAN_STATIC(flg) (flg & LAN_FLAG_TYPE_POS)         // returns true if flag is set to STATIC
#define IS_LAN_DHCP(flg)   ((flg & LAN_FLAG_TYPE_POS) == 0)  // returns true if flag is set to DHCP

#define CHANGE_LAN_TO_STATIC(flg) (flg |= LAN_FLAG_TYPE_POS)   // flip lan type flg to STATIC
#define CHANGE_LAN_TO_DHCP(flg)   (flg &= ~LAN_FLAG_TYPE_POS)  // flip lan type flg to DHCP
#define TURN_LAN_ON(flg)          (flg &= ~LAN_FLAG_ONOFF_POS) // flip lan switch flg to ON
#define TURN_LAN_OFF(flg)         (flg |= LAN_FLAG_ONOFF_POS)  // flip lan switch flg to OFF

#define ETH_HOSTNAME_LEN 20 // ethernet hostname MAX length

#define ETHVAR_MSK(n_id) ((uint32_t)1 << (n_id))
#define ETHVAR_STATIC_LAN_ADDRS \
    (ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4) | ETHVAR_MSK(ETHVAR_LAN_MSK_IP4) | ETHVAR_MSK(ETHVAR_LAN_GW_IP4))

#define ETHVAR_STATIC_DNS_ADDRS \
    (ETHVAR_MSK(ETHVAR_DNS1_IP4) | ETHVAR_MSK(ETHVAR_DNS2_IP4))

#define ETHVAR_EEPROM_CONFIG \
    (ETHVAR_STATIC_LAN_ADDRS | ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_MSK(ETHVAR_HOSTNAME) | ETHVAR_MSK(ETHVAR_DNS1_IP4) | ETHVAR_MSK(ETHVAR_DNS2_IP4))

typedef enum {
    ETHVAR_LAN_FLAGS,    // uint8_t, lan.flag
    ETHVAR_HOSTNAME,     // char[20 + 1], hostname
    ETHVAR_LAN_ADDR_IP4, // ip4_addr_t, lan.addr_ip4
    ETHVAR_LAN_MSK_IP4,  // ip4_addr_t, lan.msk_ip4
    ETHVAR_LAN_GW_IP4,   // ip4_addr_t, lan.gw_ip4
    ETHVAR_TIMEZONE,     // int8_t, timezone
    ETHVAR_DNS1_IP4,     // ip_addr_t, dns1_ip4
    ETHVAR_DNS2_IP4,     // ip_addr_t, dns2_ip4
} ETHVAR_t;

typedef enum {
    ETH_UNLINKED,   // ETH cabel is unlinked
    ETH_NETIF_DOWN, // ETH interface is DOWN
    ETH_NETIF_UP,   // ETH interface is UP
} ETH_STATUS_t;

typedef enum {
    WUI_ETH_LINK_DOWN, // ETH cable is unplugged
    WUI_ETH_LINK_UP,   // ETH cable is plugged
} WUI_ETH_LINK_STATUS_t;

typedef enum {
    WUI_ETH_NETIF_DOWN, // ETH interface is down
    WUI_ETH_NETIF_UP,   // ETH interface is up
} WUI_ETH_NETIF_STATUS_t;

typedef struct {
    uint8_t flag;        // lan flags: pos0 = switch(ON=0, OFF=1), pos1 = type(DHCP=0, STATIC=1)
    ip4_addr_t addr_ip4; // user defined static ip4 address
    ip4_addr_t msk_ip4;  // user defined ip4 netmask
    ip4_addr_t gw_ip4;   // user define ip4 default gateway
} lan_t;

typedef struct {
    char hostname[ETH_HOSTNAME_LEN + 1]; // ETH hostname: MAX 20 chars
    int8_t timezone;                     // user defined difference to UTC
    ip_addr_t dns1_ip4;                  // user defined DNS #1
    ip_addr_t dns2_ip4;                  // user defined DNS #2
    lan_t lan;                           // user defined LAN configurations
    uint32_t var_mask;                   // mask for setting ethvars
} ETH_config_t;

#ifdef __cplusplus
extern "C" {
#endif

void set_eth_update_mask(uint32_t var_mask);
void clear_eth_update_mask();
uint32_t get_eth_update_mask();

#ifdef __cplusplus
}
#endif

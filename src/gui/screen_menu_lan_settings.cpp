/*
 * screen_lan_settings.cpp
 *
 *  Created on: Nov 27, 2019
 *      Author: Migi
 */

#include "gui.hpp"
#include "marlin_client.h"
#include "ini_handler.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "wui_api.h"
#include "config.h"
#include "RAII.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//Eth static class used by menu and its items
//And NO David a do not want to use singleton here
class Eth {
public:
    enum class Msg : uint8_t {
        NoMsg,
        StaicAddrErr,
        NoUSB,
        SaveOK,
        SaveNOK,
        LoadOK,
        LoadNOK
    };

    enum class LinkType : uint32_t {
        Off,
        Lan,
        Wifi
    };

    static Msg msg;
    static uint8_t lan_flag;
    static uint8_t saveIni();

public:
    static uint8_t GetFlag();
    static void SaveMessage(); // to show message while saving ini file
    static void LoadIni();
    static void Lan();
    static void Wifi();
    static void Off();
    static bool IsStatic();
    static bool IsDHCP();
    static LinkType LinkStatus();
    static void Init();
    static bool IsUpdated();
    static bool SetStatic();
    static bool SetDHCP();
    static Msg ConsumeMsg();
};

Eth::Msg Eth::msg = Eth::Msg::NoMsg;
/*****************************************************************************/
//Eth methods
uint8_t Eth::GetFlag() {
    return get_lan_flag();
}

uint8_t Eth::saveIni() {
    ETH_config_t ethconfig = {};
    ini_file_str_t ini_str;
    ethconfig.var_mask = ETHVAR_EEPROM_CONFIG;
    load_eth_params(&ethconfig);
    if (LinkStatus() == LinkType::Lan) {
        stringify_eth_for_ini(&ini_str, &ethconfig);
    } else if (LinkStatus() == LinkType::Wifi) {
        stringify_wifi_for_ini(&ini_str, &ethconfig);
    } else {
        return 0;
    }
    return ini_save_file((const char *)&ini_str);
}

void Eth::Off() {
    ETH_config_t ethconfig = {};
    TURN_NETWORK_OFF(ethconfig.lan.flag);
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    save_eth_params(&ethconfig);
    set_eth_update_mask(ethconfig.var_mask);
}

void Eth::Lan() {
    ETH_config_t ethconfig = {};
    TURN_LAN_ON(ethconfig.lan.flag);
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    save_eth_params(&ethconfig);
    set_eth_update_mask(ethconfig.var_mask);
}

void Eth::Wifi() {
    ETH_config_t ethconfig = {};
    TURN_WIFI_ON(ethconfig.lan.flag);
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    save_eth_params(&ethconfig);
    set_eth_update_mask(ethconfig.var_mask);
}

bool Eth::IsStatic() {
    return IS_LAN_STATIC(GetFlag());
}

bool Eth::IsDHCP() {
    return IS_LAN_DHCP(GetFlag());
}

Eth::LinkType Eth::LinkStatus() {
    if (!IS_LAN_OFF(GetFlag())) {
        return LinkType::Lan;
    } else if (!IS_WIFI_OFF(GetFlag())) {
        return LinkType::Wifi;
    } else {
        return LinkType::Off;
    }
}

bool Eth::IsUpdated() {
    return false;
}

bool Eth::SetStatic() {
    ETH_config_t ethconfig = {};
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_STATIC_LAN_ADDRS | ETHVAR_STATIC_DNS_ADDRS;
    load_eth_params(&ethconfig);

    if (ethconfig.lan.addr_ip4.addr == 0) {
        msg = Msg::StaicAddrErr;
        return false;
    }
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    CHANGE_LAN_TO_STATIC(ethconfig.lan.flag);
    save_eth_params(&ethconfig);
    set_eth_update_mask(ethconfig.var_mask);
    return true;
}

bool Eth::SetDHCP() {
    ETH_config_t ethconfig = {};
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    CHANGE_LAN_TO_DHCP(ethconfig.lan.flag);
    save_eth_params(&ethconfig);
    set_eth_update_mask(ethconfig.var_mask);
    return true;
}

Eth::Msg Eth::ConsumeMsg() {
    Msg ret = msg;
    msg = Msg::NoMsg;
    return ret;
}

void Eth::SaveMessage() {
    if (!(marlin_vars()->media_inserted)) {
        msg = Msg::NoUSB;
    } else {
        if (saveIni()) { // !its possible to save empty configurations!
            msg = Msg::SaveOK;
        } else {
            msg = Msg::SaveNOK;
        }
    }
}

void Eth::LoadIni() {
    if (!(marlin_vars()->media_inserted)) {
        msg = Msg::NoUSB;
    } else {
        ETH_config_t ethconfig = {};
        if (load_ini_file(&ethconfig)) {
            ethconfig.var_mask = ETHVAR_EEPROM_CONFIG;
            save_eth_params(&ethconfig);
            set_eth_update_mask(ethconfig.var_mask);
            msg = Msg::LoadOK;
        } else {
            msg = Msg::LoadNOK;
        }
    }
}

/*****************************************************************************/
//ITEMS
class MI_LAN_ONOFF : public WI_SWITCH_t<3> {
    constexpr static const char *const label = "NETWORK"; //do not translate
    constexpr static const char *str_disconnect = "OFF";  //do not translate
    constexpr static const char *str_lan = "LAN";         //do not translate
    constexpr static const char *str_wifi = "WIFI";       //do not translate
public:
    MI_LAN_ONOFF()
        : WI_SWITCH_t(static_cast<uint32_t>(Eth::LinkStatus()), string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_disconnect), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_lan), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_wifi)) {}

    virtual void OnChange(size_t old_index) override {
        Screens::Access()->Get()->WindowEvent(nullptr,
            GUI_event_t::CHILD_CLICK,
            (void *)index);
    }
};

class MI_LAN_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "NETWORK IP"; //do not translate
    constexpr static const char *str_static = "static";      //do not translate
    constexpr static const char *str_DHCP = "DHCP";          //do not translate

public:
    MI_LAN_IP_t()
        : WI_SWITCH_t(Eth::IsStatic() ? 1 : 0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, Eth::LinkStatus() == Eth::LinkType::Off ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_DHCP), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_static)) {}
    virtual void OnChange(size_t old_index) override {
        bool success = old_index == 0 ? Eth::SetStatic() : Eth::SetDHCP();
        if (!success)
            this->SetIndex(old_index);
    }
};

class MI_LAN_SAVE : public WI_LABEL_t {
    constexpr static const char *const label = N_("Save Settings");

public:
    MI_LAN_SAVE()
        : WI_LABEL_t(_(label), 0, Eth::LinkStatus() == Eth::LinkType::Off ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        Eth::SaveMessage();
    }
};

class MI_LAN_LOAD : public WI_LABEL_t {
    constexpr static const char *const label = N_("Load Settings");

public:
    MI_LAN_LOAD()
        : WI_LABEL_t(_(label), 0, Eth::LinkStatus() == Eth::LinkType::Off ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        Eth::LoadIni();
    }
};

/*****************************************************************************/
//parent alias
using MenuContainer = WinMenuContainer<MI_RETURN, MI_LAN_ONOFF, MI_LAN_IP_t, MI_LAN_SAVE, MI_LAN_LOAD>;

class ScreenMenuLanSettings : public AddSuperWindow<screen_t> {
    constexpr static const char *label = N_("NETWORK SETTINGS");
    static constexpr size_t helper_lines = 8;
    static constexpr int helper_font = IDR_FNT_SPECIAL;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;

    lan_descp_str_t plan_str; //todo not initialized in constructor
    bool msg_shown;           //todo not initialized in constructor
    void refresh_addresses();
    void show_msg(Eth::Msg msg);

public:
    ScreenMenuLanSettings();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuLanSettings::ScreenMenuLanSettings()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectScreen.Height()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes) {
    header.SetText(_(label));
    help.font = resource_font(helper_font);
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list

    refresh_addresses();
    msg_shown = false;
}

/*****************************************************************************/
//non static member function definition
void ScreenMenuLanSettings::refresh_addresses() {
    ETH_config_t ethconfig = {};
    get_eth_address(&ethconfig);
    if (Eth::LinkStatus() == Eth::LinkType::Wifi) {
        stringify_wifi_for_screen(&plan_str, &ethconfig);
    } else {
        stringify_eth_for_screen(&plan_str, &ethconfig);
    }
    // this MakeRAM is safe - plan_str is statically allocated
    help.text = string_view_utf8::MakeRAM((const uint8_t *)plan_str);
    help.Invalidate();
}

ScreenFactory::UniquePtr GetScreenMenuLanSettings() {
    return ScreenFactory::Screen<ScreenMenuLanSettings>();
}

void ScreenMenuLanSettings::show_msg(Eth::Msg msg) {
    if (msg_shown)
        return;
    AutoRestore<bool> AR(msg_shown);
    msg_shown = true;
    switch (msg) {
    case Eth::Msg::StaicAddrErr:
        MsgBoxError(_("Static IPv4 addresses were not set."), Responses_Ok);
        break;
    case Eth::Msg::NoUSB:
        MsgBoxError(_("Please insert a USB drive and try again."), Responses_Ok);
        break;
    case Eth::Msg::SaveOK:
        MsgBoxInfo(_("The settings have been saved successfully in the \"lan_settings.ini\" file."), Responses_Ok);
        break;
    case Eth::Msg::SaveNOK:
        MsgBoxError(_("There was an error saving the settings in the \"lan_settings.ini\" file."), Responses_Ok);
        break;
    case Eth::Msg::LoadOK:
        MsgBoxInfo(_("Settings successfully loaded"), Responses_Ok);
        break;
    case Eth::Msg::LoadNOK:
        MsgBoxError(_("IP addresses or parameters are not valid or the file \"lan_settings.ini\" is not in the root directory of the USB drive."), Responses_Ok);
        break;
    default:
        break;
    }
}

void ScreenMenuLanSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        refresh_addresses();
    }

    if (event == GUI_event_t::CHILD_CLICK) {
        Eth::LinkType action = static_cast<Eth::LinkType>((uint32_t)param);
        switch (action) {
        case Eth::LinkType::Off:
            Eth::Off();
            container.Item<MI_LAN_IP_t>().Disable();
            container.Item<MI_LAN_SAVE>().Disable();
            container.Item<MI_LAN_LOAD>().Disable();
            break;
        case Eth::LinkType::Lan:
            Eth::Lan();
            container.Item<MI_LAN_IP_t>().Enable();
            container.Item<MI_LAN_SAVE>().Enable();
            container.Item<MI_LAN_LOAD>().Enable();
            break;
        case Eth::LinkType::Wifi:
            Eth::Wifi();
            container.Item<MI_LAN_IP_t>().Enable();
            container.Item<MI_LAN_SAVE>().Enable();
            container.Item<MI_LAN_LOAD>().Enable();
            break;
        default:
            break;
        }
        menu.ForceMenuRedraw();
    }

    show_msg(Eth::ConsumeMsg());
    SuperWindowEvent(sender, event, param);
}

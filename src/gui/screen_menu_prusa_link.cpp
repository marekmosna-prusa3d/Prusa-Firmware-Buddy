/*
 * screen_menu_prusa_link.cpp
 *
 *  Created on: Jun 12, 2021
 *      Author: Marek Mosna
 */

#include "gui.hpp"
#include <string.h>
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "RAII.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//ITEMS
class MI_PL_ONOFF : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "LAN"; //do not translate

public:
    MI_PL_ONOFF()
        : WI_SWITCH_OFF_ON_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void OnChange(size_t old_index) override {
        ;
    }
};

class MI_API_KEY : public WI_LABEL_t {
    constexpr static const char *const label = N_("Save Settings");

public:
    MI_API_KEY()
        : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        ;
    }
};

/*****************************************************************************/
//parent alias
using MenuContainer = WinMenuContainer<MI_RETURN, MI_PL_ONOFF, MI_API_KEY>;

class ScreenPrusaLink : public AddSuperWindow<screen_t> {
    constexpr static const char *label = N_("Prusa Link");

    MenuContainer container;
    window_header_t header;

public:
    ScreenPrusaLink();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

ScreenPrusaLink::ScreenPrusaLink()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , header(this) {
    header.SetText(_(label));
}

void ScreenPrusaLink::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        ;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenFactory::UniquePtr GetScreenPrusaLink() {
    return ScreenFactory::Screen<ScreenPrusaLink>();
}

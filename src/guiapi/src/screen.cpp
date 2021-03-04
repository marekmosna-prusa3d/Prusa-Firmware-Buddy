/**
 * @file screen.cpp
 * @author Radek Vana
 * @date 2020-11-29
 */

#include "screen.hpp"

screen_t::screen_t(window_t *parent, Rect16 rect, win_type_t type, is_closed_on_timeout_t timeout, is_closed_on_serial_t serial)
    : AddSuperWindow<window_frame_t>(parent, rect, type, timeout, serial)
    , captured_normal_window(nullptr)
    , first_dialog(nullptr)
    , last_dialog(nullptr)
    , first_strong_dialog(nullptr)
    , last_strong_dialog(nullptr)
    , first_popup(nullptr)
    , last_popup(nullptr) {}

bool screen_t::registerSubWin(window_t &win) {
    switch (win.GetType()) {
    case win_type_t::normal:
        registerAnySubWin(win, first_normal, last_normal);
        break;
    case win_type_t::dialog:
        registerAnySubWin(win, first_dialog, last_dialog);
        if (&win == first_dialog && last_normal) { //connect to list
            last_normal->SetNext(&win);
            win.SetNext(first_strong_dialog ? first_strong_dialog : first_popup);
        }
        break;
    case win_type_t::strong_dialog:
        registerAnySubWin(win, first_strong_dialog, last_strong_dialog);
        //connect to list
        if (&win == first_strong_dialog) {
            if (last_dialog) {
                last_dialog->SetNext(&win);
            } else if (last_normal) {
                last_normal->SetNext(&win);
            }
            win.SetNext(first_popup);
        }
        break;
    case win_type_t::popup:
        if (!canRegisterPopup(win)) {
            return false;
        }
        registerAnySubWin(win, first_popup, last_popup);
        //connect to list
        if (&win == first_popup) {
            if (last_strong_dialog) {
                last_strong_dialog->SetNext(&win);
            } else if (last_dialog) {
                last_dialog->SetNext(&win);
            } else if (last_normal) {
                last_normal->SetNext(&win);
            }
        }
        break;
    default:
        return false;
    }

    unregisterConflictingPopUps(win.rect, win.GetType() == win_type_t::popup ? &win : nullptr);

    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
    return true;
}

void screen_t::unregisterConflictingPopUps(Rect16 rect, window_t *end) {

    if (!getFirstPopUp())
        return;
    WinFilterIntersectingPopUp filter_popup(rect);
    window_t *popup;
    //find intersecting popups and close them
    while ((popup = findFirst(getFirstPopUp(), end, filter_popup)) != end) {
        UnregisterSubWin(popup);
    }
}

bool screen_t::canRegisterPopup(window_t &win) {
    WinFilterIntersectingDialog filter(win.rect);
    //find intersecting non popup
    if (findFirst(first_normal, nullptr, filter)) {
        //registration failed
        win.SetParent(nullptr);
        return false;
    }
    return true;
}

void screen_t::hideSubwinsBehindDialogs() {
    if ((!first_normal) || (!last_normal))
        return; //error, must have normal window
    window_t *pBeginAbnormal = first_popup;
    if (first_strong_dialog)
        pBeginAbnormal = first_strong_dialog;
    if (first_dialog)
        pBeginAbnormal = first_dialog;
    if (!pBeginAbnormal)
        return; //nothing to hide
    window_t *pEndAbnormal = nullptr;

    //find last_normal visible dialog
    WinFilterVisible filter_visible;
    window_t *pLastVisibleDialog;
    while ((pLastVisibleDialog = findLast(pBeginAbnormal, pEndAbnormal, filter_visible)) != pEndAbnormal) {
        //hide all conflicting windows
        WinFilterIntersectingVisible filter_intersecting(pLastVisibleDialog->rect);
        window_t *pIntersectingWin;
        while ((pIntersectingWin = findFirst(first_normal, pLastVisibleDialog, filter_intersecting)) != pLastVisibleDialog) {
            pIntersectingWin->HideBehindDialog();
        }

        pEndAbnormal = pLastVisibleDialog; //new end of search
    }
}

void screen_t::unregisterSubWin(window_t &win) {
    switch (win.GetType()) {
    case win_type_t::normal:
        unregisterAnySubWin(win, first_normal, last_normal);
        return; //return - normal window does not affect other windows
    case win_type_t::dialog:
        unregisterAnySubWin(win, first_dialog, last_dialog);
        break;
    case win_type_t::popup:
        unregisterAnySubWin(win, first_popup, last_popup);
        break;
    case win_type_t::strong_dialog:
        unregisterAnySubWin(win, first_strong_dialog, last_strong_dialog);
        break;
    }

    clearAllHiddenBehindDialogFlags();
    hideSubwinsBehindDialogs();
}

bool screen_t::CaptureNormalWindow(window_t &win) {
    if (win.GetParent() != this || win.GetType() != win_type_t::normal)
        return false;
    window_t *last_captured = GetCapturedWindow();
    if (last_captured) {
        last_captured->WindowEvent(this, GUI_event_t::CAPT_0, 0); //will not resend event to anyone
    }
    captured_normal_window = &win;
    win.WindowEvent(this, GUI_event_t::CAPT_1, 0); //will not resend event to anyone
    gui_invalidate();

    return true;
}

void screen_t::ReleaseCaptureOfNormalWindow() {
    if (captured_normal_window) {
        captured_normal_window->WindowEvent(this, GUI_event_t::CAPT_0, 0); //will not resend event to anyone
    }
    captured_normal_window = nullptr;
    gui_invalidate();
}

bool screen_t::IsChildCaptured() const {
    return captured_normal_window != nullptr;
}

window_t *screen_t::getCapturedNormalWin() const {
    return captured_normal_window;
}

window_t *screen_t::getFirstDialog() const {
    return first_dialog;
}

window_t *screen_t::getLastDialog() const {
    return last_dialog;
}

window_t *screen_t::getFirstStrongDialog() const {
    return first_strong_dialog;
}

window_t *screen_t::getLastStrongDialog() const {
    return last_strong_dialog;
}

window_t *screen_t::getFirstPopUp() const {
    return first_popup;
}

window_t *screen_t::getLastPopUp() const {
    return last_popup;
}

window_t *screen_t::GetCapturedWindow() {
    if (last_strong_dialog)
        return last_strong_dialog->GetCapturedWindow();
    if (last_dialog)
        return last_dialog->GetCapturedWindow();
    if (captured_normal_window)
        return captured_normal_window->GetCapturedWindow();
    return this;
}
